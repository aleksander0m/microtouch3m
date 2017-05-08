/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * All rights reserved.
 *
 * Author: Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>

#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <libusb.h>

#include "common.h"

#include "microtouch3m.h"
#include "microtouch3m-log.h"

#include "ihex.h"

/* Define this symbol to enable verbose traces */
/* #define ENABLE_TRACES */

#if !defined __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
# error GCC built-in atomic method support is required
#endif

/******************************************************************************/
/* Common */

#define MICROTOUCH3M_VID 0x0596
#define MICROTOUCH3M_PID 0x0001

#if defined ENABLE_TRACES
static void
trace_buffer (const char *name, const uint8_t *buffer, size_t buffer_size)
{
    char *hex;

    hex = strhex (buffer, buffer_size, ":");
    microtouch3m_log ("%s (%d bytes): %s", name, buffer_size, hex);
    free (hex);
}
#else
#define trace_buffer(...)
#endif

/******************************************************************************/
/* Status */

static const char *status_str[] = {
    [MICROTOUCH3M_STATUS_OK]                = "success",
    [MICROTOUCH3M_STATUS_FAILED]            = "failed",
    [MICROTOUCH3M_STATUS_NO_MEMORY]         = "no memory",
    [MICROTOUCH3M_STATUS_INVALID_ARGUMENTS] = "invalid arguments",
    [MICROTOUCH3M_STATUS_INVALID_IO]        = "invalid input/output",
    [MICROTOUCH3M_STATUS_INVALID_DATA]      = "invalid data",
    [MICROTOUCH3M_STATUS_INVALID_FORMAT]    = "invalid format",
    [MICROTOUCH3M_STATUS_INVALID_STATE]     = "invalid state",
};

const char *
microtouch3m_status_to_string (microtouch3m_status_t st)
{
    return ((st < (sizeof (status_str) / sizeof (status_str[0]))) ? status_str[st] : "unknown");
}

/******************************************************************************/
/* Library context */

struct microtouch3m_context_s {
    volatile int    refcount;
    libusb_context *usb;
};

microtouch3m_context_t *
microtouch3m_context_new (void)
{
    microtouch3m_context_t *ctx;

    ctx = calloc (1, sizeof (microtouch3m_context_t));
    if (!ctx)
        return NULL;

    if (libusb_init (&ctx->usb) != 0) {
        free (ctx);
        return NULL;
    }

    ctx->refcount = 1;
    return ctx;
}

microtouch3m_context_t *
microtouch3m_context_ref (microtouch3m_context_t *ctx)
{
    assert (ctx);
    __sync_fetch_and_add (&ctx->refcount, 1);
    return ctx;
}

void
microtouch3m_context_unref (microtouch3m_context_t *ctx)
{
    assert (ctx);
    if (__sync_fetch_and_sub (&ctx->refcount, 1) != 1)
        return;

    assert (ctx->usb);
    libusb_exit (ctx->usb);

    free (ctx);
}

/******************************************************************************/
/* Device */

struct microtouch3m_device_s {
    volatile int            refcount;
    microtouch3m_context_t *ctx;
    libusb_device          *usbdev;
    libusb_device_handle   *usbhandle;
};

static microtouch3m_device_t *
device_new_by_usbdev (microtouch3m_context_t *ctx,
                      libusb_device          *usbdev)
{
    microtouch3m_device_t *dev;

    dev = calloc (1, sizeof (microtouch3m_device_t));
    if (!dev)
        goto outerr;

    dev->ctx      = microtouch3m_context_ref (ctx);
    dev->refcount = 1;
    dev->usbdev   = usbdev;
    return dev;

outerr:
    if (usbdev)
        libusb_unref_device (usbdev);
    if (dev)
        free (dev);
    return NULL;
}

static libusb_device *
find_usb_device_by_bus (microtouch3m_context_t *ctx,
                        uint8_t                 bus_number,
                        uint8_t                 device_address)
{
    libusb_device **list = NULL;
    libusb_device  *found = NULL;
    unsigned int    i;
    ssize_t         ret;

    if ((ret = libusb_get_device_list (ctx->usb, &list)) < 0) {
        microtouch3m_log ("error: couldn't list USB devices: %s", libusb_strerror (ret));
        return NULL;
    }

    assert (list);
    for (i = 0; !found && list[i]; i++) {
        libusb_device                   *iter;
        uint8_t                          iter_bus_number;
        uint8_t                          iter_device_address;
        struct libusb_device_descriptor  desc;

        iter = list[i];

        iter_bus_number = libusb_get_bus_number (iter);
        if (bus_number != iter_bus_number && bus_number != 0)
            continue;

        iter_device_address = libusb_get_device_address (iter);
        if (device_address != iter_device_address && device_address != 0)
            continue;

        if (libusb_get_device_descriptor (iter, &desc) != 0)
            continue;

        if (desc.idVendor != MICROTOUCH3M_VID)
            continue;

        if (desc.idProduct != MICROTOUCH3M_PID)
            continue;

        found = libusb_ref_device (iter);
        microtouch3m_log ("found MicroTouch 3M device at %03u:%03u", iter_bus_number, iter_device_address);
    }

    libusb_free_device_list (list, 1);

    if (!found)
        microtouch3m_log ("error: couldn't find MicroTouch 3M device at %03u:%03u", bus_number, device_address);

    return found;
}

microtouch3m_device_t *
microtouch3m_device_new_by_usb_bus (microtouch3m_context_t *ctx,
                                    uint8_t                 bus_number,
                                    uint8_t                 device_address)
{
    libusb_device *usbdev;

    usbdev = find_usb_device_by_bus (ctx, bus_number, device_address);
    if (!usbdev)
        return NULL;

    /* On device creation failure, usbdev is consumed as well */
    return device_new_by_usbdev (ctx, usbdev);
}

#define MAX_PORT_NUMBERS 7

static libusb_device *
find_usb_device_by_location (microtouch3m_context_t *ctx,
                             const uint8_t          *port_numbers,
                             int                     port_numbers_len)
{
    libusb_device **list = NULL;
    libusb_device  *found = NULL;
    unsigned int    i;
    ssize_t         ret;

    if ((ret = libusb_get_device_list (ctx->usb, &list)) < 0) {
        microtouch3m_log ("error: couldn't list USB devices: %s", libusb_strerror (ret));
        return NULL;
    }

    assert (list);
    for (i = 0; !found && list[i]; i++) {
        libusb_device                   *iter;
        struct libusb_device_descriptor  desc;
        uint8_t                          iter_port_numbers[MAX_PORT_NUMBERS];
        int                              iter_port_numbers_len;
        char                            *location_str;

        iter = list[i];

        iter_port_numbers_len = libusb_get_port_numbers (iter, (uint8_t *) iter_port_numbers, MAX_PORT_NUMBERS);

        /* We're looking for an exact match, so quick check on the number of ports */
        if (iter_port_numbers_len != port_numbers_len)
            continue;

        /* Compare arrays completely */
        if (memcmp (port_numbers, iter_port_numbers, port_numbers_len * sizeof (uint8_t)) != 0)
            continue;

        if (libusb_get_device_descriptor (iter, &desc) != 0)
            continue;

        if (desc.idVendor != MICROTOUCH3M_VID)
            continue;

        if (desc.idProduct != MICROTOUCH3M_PID)
            continue;

        found = libusb_ref_device (iter);

        location_str = str_usb_location (libusb_get_bus_number (found), port_numbers, port_numbers_len);
        microtouch3m_log ("found MicroTouch 3M device at %s", location_str ? location_str : "unknown");
        free (location_str);
    }

    libusb_free_device_list (list, 1);

    if (!found)
        microtouch3m_log ("error: couldn't find MicroTouch 3M device at given location");

    return found;
}

microtouch3m_device_t *
microtouch3m_device_new_by_usb_location (microtouch3m_context_t *ctx,
                                         const uint8_t          *port_numbers,
                                         int                     port_numbers_len)
{
    libusb_device *usbdev;

    usbdev = find_usb_device_by_location (ctx, port_numbers, port_numbers_len);
    if (!usbdev)
        return NULL;

    /* On device creation failure, usbdev is consumed as well */
    return device_new_by_usbdev (ctx, usbdev);
}

microtouch3m_device_t *
microtouch3m_device_ref (microtouch3m_device_t *dev)
{
    assert (dev);
    __sync_fetch_and_add (&dev->refcount, 1);
    return dev;
}

void
microtouch3m_device_unref (microtouch3m_device_t *dev)
{
    assert (dev);
    if (__sync_fetch_and_sub (&dev->refcount, 1) != 1)
        return;

    assert (dev->usbdev);
    libusb_unref_device (dev->usbdev);

    assert (dev->ctx);
    microtouch3m_context_unref (dev->ctx);

    free (dev);
}

uint8_t
microtouch3m_device_get_usb_bus_number (microtouch3m_device_t *dev)
{
    return libusb_get_bus_number (dev->usbdev);
}

uint8_t
microtouch3m_device_get_usb_device_address (microtouch3m_device_t *dev)
{
    return libusb_get_device_address (dev->usbdev);
}

uint8_t
microtouch3m_device_get_usb_location (microtouch3m_device_t *dev,
                                      uint8_t               *port_numbers,
                                      int                    port_numbers_len)
{
    int n;

    return (((n = libusb_get_port_numbers (dev->usbdev, port_numbers, port_numbers_len)) < 0) ? 0 : n);
}

/******************************************************************************/
/* Open and close */

microtouch3m_status_t
microtouch3m_device_open (microtouch3m_device_t *dev)
{
    assert (dev);

    if (!dev->usbhandle && libusb_open (dev->usbdev, &dev->usbhandle) < 0) {
        microtouch3m_log ("error: couldn't open usb device");
        return MICROTOUCH3M_STATUS_FAILED;
    }

    return MICROTOUCH3M_STATUS_OK;
}

void
microtouch3m_device_close (microtouch3m_device_t *dev)
{
    assert (dev);

    if (!dev->usbhandle)
        return;

    libusb_close (dev->usbhandle);
    dev->usbhandle = NULL;
}

/******************************************************************************/
/* IN/OUT requests */

enum request_e {
    REQUEST_GET_PARAMETER_BLOCK = 0x02,
    REQUEST_SET_PARAMETER_BLOCK = 0x03,
    REQUEST_RESET               = 0x07,
    REQUEST_GET_PARAMETER       = 0x10,
    REQUEST_SET_PARAMETER       = 0x11,
};

enum parameter_id_e {
    PARAMETER_ID_CONTROLLER_NOVRAM = 0x0000,
    PARAMETER_ID_CONTROLLER_EEPROM = 0x0020,
};

enum report_id_e {
    REPORT_ID_PARAMETER = 0x04,
};

struct parameter_report_s {
    uint8_t  report_id;
    uint16_t data_size;
    uint8_t  data [];
} __attribute__((packed));

static microtouch3m_status_t
run_in_request (microtouch3m_device_t     *dev,
                enum request_e             parameter_cmd,
                uint16_t                   parameter_value,
                uint16_t                   parameter_index,
                struct parameter_report_s *parameter_report,
                size_t                     parameter_report_size)
{
    int desc_size;

    assert (dev);
    assert (parameter_report);

    if ((desc_size = libusb_control_transfer (dev->usbhandle,
                                              LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                              parameter_cmd,
                                              parameter_value,
                                              parameter_index,
                                              (uint8_t *) parameter_report,
                                              parameter_report_size,
                                              5000)) < 0) {
        microtouch3m_log ("error: couldn't run IN request 0x%02x value 0x%04x index 0x%04x: %s",
                          parameter_cmd, parameter_value, parameter_index, libusb_strerror (desc_size));
        return MICROTOUCH3M_STATUS_INVALID_IO;
    }

    if (desc_size != parameter_report_size) {
        microtouch3m_log ("error: couldn't run IN request 0x%02x value 0x%04x index 0x%04x: invalid data size read (%d != %d)",
                          parameter_cmd, parameter_value, parameter_index, desc_size, parameter_report_size);
        return MICROTOUCH3M_STATUS_INVALID_DATA;
    }

    if (parameter_report->report_id != REPORT_ID_PARAMETER) {
        microtouch3m_log ("error: couldn't run IN request 0x%02x value 0x%04x index 0x%04x: invalid report id (%d != %d)",
                          parameter_cmd, parameter_value, parameter_index, parameter_report->report_id, REPORT_ID_PARAMETER);
        return MICROTOUCH3M_STATUS_INVALID_DATA;
    }

    if (parameter_report->data_size != (parameter_report_size - sizeof (struct parameter_report_s))) {
        microtouch3m_log ("error: couldn't run IN request 0x%02x value 0x%04x index 0x%04x: invalid read data size reported (%d != %d)",
                          parameter_cmd, parameter_value, parameter_index, parameter_report->data_size, (parameter_report_size - sizeof (struct parameter_report_s)));
        return MICROTOUCH3M_STATUS_INVALID_FORMAT;
    }

    microtouch3m_log ("successfully run IN request 0x%02x value 0x%04x index 0x%04x",
                      parameter_cmd, parameter_value, parameter_index);
    return MICROTOUCH3M_STATUS_OK;
}

static microtouch3m_status_t
run_out_request (microtouch3m_device_t *dev,
                 enum request_e         parameter_cmd,
                 uint16_t               parameter_value,
                 uint16_t               parameter_index,
                 const uint8_t         *parameter_data,
                 size_t                 parameter_data_size)
{
    int desc_size;

    assert (dev);

    if ((desc_size = libusb_control_transfer (dev->usbhandle,
                                              LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                              parameter_cmd,
                                              parameter_value,
                                              parameter_index,
                                              (uint8_t *) parameter_data,
                                              parameter_data_size,
                                              5000)) < 0) {
        microtouch3m_log ("error: couldn't run OUT request 0x%02x value 0x%04x index 0x%04x data %u bytes: %s",
                          parameter_cmd, parameter_value, parameter_index, parameter_data_size, libusb_strerror (desc_size));
        return MICROTOUCH3M_STATUS_INVALID_IO;
    }

    if (desc_size != parameter_data_size) {
        microtouch3m_log ("error: couldn't run OUT request 0x%02x value 0x%04x index 0x%04x: invalid data size written (%d != %d)",
                          parameter_cmd, parameter_value, parameter_index, desc_size, parameter_data_size);
        return MICROTOUCH3M_STATUS_INVALID_DATA;
    }

    microtouch3m_log ("successfully run OUT request 0x%02x value 0x%04x index 0x%04x data %u bytes",
                      parameter_cmd, parameter_value, parameter_index, parameter_data_size);
    return MICROTOUCH3M_STATUS_OK;
}

/******************************************************************************/
/* Reset */

static const char *reset_str[] = {
    [MICROTOUCH3M_DEVICE_RESET_SOFT]   = "soft",
    [MICROTOUCH3M_DEVICE_RESET_HARD]   = "hard",
    [MICROTOUCH3M_DEVICE_RESET_REBOOT] = "reboot",
};

const char *
microtouch3m_device_reset_to_string (microtouch3m_device_reset_t reset)
{
    return (((reset < (sizeof (reset_str) / sizeof (reset_str[0]))) && (reset_str[reset])) ? reset_str[reset] : "unknown");
}

microtouch3m_status_t
microtouch3m_device_reset (microtouch3m_device_t       *dev,
                           microtouch3m_device_reset_t  reset)
{
    microtouch3m_status_t st;

    microtouch3m_log ("requesting controller reset: %s", microtouch3m_device_reset_to_string (reset));
    if ((st = run_out_request (dev,
                               REQUEST_RESET,
                               reset,
                               0x0000,
                               NULL, 0)) != MICROTOUCH3M_STATUS_OK)
        return st;

    /* Success! */
    microtouch3m_log ("successfully requested controller reset");
    return MICROTOUCH3M_STATUS_OK;
}

/******************************************************************************/
/* Firmware dump */

#define PARAMETER_REPORT_FIRMWARE_DUMP_DATA_SIZE 64

struct parameter_report_firmware_dump_s {
    struct parameter_report_s header;
    uint8_t                   data [PARAMETER_REPORT_FIRMWARE_DUMP_DATA_SIZE];
} __attribute__((packed));

microtouch3m_status_t
microtouch3m_device_firmware_dump (microtouch3m_device_t *dev,
                                   uint8_t               *buffer,
                                   size_t                 buffer_size)
{
    uint16_t     offset;
    unsigned int i;

    assert (dev);
    assert (buffer);

    if (buffer_size < MICROTOUCH3M_FW_IMAGE_SIZE) {
        microtouch3m_log ("error: not enough space in buffer to contain the full firmware image file (%zu < %zu)",
                          buffer_size, MICROTOUCH3M_FW_IMAGE_SIZE);
        return MICROTOUCH3M_STATUS_INVALID_ARGUMENTS;
    }

    if (!dev->usbhandle) {
        microtouch3m_log ("error: device not open");
        return MICROTOUCH3M_STATUS_INVALID_STATE;
    }

    microtouch3m_log ("reading firmware from controller EEPROM...");

    for (i = 0, offset = 0; offset < MICROTOUCH3M_FW_IMAGE_SIZE; offset += PARAMETER_REPORT_FIRMWARE_DUMP_DATA_SIZE, i++) {
        struct parameter_report_firmware_dump_s parameter_report;
        microtouch3m_status_t                   st;

        if ((st = run_in_request (dev,
                                  REQUEST_GET_PARAMETER_BLOCK,
                                  PARAMETER_ID_CONTROLLER_EEPROM,
                                  offset,
                                  (struct parameter_report_s *) &parameter_report,
                                  sizeof (parameter_report))) != MICROTOUCH3M_STATUS_OK)
            return st;

        memcpy (&buffer[offset], parameter_report.data, PARAMETER_REPORT_FIRMWARE_DUMP_DATA_SIZE);
    }

    /* Success! */
    microtouch3m_log ("successfully read firmware from controller EEPROM");
    return MICROTOUCH3M_STATUS_OK;
}

/******************************************************************************/
/* Device data backup/restore */

#define CALIBRATION_DATA_BLOCK  1
#define CALIBRATION_DATA_SIZE  30
struct parameter_report_calibration_data_s {
    struct parameter_report_s header;
    uint8_t                   data [CALIBRATION_DATA_SIZE];
} __attribute__((packed));

#define LINEARIZATION_DATA_BLOCK  2
#define LINEARIZATION_DATA_SIZE  50
struct parameter_report_linearization_data_s {
    struct parameter_report_s header;
    uint8_t                   data [LINEARIZATION_DATA_SIZE];
} __attribute__((packed));

#define ORIENTATION_PARAMETER_NUMBER 1
#define ORIENTATION_DATA_SIZE        2
struct parameter_report_orientation_data_s {
    struct parameter_report_s header;
    uint8_t                   data [ORIENTATION_DATA_SIZE];
} __attribute__((packed));

#define IDENTIFIER_PARAMETER_NUMBER 2
#define IDENTIFIER_DATA_SIZE        4
struct parameter_report_identifier_data_s {
    struct parameter_report_s header;
    uint8_t                   data [IDENTIFIER_DATA_SIZE];
} __attribute__((packed));

struct microtouch3m_device_data_s {
    uint8_t calibration_data   [CALIBRATION_DATA_SIZE];
    uint8_t linearization_data [LINEARIZATION_DATA_SIZE];
    uint8_t orientation_data   [ORIENTATION_DATA_SIZE];
    uint8_t identifier_data    [IDENTIFIER_DATA_SIZE];
};

microtouch3m_status_t
microtouch3m_device_backup_data (microtouch3m_device_t       *dev,
                                 microtouch3m_device_data_t **out_data)
{
    microtouch3m_status_t       st;
    microtouch3m_device_data_t *data;

    assert (dev);
    assert (out_data);

    data = calloc (1, sizeof (struct microtouch3m_device_data_s));
    if (!data)
        return MICROTOUCH3M_STATUS_NO_MEMORY;

    microtouch3m_log ("backing up calibration data...");
    {
        struct parameter_report_calibration_data_s parameter_report;

        if ((st = run_in_request (dev,
                                  REQUEST_GET_PARAMETER_BLOCK,
                                  PARAMETER_ID_CONTROLLER_NOVRAM,
                                  (CALIBRATION_DATA_BLOCK << 8),
                                  (struct parameter_report_s *) &parameter_report,
                                  sizeof (parameter_report))) != MICROTOUCH3M_STATUS_OK)
            goto out;

        memcpy (data->calibration_data, parameter_report.data, CALIBRATION_DATA_SIZE);
        trace_buffer ("calibration data backed up", data->calibration_data, CALIBRATION_DATA_SIZE);
    }

    microtouch3m_log ("backing up linearization data...");
    {
        struct parameter_report_linearization_data_s parameter_report;

        if ((st = run_in_request (dev,
                                  REQUEST_GET_PARAMETER_BLOCK,
                                  PARAMETER_ID_CONTROLLER_NOVRAM,
                                  (LINEARIZATION_DATA_BLOCK << 8),
                                  (struct parameter_report_s *) &parameter_report,
                                  sizeof (parameter_report))) != MICROTOUCH3M_STATUS_OK)
            goto out;

        memcpy (data->linearization_data, parameter_report.data, LINEARIZATION_DATA_SIZE);
        trace_buffer ("linearization data backed up", data->linearization_data, LINEARIZATION_DATA_SIZE);
    }

    microtouch3m_log ("backing up orientation data...");
    {
        struct parameter_report_orientation_data_s parameter_report;

        if ((st = run_in_request (dev,
                                  REQUEST_GET_PARAMETER,
                                  ORIENTATION_PARAMETER_NUMBER,
                                  0x0000,
                                  (struct parameter_report_s *) &parameter_report,
                                  sizeof (parameter_report))) != MICROTOUCH3M_STATUS_OK)
            goto out;

        memcpy (data->orientation_data, parameter_report.data, ORIENTATION_DATA_SIZE);
        trace_buffer ("orientation data backed up", data->orientation_data, ORIENTATION_DATA_SIZE);
    }

    microtouch3m_log ("backing up identifier data...");
    {
        struct parameter_report_identifier_data_s parameter_report;

        if ((st = run_in_request (dev,
                                  REQUEST_GET_PARAMETER,
                                  IDENTIFIER_PARAMETER_NUMBER,
                                  0x0000,
                                  (struct parameter_report_s *) &parameter_report,
                                  sizeof (parameter_report))) != MICROTOUCH3M_STATUS_OK)
            goto out;

        memcpy (data->identifier_data, parameter_report.data, IDENTIFIER_DATA_SIZE);
        trace_buffer ("identifier data backed up", data->identifier_data, IDENTIFIER_DATA_SIZE);
    }

    /* Success! */
    microtouch3m_log ("successfully backed up controller data");
    st = MICROTOUCH3M_STATUS_OK;

out:
    if (st != MICROTOUCH3M_STATUS_OK)
        free (data);
    else
        *out_data = data;
    return st;
}

microtouch3m_status_t
microtouch3m_device_restore_data (microtouch3m_device_t            *dev,
                                  const microtouch3m_device_data_t *data)
{
    microtouch3m_status_t st;

    assert (dev);
    assert (data);

    microtouch3m_log ("restoring calibration data...");
    if ((st = run_out_request (dev,
                               REQUEST_SET_PARAMETER_BLOCK,
                               PARAMETER_ID_CONTROLLER_NOVRAM,
                               (CALIBRATION_DATA_BLOCK << 8),
                               data->calibration_data,
                               CALIBRATION_DATA_SIZE)) != MICROTOUCH3M_STATUS_OK)
        return st;

    microtouch3m_log ("restoring linearization data...");
    if ((st = run_out_request (dev,
                               REQUEST_SET_PARAMETER_BLOCK,
                               PARAMETER_ID_CONTROLLER_NOVRAM,
                               (LINEARIZATION_DATA_BLOCK << 8),
                               data->linearization_data,
                               LINEARIZATION_DATA_SIZE)) != MICROTOUCH3M_STATUS_OK)
        return st;

    microtouch3m_log ("restoring orientation data...");
    if ((st = run_out_request (dev,
                               REQUEST_SET_PARAMETER,
                               ORIENTATION_PARAMETER_NUMBER,
                               0x0000,
                               data->orientation_data,
                               ORIENTATION_DATA_SIZE)) != MICROTOUCH3M_STATUS_OK)
        return st;

    microtouch3m_log ("restoring identifier data...");
    if ((st = run_out_request (dev,
                               REQUEST_SET_PARAMETER,
                               IDENTIFIER_PARAMETER_NUMBER,
                               0x0000,
                               data->identifier_data,
                               IDENTIFIER_DATA_SIZE)) != MICROTOUCH3M_STATUS_OK)
        return st;

    /* Success! */
    microtouch3m_log ("successfully restored controller data");
    return MICROTOUCH3M_STATUS_OK;
}

/******************************************************************************/
/* Firmware update */

#define FIRMWARE_UPDATE_DATA_SIZE 64

microtouch3m_status_t
microtouch3m_device_firmware_update (microtouch3m_device_t *dev,
                                     const uint8_t         *buffer,
                                     size_t                 buffer_size)
{
    uint16_t     offset;
    unsigned int i;

    assert (dev);
    assert (buffer);

    if (buffer_size < MICROTOUCH3M_FW_IMAGE_SIZE) {
        microtouch3m_log ("error: not enough space in buffer to contain the full firmware image file (%zu < %zu)",
                          buffer_size, MICROTOUCH3M_FW_IMAGE_SIZE);
        return MICROTOUCH3M_STATUS_INVALID_ARGUMENTS;
    }

    if (!dev->usbhandle) {
        microtouch3m_log ("error: device not open");
        return MICROTOUCH3M_STATUS_INVALID_STATE;
    }

    microtouch3m_log ("updating firmware in controller EEPROM...");

    for (i = 0, offset = 0; offset < MICROTOUCH3M_FW_IMAGE_SIZE; offset += FIRMWARE_UPDATE_DATA_SIZE, i++) {
        microtouch3m_status_t st;

        if ((st = run_out_request (dev,
                                   REQUEST_SET_PARAMETER_BLOCK,
                                   PARAMETER_ID_CONTROLLER_EEPROM,
                                   offset,
                                   &buffer[offset],
                                   FIRMWARE_UPDATE_DATA_SIZE)) != MICROTOUCH3M_STATUS_OK)
            return st;
    }

    /* Success! */
    microtouch3m_log ("successfully written firmware to controller EEPROM");
    return MICROTOUCH3M_STATUS_OK;
}

/******************************************************************************/
/* Firmware files */

#define RECORD_DATA_SIZE        16
#define EXPECTED_N_DATA_RECORDS MICROTOUCH3M_FW_IMAGE_SIZE / RECORD_DATA_SIZE

static microtouch3m_status_t
ihex_error_to_status (int ihex_error)
{
    switch (ihex_error) {
    case IHEX_ERROR_FILE:
        return MICROTOUCH3M_STATUS_INVALID_IO;
    case IHEX_ERROR_INVALID_RECORD:
        return MICROTOUCH3M_STATUS_INVALID_DATA;
    case IHEX_ERROR_INVALID_ARGUMENTS:
        return MICROTOUCH3M_STATUS_INVALID_ARGUMENTS;
    case IHEX_ERROR_NEWLINE:
    case IHEX_ERROR_EOF:
    case IHEX_OK:
    default:
        assert (0);
    }
}

microtouch3m_status_t
microtouch3m_firmware_file_write (const char    *path,
                                  const uint8_t *buffer,
                                  size_t         buffer_size)
{
    microtouch3m_status_t  status = MICROTOUCH3M_STATUS_FAILED;
    FILE                  *f;
    uint16_t               offset;
    IHexRecord             record;
    int                    ihex_ret;
    static const uint8_t   segment_record_data[2] = { 0 };

    assert (path);
    assert (buffer);

    if (buffer_size < MICROTOUCH3M_FW_IMAGE_SIZE) {
        microtouch3m_log ("error: not enough space in buffer to contain the full firmware image file (%zu < %zu)", buffer_size, MICROTOUCH3M_FW_IMAGE_SIZE);
        status = MICROTOUCH3M_STATUS_INVALID_ARGUMENTS;
        goto out;
    }

    f = fopen (path, "w");
    if (!f) {
        microtouch3m_log ("error: opening firmware file failed: %s", strerror (errno));
        status = MICROTOUCH3M_STATUS_FAILED;
        goto out;
    }

    /* Write initial segment record */
    if (((ihex_ret = New_IHexRecord (IHEX_TYPE_02, 0x0000, segment_record_data, sizeof (segment_record_data), &record)) != IHEX_OK) ||
        ((ihex_ret = Write_IHexRecord (&record, f)) != IHEX_OK)) {
        status = ihex_error_to_status (ihex_ret);
        goto out;
    }

    /* Write all data records */
    for (offset = 0; offset < MICROTOUCH3M_FW_IMAGE_SIZE; offset += RECORD_DATA_SIZE) {
        if (((ihex_ret = New_IHexRecord (IHEX_TYPE_00, offset, &buffer[offset], RECORD_DATA_SIZE, &record)) != IHEX_OK) ||
            ((ihex_ret = Write_IHexRecord (&record, f)) != IHEX_OK)) {
            status = ihex_error_to_status (ihex_ret);
            goto out;
        }
    }

    /* Write final end-of-file record */
    if (((ihex_ret = New_IHexRecord (IHEX_TYPE_01, 0x0000, NULL, 0, &record)) != IHEX_OK) ||
        ((ihex_ret = Write_IHexRecord (&record, f)) != IHEX_OK)) {
        status = ihex_error_to_status (ihex_ret);
        goto out;
    }

    status = MICROTOUCH3M_STATUS_OK;

out:

    if (f)
        fclose (f);

    return status;
}

microtouch3m_status_t
microtouch3m_firmware_file_read (const char *path,
                                 uint8_t    *buffer,
                                 size_t      buffer_size)
{
    microtouch3m_status_t  status = MICROTOUCH3M_STATUS_FAILED;
    FILE                  *f;
    bool                   exii_first_found = false;
    bool                   exii_last_found = false;
    unsigned int           n_data_records = 0;
    size_t                 bytes_read = 0;

    assert (path);

    /* Note: if buffer not given, we just validate file */

    if (buffer && buffer_size < MICROTOUCH3M_FW_IMAGE_SIZE) {
        microtouch3m_log ("error: not enough space in buffer to store the full firmware image file (%zu < %zu)", buffer_size, MICROTOUCH3M_FW_IMAGE_SIZE);
        status = MICROTOUCH3M_STATUS_INVALID_ARGUMENTS;
        goto out;
    }

    f = fopen (path, "r");
    if (!f) {
        microtouch3m_log ("error: opening firmware file failed: %s", strerror (errno));
        status = MICROTOUCH3M_STATUS_FAILED;
        goto out;
    }

    while (1) {
        IHexRecord record;
        int        ihex_ret;

        if ((ihex_ret = Read_IHexRecord (&record, f)) != IHEX_OK) {
            /* No more records to read? */
            if (ihex_ret == IHEX_ERROR_EOF) {
                /* Did we not receive the last record? */
                if (!exii_last_found) {
                    microtouch3m_log ("error: last record missing");
                    status = MICROTOUCH3M_STATUS_INVALID_FORMAT;
                    goto out;
                }
                break;
            }
            /* Empty line with no record? ignore */
            if (ihex_ret == IHEX_ERROR_NEWLINE)
                continue;
            /* Other fatal error */
            status = ihex_error_to_status (ihex_ret);
            goto out;
        }

        /* A valid record was read */

        /* Are we expecting the first record in the file? */
        if (!exii_first_found) {
            /* It must be a extended segment address record */
            if (record.type != IHEX_TYPE_02) {
                microtouch3m_log ("error: unexpected record type found (0x%x) when expecting the first record (0x%x)", record.type, IHEX_TYPE_02);
                status = MICROTOUCH3M_STATUS_INVALID_FORMAT;
                goto out;
            }

            /* The first record should report record address 0 */
            if (record.address != 0) {
                microtouch3m_log ("error: unexpected record address (0x%04x) when expecting the first record", record.address);
                status = MICROTOUCH3M_STATUS_INVALID_FORMAT;
                goto out;
            }

            exii_first_found = true;
            continue;
        }

        /* Last record reported */
        if (record.type == IHEX_TYPE_01) {
            exii_last_found = true;
            continue;
        }

        /* No data records should happen after the last record reported */
        if (exii_last_found) {
            microtouch3m_log ("error: additional record found after the last one");
            status = MICROTOUCH3M_STATUS_INVALID_FORMAT;
            goto out;
        }

        /* Make sure we don't read more bytes than the expected ones */
        if (record.dataLen + bytes_read > MICROTOUCH3M_FW_IMAGE_SIZE) {
            microtouch3m_log ("error: too many bytes read (%zu > %zu)", (record.dataLen + bytes_read), MICROTOUCH3M_FW_IMAGE_SIZE);
            status = MICROTOUCH3M_STATUS_INVALID_FORMAT;
            goto out;
        }

        /* Records in a EXII firmware file have 16 bytes max */
        if (record.dataLen != 16) {
            microtouch3m_log ("error: unexpected number of bytes in record (%zu != 16)", record.dataLen);
            status = MICROTOUCH3M_STATUS_INVALID_FORMAT;
            goto out;
        }

        /* Copy bytes to buffer, if buffer given */
        if (buffer)
            memcpy (&buffer[bytes_read], record.data, record.dataLen);

        /* Update total number of bytes read */
        bytes_read += record.dataLen;

        n_data_records++;
    }

    /* Firmware files are fixed size, so if the number of bytes per record is
     * also fixed, the number of data records themselves must also be fixed */
    if (n_data_records != EXPECTED_N_DATA_RECORDS) {
        microtouch3m_log ("error: unexpected number of data records (%zu != %zu)", n_data_records, EXPECTED_N_DATA_RECORDS);
        status = MICROTOUCH3M_STATUS_INVALID_FORMAT;
        goto out;
    }

    /* This assertion must always be valid, because we are validating separately
     * the amount of bytes per data record and the amount of data records. */
    assert (bytes_read == MICROTOUCH3M_FW_IMAGE_SIZE);
    status = MICROTOUCH3M_STATUS_OK;

#if 0
    if (buffer) {
        char *hex;

        hex = strhex_multiline (buffer, bytes_read, 32, "\t", ":");
        printf ("HEX:\n"
                "\t%s\n", hex);
        free (hex);
    }
#endif

out:

    if (f)
        fclose (f);

    return status;
}
