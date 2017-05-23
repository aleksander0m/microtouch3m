/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * libmicrotouch3m - MicroTouch 3M touchscreen control library
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2017 Zodiac Inflight Innovations
 * Copyright (C) 2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>

#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <endian.h>
#include <unistd.h>

#include <libusb.h>

#include "common.h"

#include "microtouch3m.h"
#include "microtouch3m-log.h"

#include "ihex.h"

#undef  GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

/* in GCC version >= 4.1.0 we can use the __sync built-in atomic operations */
#if !defined(GCC_VERSION) || GCC_VERSION < 40100
# error GCC >= 4.1.0 required for built-in atomic method support
#endif

/******************************************************************************/
/* Common */

#define MICROTOUCH3M_VID 0x0596
#define MICROTOUCH3M_PID 0x0001

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
    /* FW operation progress callback */
    microtouch3m_device_firmware_progress_f *progress_callback;
    float                                    progress_freq;
    void                                    *progress_user_data;
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

#define MAX_PORT_NUMBERS 7

static libusb_device **
find_usb_device (microtouch3m_context_t *ctx,
                 bool                    any,
                 bool                    first,
                 uint8_t                 bus_number,
                 uint8_t                 device_address,
                 const uint8_t          *port_numbers,
                 int                     port_numbers_len,
                 unsigned int           *out_n_devices)
{
    libusb_device **list  = NULL;
    libusb_device **found = NULL;
    unsigned int    n_found = 0;
    unsigned int    i;
    ssize_t         ret;
    char           *port_numbers_str = NULL;

    /* At least one search method requested */
    assert ((device_address) || (port_numbers_len) || (first) || (any));
    /* At maximum one search method requested */
    assert ((!!device_address + !!port_numbers_len + first + any) == 1);
    /* bus number requested unless first or any */
    assert (bus_number || first || any);

    if ((ret = libusb_get_device_list (ctx->usb, &list)) < 0) {
        microtouch3m_log ("error: couldn't list USB devices: %s", libusb_strerror (ret));
        return NULL;
    }

    if (port_numbers_len) {
        port_numbers_str = str_usb_location (bus_number, port_numbers, port_numbers_len);
        if (!port_numbers_str)
            return NULL;
    }

    assert (list);
    for (i = 0; list[i]; i++) {
        libusb_device                    *iter;
        struct libusb_device_descriptor   desc;
        libusb_device                   **aux;

        iter = list[i];

        if (libusb_get_device_descriptor (iter, &desc) != 0)
            continue;

        if (desc.idVendor != MICROTOUCH3M_VID)
            continue;

        if (desc.idProduct != MICROTOUCH3M_PID)
            continue;

        microtouch3m_log ("Microtouch 3M device found at %03u:%03u",
                          libusb_get_bus_number (iter), libusb_get_device_address (iter));

        if (bus_number) {
            uint8_t iter_bus_number;

            iter_bus_number = libusb_get_bus_number (iter);
            if (bus_number != iter_bus_number && bus_number != 0) {
                microtouch3m_log ("  skipped because bus number (%03u) is not %03u", iter_bus_number, bus_number);
                continue;
            }
        }

        if (device_address) {
            uint8_t iter_device_address;

            iter_device_address = libusb_get_device_address (iter);
            if (device_address != iter_device_address && device_address != 0) {
                microtouch3m_log ("  skipped because device address (%03u) is not %03u", iter_device_address, device_address);
                continue;
            }
        }

        if (port_numbers_len) {
            uint8_t  iter_port_numbers[MAX_PORT_NUMBERS];
            int      iter_port_numbers_len;
            char    *iter_port_numbers_str = NULL;
            bool     matched = false;

            assert (port_numbers_str);

            iter_port_numbers_len = libusb_get_port_numbers (iter, (uint8_t *) iter_port_numbers, MAX_PORT_NUMBERS);
            iter_port_numbers_str = str_usb_location (bus_number, iter_port_numbers, iter_port_numbers_len);
            if (!iter_port_numbers_str)
                goto out_port_numbers;

            if (strcmp (port_numbers_str, iter_port_numbers_str) != 0) {
                microtouch3m_log ("  skipped because location (%s) is not '%s'", iter_port_numbers_str, port_numbers_str);
                goto out_port_numbers;
            }

            matched = true;

        out_port_numbers:
            free (iter_port_numbers_str);

            if (!matched)
                continue;
        }

        aux = realloc (found, (n_found + 1) * sizeof (libusb_device *));
        if (!aux) {
            microtouch3m_log ("error reallocating array of devices");
            continue;
        }
        found = aux;
        found[n_found++] = libusb_ref_device (iter);

        if (!any)
            break;
    }

    free (port_numbers_str);
    libusb_free_device_list (list, 1);

    if (!found)
        microtouch3m_log ("error: couldn't find MicroTouch 3M device");

    *out_n_devices = n_found;

    return found;
}

/* Note: don't use libusb_free_device_list () as WE created this array */
static void
usb_device_array_free (libusb_device **array,
                       unsigned int    n_items)
{
    unsigned int i;

    if (!array || !n_items)
        return;

    for (i = 0; i < n_items; i++) {
        if (array[i])
            libusb_unref_device (array[i]);
    }
    free (array);
}

static libusb_device *
find_one_usb_device (microtouch3m_context_t *ctx,
                     bool                    first,
                     uint8_t                 bus_number,
                     uint8_t                 device_address,
                     const uint8_t          *port_numbers,
                     int                     port_numbers_len)
{
    libusb_device  *usbdev = NULL;
    libusb_device **usbdevs;
    unsigned int    n_devices;

    usbdevs = find_usb_device (ctx, false, first, bus_number, device_address, port_numbers, port_numbers_len, &n_devices);
    if (!usbdevs || !n_devices)
        return NULL;

    assert (n_devices == 1);

    usbdev = libusb_ref_device (usbdevs[0]);

    usb_device_array_free (usbdevs, n_devices);

    return usbdev;
}

void
microtouch3m_device_array_free (microtouch3m_device_t **array,
                                unsigned int            n_items)
{
    unsigned int i;

    if (!array || !n_items)
        return;

    for (i = 0; i < n_items; i++) {
        if (array[i])
            microtouch3m_device_unref (array[i]);
    }
    free (array);
}

microtouch3m_device_t **
microtouch3m_device_array_new (microtouch3m_context_t *ctx,
                               unsigned int           *out_n_items)
{
    microtouch3m_device_t **devices = NULL;
    libusb_device         **usbdevs = NULL;
    unsigned int            n_devices = 0;
    unsigned int            i;

    assert (out_n_items);

    usbdevs = find_usb_device (ctx, true, false, 0, 0, NULL, 0, &n_devices);
    if (!usbdevs || !n_devices)
        goto out_err;

    devices = calloc (n_devices, sizeof (microtouch3m_device_t *));
    if (!devices)
        goto out_err;

    for (i = 0; i < n_devices; i++) {
        /* On device creation failure, usbdev is consumed as well */
        devices[i] = device_new_by_usbdev (ctx, libusb_ref_device (usbdevs[i]));
        /* Any error creating a device makes the whole process fail */
        if (!devices[i])
            goto out_err;
    }
    usb_device_array_free (usbdevs, n_devices);

    *out_n_items = n_devices;
    return devices;

out_err:
    microtouch3m_device_array_free (devices, n_devices);
    usb_device_array_free (usbdevs, n_devices);
    *out_n_items = 0;
    return NULL;
}

microtouch3m_device_t *
microtouch3m_device_new_first (microtouch3m_context_t *ctx)
{
    libusb_device *usbdev;

    usbdev = find_one_usb_device (ctx, true, 0, 0, NULL, 0);
    if (!usbdev)
        return NULL;

    /* On device creation failure, usbdev is consumed as well */
    return device_new_by_usbdev (ctx, usbdev);
}

microtouch3m_device_t *
microtouch3m_device_new_by_usb_address (microtouch3m_context_t *ctx,
                                        uint8_t                 bus_number,
                                        uint8_t                 device_address)
{
    libusb_device *usbdev;

    usbdev = find_one_usb_device (ctx, false, bus_number, device_address, NULL, 0);
    if (!usbdev)
        return NULL;

    /* On device creation failure, usbdev is consumed as well */
    return device_new_by_usbdev (ctx, usbdev);
}

microtouch3m_device_t *
microtouch3m_device_new_by_usb_location (microtouch3m_context_t *ctx,
                                         uint8_t                 bus_number,
                                         const uint8_t          *port_numbers,
                                         int                     port_numbers_len)
{
    libusb_device *usbdev;

    usbdev = find_one_usb_device (ctx, false, bus_number, 0, port_numbers, port_numbers_len);
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

    if (dev->usbhandle)
        libusb_close (dev->usbhandle);

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
    int ret;

    assert (dev);

    if (!dev->usbhandle && ((ret = libusb_open (dev->usbdev, &dev->usbhandle)) < 0)) {
        microtouch3m_log ("error: couldn't open usb device: %s", libusb_strerror (ret));
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
    REQUEST_ASYNC_SET_REPORT    = 0x01,
    REQUEST_GET_PARAMETER_BLOCK = 0x02,
    REQUEST_SET_PARAMETER_BLOCK = 0x03,
    REQUEST_STATUS              = 0x06,
    REQUEST_RESET               = 0x07,
    REQUEST_CONTROLLER_ID       = 0x0A,
    REQUEST_GET_PARAMETER       = 0x10,
    REQUEST_SET_PARAMETER       = 0x11,
    REQUEST_GET_GENERIC         = 0x12,
    REQUEST_SET_GENERIC         = 0x13,
};

enum parameter_id_e {
    PARAMETER_ID_CONTROLLER_NOVRAM      = 0x0000,
    PARAMETER_ID_CONTROLLER_STRAYS      = 0x0003,
    PARAMETER_ID_CONTROLLER_SENSITIVITY = 0x0017,
    PARAMETER_ID_CONTROLLER_EEPROM      = 0x0020,
};

enum report_id_e {
    REPORT_ID_COORDINATE_DATA = 0x0001,
    REPORT_ID_SCOPE_DATA      = 0x0002,
    REPORT_ID_PARAMETER       = 0x0004,
};

enum async_set_report_e {
    ASYNC_SET_REPORT_DISABLE = 0x0000,
    ASYNC_SET_REPORT_ENABLE  = 0x0001,
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
                uint8_t                   *parameter_data,
                size_t                     parameter_data_size,
                enum libusb_error         *out_usb_error)
{
    int desc_size;

    assert (dev);
    assert (parameter_data);

    if ((desc_size = libusb_control_transfer (dev->usbhandle,
                                              LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                              parameter_cmd,
                                              parameter_value,
                                              parameter_index,
                                              parameter_data,
                                              parameter_data_size,
                                              5000)) < 0) {
        if (out_usb_error)
            *out_usb_error = (enum libusb_error) desc_size;
        microtouch3m_log ("warn: while running IN request 0x%02x value 0x%04x index 0x%04x: %s",
                          parameter_cmd, parameter_value, parameter_index, libusb_strerror (desc_size));
        return MICROTOUCH3M_STATUS_INVALID_IO;
    }

    if (desc_size != parameter_data_size) {
        microtouch3m_log ("error: couldn't run IN request 0x%02x value 0x%04x index 0x%04x: invalid data size read (%d != %d)",
                          parameter_cmd, parameter_value, parameter_index, desc_size, parameter_data_size);
        return MICROTOUCH3M_STATUS_INVALID_DATA;
    }

    microtouch3m_log ("successfully run IN request 0x%02x value 0x%04x index 0x%04x",
                      parameter_cmd, parameter_value, parameter_index);
    return MICROTOUCH3M_STATUS_OK;
}

static microtouch3m_status_t
run_parameter_in_request (microtouch3m_device_t     *dev,
                          enum request_e             parameter_cmd,
                          uint16_t                   parameter_value,
                          uint16_t                   parameter_index,
                          struct parameter_report_s *parameter_report,
                          size_t                     parameter_report_size,
                          enum libusb_error         *out_usb_error)
{
    microtouch3m_status_t st;

    if ((st = run_in_request (dev,
                              parameter_cmd,
                              parameter_value,
                              parameter_index,
                              (uint8_t *) parameter_report,
                              parameter_report_size,
                              out_usb_error)) != MICROTOUCH3M_STATUS_OK)
        return st;

    if (parameter_report->report_id != REPORT_ID_PARAMETER) {
        microtouch3m_log ("error: couldn't run parameter IN request 0x%02x value 0x%04x index 0x%04x: invalid report id (%d != %d)",
                          parameter_cmd, parameter_value, parameter_index, parameter_report->report_id, REPORT_ID_PARAMETER);
        return MICROTOUCH3M_STATUS_INVALID_DATA;
    }

    if (le16toh (parameter_report->data_size) != (parameter_report_size - sizeof (struct parameter_report_s))) {
        microtouch3m_log ("error: couldn't run parameter IN request 0x%02x value 0x%04x index 0x%04x: invalid read data size reported (%d != %d)",
                          parameter_cmd, parameter_value, parameter_index, le16toh (parameter_report->data_size), (parameter_report_size - sizeof (struct parameter_report_s)));
        return MICROTOUCH3M_STATUS_INVALID_FORMAT;
    }

    return MICROTOUCH3M_STATUS_OK;
}

static microtouch3m_status_t
run_out_request (microtouch3m_device_t *dev,
                 enum request_e         parameter_cmd,
                 uint16_t               parameter_value,
                 uint16_t               parameter_index,
                 const uint8_t         *parameter_data,
                 size_t                 parameter_data_size,
                 enum libusb_error     *out_usb_error)
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
        if (out_usb_error)
            *out_usb_error = (enum libusb_error) desc_size;
        microtouch3m_log ("warn: while running OUT request 0x%02x value 0x%04x index 0x%04x data %u bytes: %s",
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
/* Status */

typedef enum {
    CMD_STATUS_FAILURE            = 0,
    CMD_STATUS_ONGOING            = 1,
    CMD_STATUS_STAGE_1_COMPLETED  = 2,
    CMD_STATUS_COMPLETED          = 3,
    CMD_STATUS_SOFT_RESET_OCCURED = 4,
    CMD_STATUS_HARD_RESET_OCCURED = 5,
} cmd_status_t;

struct standard_status_report_s {
    uint8_t  report_id;
    uint16_t poc_status;
    uint8_t  cmd_status;
    uint8_t  touch_status;
    uint16_t async_reports;
    uint8_t  reserved;
} __attribute__((packed));

struct extended_status_report_s {
    struct standard_status_report_s standard;
    uint8_t  reserved;
    uint16_t extended_poc_status;
    uint8_t  reserved2[9];
} __attribute__((packed));

static microtouch3m_status_t
device_get_status_standard (microtouch3m_device_t           *dev,
                            struct standard_status_report_s *out_status)
{
    microtouch3m_status_t st;

    microtouch3m_log ("reading standard status...");
    if ((st = run_in_request (dev,
                              REQUEST_STATUS,
                              0x0000,
                              0x0000,
                              (uint8_t *) out_status,
                              sizeof (struct standard_status_report_s),
                              NULL)) != MICROTOUCH3M_STATUS_OK) {
        microtouch3m_log ("error: couldn't read standard status");
    }
    return st;
}

static microtouch3m_status_t
device_get_status_extended (microtouch3m_device_t           *dev,
                            struct extended_status_report_s *out_status)
{
    microtouch3m_status_t st;

    microtouch3m_log ("reading extended status...");
    if ((st = run_in_request (dev,
                              REQUEST_STATUS,
                              0x0000,
                              0x0000,
                              (uint8_t *) out_status,
                              sizeof (struct extended_status_report_s),
                              NULL)) != MICROTOUCH3M_STATUS_OK) {
        microtouch3m_log ("error: couldn't read extended status");
    }
    return st;
}

static microtouch3m_status_t
device_wait_cmd_status (microtouch3m_device_t *dev,
                        cmd_status_t           cmd_status,
                        unsigned int           retry_ms,
                        unsigned int           max_retries)
{
    unsigned int          n_retries;
    microtouch3m_status_t st;

    for (n_retries = 0; n_retries < max_retries; n_retries++) {
        struct standard_status_report_s status;

        if ((st = device_get_status_standard (dev, &status)) != MICROTOUCH3M_STATUS_OK)
            return st;

        if (status.cmd_status == cmd_status)
            return MICROTOUCH3M_STATUS_OK;

        usleep (retry_ms * 1000);
    }

    return MICROTOUCH3M_STATUS_FAILED;
}

/******************************************************************************/
/* Query controller ID */

struct report_controller_id_s {
    uint8_t  report_id;
    uint16_t controller_type;
    uint8_t  firmware_major;
    uint8_t  firmware_minor;
    uint8_t  features;
    uint16_t constants_checksum;
    uint16_t max_param_write;
    uint8_t  reserved[8];
    uint32_t pc_checksum;
    uint16_t asic_type;
} __attribute__((packed));

microtouch3m_status_t
microtouch3m_device_query_controller_id (microtouch3m_device_t *dev,
                                         uint16_t              *controller_type,
                                         uint8_t               *firmware_major,
                                         uint8_t               *firmware_minor,
                                         uint8_t               *features,
                                         uint16_t              *constants_checksum,
                                         uint16_t              *max_param_write,
                                         uint32_t              *pc_checksum,
                                         uint16_t              *asic_type)
{
    microtouch3m_status_t         st;
    struct report_controller_id_s report_controller_id = { 0 };

    microtouch3m_log ("querying controller id");
    if ((st = run_in_request (dev,
                              REQUEST_CONTROLLER_ID,
                              0x0000,
                              0x0000,
                              (uint8_t *) &report_controller_id,
                              sizeof (report_controller_id),
                              NULL)) != MICROTOUCH3M_STATUS_OK)
        return st;

    if (controller_type)
        *controller_type = le16toh (report_controller_id.controller_type);
    if (firmware_major)
        *firmware_major = report_controller_id.firmware_major;
    if (firmware_minor)
        *firmware_minor = report_controller_id.firmware_minor;
    if (features)
        *features = report_controller_id.features;
    if (constants_checksum)
        *constants_checksum = le16toh (report_controller_id.constants_checksum);
    if (max_param_write)
        *max_param_write = le16toh (report_controller_id.max_param_write);
    if (pc_checksum)
        *pc_checksum = le32toh (report_controller_id.pc_checksum);
    if (asic_type)
        *asic_type = le16toh (report_controller_id.asic_type);

    /* Success! */
    microtouch3m_log ("successfully queried controller id");
    return MICROTOUCH3M_STATUS_OK;
}

/******************************************************************************/
/* Reset */

#define RESET_STATUS_CHECKS_MAX        20
#define RESET_STATUS_CHECKS_TIMEOUT_MS 100

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
    enum libusb_error     usb_error;
    cmd_status_t          expected_cmd_status;

    microtouch3m_log ("requesting controller reset: %s", microtouch3m_device_reset_to_string (reset));
    if ((st = run_out_request (dev,
                               REQUEST_RESET,
                               reset,
                               0x0000,
                               NULL,
                               0,
                               &usb_error)) != MICROTOUCH3M_STATUS_OK) {
        /* EIO on a reboot request is what we expect */
        if ((reset == MICROTOUCH3M_DEVICE_RESET_REBOOT) &&
            (st == MICROTOUCH3M_STATUS_INVALID_IO) &&
            (usb_error == LIBUSB_ERROR_PIPE)) {
            microtouch3m_log ("successfully requested controller 'reboot' reset");
            return MICROTOUCH3M_STATUS_OK;
        }

        /* All other errors are fatal */
        return st;
    }

    /* If for some reason we get here during a 'reboot' reset, return an error.
     * We do want to get a EPIPE error */
    switch (reset) {
    case MICROTOUCH3M_DEVICE_RESET_REBOOT:
        microtouch3m_log ("error: reboot reset request ignored");
        return MICROTOUCH3M_STATUS_FAILED;
    case MICROTOUCH3M_DEVICE_RESET_SOFT:
        expected_cmd_status = CMD_STATUS_SOFT_RESET_OCCURED;
        break;
    case MICROTOUCH3M_DEVICE_RESET_HARD:
        expected_cmd_status = CMD_STATUS_HARD_RESET_OCCURED;
        break;
    default:
        microtouch3m_log ("error: invalid reset type requested");
        return MICROTOUCH3M_STATUS_INVALID_ARGUMENTS;
    }

    if ((st = device_wait_cmd_status (dev,
                                      expected_cmd_status,
                                      RESET_STATUS_CHECKS_TIMEOUT_MS,
                                      RESET_STATUS_CHECKS_MAX)) != MICROTOUCH3M_STATUS_OK)
        return st;

    /* Success! */
    microtouch3m_log ("successfully requested controller reset");
    return MICROTOUCH3M_STATUS_OK;
}

/******************************************************************************/
/* Sensitivity levels */

#define VALUE_SENSITIVITY 0x005a

/* These constants are gathered after analyzing the USB traffic generated by the
 * MT7SDU program in Windows. These are the 7 levels going from min to max that
 * the app allows to set.
 *
 * Worth noting that this values are given in BIG ENDIAN in the protocol.
 */
static const uint16_t level_ids[MICROTOUCH3M_DEVICE_SENSITIVITY_LEVEL_MAX + 1] = {
    0x01a4,
    0x0165,
    0x0126,
    0x00d2,
    0x0093,
    0x0069,
    0x003f
};

struct parameter_report_sensitivity_s {
    struct parameter_report_s header;
    uint16_t level_be;
    uint16_t no_idea_just_zero_it;
} __attribute__((packed));

microtouch3m_status_t
microtouch3m_device_get_sensitivity_level (microtouch3m_device_t *dev,
                                           uint8_t               *level)
{
    microtouch3m_status_t                 st;
    struct parameter_report_sensitivity_s parameter_report;
    int                                   i;
    uint16_t                              level_id;

    microtouch3m_log ("reading sensitivity");
    if ((st = run_parameter_in_request (dev,
                                        REQUEST_GET_PARAMETER_BLOCK,
                                        PARAMETER_ID_CONTROLLER_SENSITIVITY,
                                        VALUE_SENSITIVITY,
                                        (struct parameter_report_s *) &parameter_report,
                                        sizeof (parameter_report),
                                        NULL)) != MICROTOUCH3M_STATUS_OK)
        return st;

    level_id = be16toh (parameter_report.level_be);
    for (i = MICROTOUCH3M_DEVICE_SENSITIVITY_LEVEL_MIN; i <= MICROTOUCH3M_DEVICE_SENSITIVITY_LEVEL_MAX; i++) {
        if (level_id == level_ids[i]) {
            if (level)
                *level = i;
            return MICROTOUCH3M_STATUS_OK;
        }
    }

    microtouch3m_log ("invalid sensitivity level id (%hu)", level_id);
    return MICROTOUCH3M_STATUS_INVALID_DATA;
}

struct sensitivity_s {
    uint16_t level_be;
    uint16_t no_idea_just_zero_it;
} __attribute__((packed));

microtouch3m_status_t
microtouch3m_device_set_sensitivity_level (microtouch3m_device_t *dev,
                                           uint8_t                level)
{
    microtouch3m_status_t st;
    struct sensitivity_s  sensitivity = { 0 };

    if (level > MICROTOUCH3M_DEVICE_SENSITIVITY_LEVEL_MAX) {
        microtouch3m_log ("invalid sensitivity level (%u > %u)", level, MICROTOUCH3M_DEVICE_SENSITIVITY_LEVEL_MAX);
        return MICROTOUCH3M_STATUS_INVALID_ARGUMENTS;
    }

    sensitivity.level_be = htobe16 (level_ids[level]);

    microtouch3m_log ("setting sensitivity level...");
    if ((st = run_out_request (dev,
                               REQUEST_SET_PARAMETER_BLOCK,
                               PARAMETER_ID_CONTROLLER_SENSITIVITY,
                               VALUE_SENSITIVITY,
                               (const uint8_t *) &sensitivity,
                               sizeof (sensitivity),
                               NULL)) != MICROTOUCH3M_STATUS_OK)
        return st;

    microtouch3m_log ("successfully set sensitivity level...");
    return MICROTOUCH3M_STATUS_OK;
}

/******************************************************************************/
/* Frequency */

#define VALUE_FREQUENCY 0x0002

/* The frequency constants are gathered after analyzing the USB traffic
 * generated by the MT7SDU program in Windows. These are the 5 different
 * frequency settings, although the application also allowed 2 more (higher)
 * that were not usable with the controller being tested.
 */

struct freq_str_s {
    microtouch3m_device_frequency_t  freq;
    const char                      *str;
};

static const struct freq_str_s freq_str[] = {
    { MICROTOUCH3M_DEVICE_FREQUENCY_109096, "109.096Hz" },
    { MICROTOUCH3M_DEVICE_FREQUENCY_95703,   "95.703Hz" },
    { MICROTOUCH3M_DEVICE_FREQUENCY_85286,   "85.286Hz" },
    { MICROTOUCH3M_DEVICE_FREQUENCY_76953,   "76.953Hz" },
    { MICROTOUCH3M_DEVICE_FREQUENCY_70135,   "70.135Hz" },
};

const char *
microtouch3m_device_frequency_to_string (microtouch3m_device_frequency_t freq)
{
    unsigned int i;

    for (i = 0; i < (sizeof (freq_str) / sizeof (freq_str[0])); i++) {
        if (freq == freq_str[i].freq)
            return freq_str[i].str;
    }
    return NULL;
}

struct get_generic_s {
    uint8_t something;
    uint16_t value;
} __attribute__((packed));

microtouch3m_status_t
microtouch3m_device_get_frequency (microtouch3m_device_t           *dev,
                                   microtouch3m_device_frequency_t *freq)
{
    microtouch3m_status_t st;
    struct get_generic_s  aux;

    microtouch3m_log ("reading current frequency");
    if ((st = run_in_request (dev,
                              REQUEST_GET_GENERIC,
                              0x0000,
                              VALUE_FREQUENCY,
                              (uint8_t *)&aux,
                              sizeof (aux),
                              NULL)) != MICROTOUCH3M_STATUS_OK)
        return st;

    /* Convert from LE to HE */
    aux.value = le16toh (aux.value);

    /* Validate the setting by looking for a string representation */
    if (!microtouch3m_device_frequency_to_string (aux.value)) {
        microtouch3m_log ("error: unknown frequency setting reported: 0x%04x", aux.value);
        return MICROTOUCH3M_STATUS_INVALID_DATA;
    }

    if (freq)
        *freq = (microtouch3m_device_frequency_t) aux.value;

    return MICROTOUCH3M_STATUS_OK;
}

struct set_generic_s {
    uint16_t value;
} __attribute__((packed));

microtouch3m_status_t
microtouch3m_device_set_frequency (microtouch3m_device_t           *dev,
                                   microtouch3m_device_frequency_t  freq)
{
    microtouch3m_status_t  st;
    struct set_generic_s   aux = { 0 };
    const char            *str;

    /* Validate the setting by looking for a string representation */
    if (!(str = microtouch3m_device_frequency_to_string (freq))) {
        microtouch3m_log ("error: unknown frequency setting requested: 0x%04x", freq);
        return MICROTOUCH3M_STATUS_INVALID_ARGUMENTS;
    }

    aux.value = htole16 ((uint16_t) freq);

    microtouch3m_log ("setting frequency to %s...", str);
    if ((st = run_out_request (dev,
                               REQUEST_SET_GENERIC,
                               0x0000,
                               VALUE_FREQUENCY,
                               (const uint8_t *) &aux,
                               sizeof (aux),
                               NULL)) != MICROTOUCH3M_STATUS_OK)
        return st;

    microtouch3m_log ("successfully set frequency to %s", str);
    return MICROTOUCH3M_STATUS_OK;
}

/******************************************************************************/
/* Read strays */

struct parameter_report_read_strays_s {
    struct parameter_report_s header;
    uint32_t ul_stray_i;
    uint32_t ul_stray_q;
    uint32_t ur_stray_i;
    uint32_t ur_stray_q;
    uint32_t ll_stray_i;
    uint32_t ll_stray_q;
    uint32_t lr_stray_i;
    uint32_t lr_stray_q;
} __attribute__((packed));

microtouch3m_status_t
microtouch3m_read_strays (microtouch3m_device_t *dev,
                          int32_t               *ul_stray_i,
                          int32_t               *ul_stray_q,
                          int32_t               *ur_stray_i,
                          int32_t               *ur_stray_q,
                          int32_t               *ll_stray_i,
                          int32_t               *ll_stray_q,
                          int32_t               *lr_stray_i,
                          int32_t               *lr_stray_q)
{
    microtouch3m_status_t                 st;
    struct parameter_report_read_strays_s parameter_report;

    microtouch3m_log ("reading strays");
    if ((st = run_parameter_in_request (dev,
                                        REQUEST_GET_PARAMETER_BLOCK,
                                        PARAMETER_ID_CONTROLLER_STRAYS,
                                        0x0000,
                                        (struct parameter_report_s *) &parameter_report,
                                        sizeof (parameter_report),
                                        NULL)) != MICROTOUCH3M_STATUS_OK)
        return st;

    if (ul_stray_i)
        *ul_stray_i = (int32_t) (le32toh (parameter_report.ul_stray_i));
    if (ul_stray_q)
        *ul_stray_q = (int32_t) (le32toh (parameter_report.ul_stray_q));

    if (ur_stray_i)
        *ur_stray_i = (int32_t) (le32toh (parameter_report.ur_stray_i));
    if (ur_stray_q)
        *ur_stray_q = (int32_t) (le32toh (parameter_report.ur_stray_q));

    if (ll_stray_i)
        *ll_stray_i = (int32_t) (le32toh (parameter_report.ll_stray_i));
    if (ll_stray_q)
        *ll_stray_q = (int32_t) (le32toh (parameter_report.ll_stray_q));

    if (lr_stray_i)
        *lr_stray_i = (int32_t) (le32toh (parameter_report.lr_stray_i));
    if (lr_stray_q)
        *lr_stray_q = (int32_t) (le32toh (parameter_report.lr_stray_q));

    /* Success! */
    microtouch3m_log ("successfully read strays");
    return MICROTOUCH3M_STATUS_OK;
}

/******************************************************************************/
/* Linearization data */

/* In the USB messages we want it to be represented as an array of 16bit values,
 * where each value is composed of both X and Y components. We want this because
 * we want to explicitly do LE->HE conversion. */
struct internal_linearization_data_s {
    uint16_t items[5][5];
} __attribute__((packed));

#define LINEARIZATION_DATA_BLOCK  2
struct parameter_report_linearization_data_s {
    struct parameter_report_s            header;
    struct internal_linearization_data_s data;
} __attribute__((packed));

microtouch3m_status_t
microtouch3m_device_get_linearization_data (microtouch3m_device_t                           *dev,
                                            struct microtouch3m_device_linearization_data_s *data)
{
    struct parameter_report_linearization_data_s parameter_report;
    microtouch3m_status_t                        st;
    int                                          i, j;

    if ((st = run_parameter_in_request (dev,
                                        REQUEST_GET_PARAMETER_BLOCK,
                                        PARAMETER_ID_CONTROLLER_NOVRAM,
                                        (LINEARIZATION_DATA_BLOCK << 8),
                                        (struct parameter_report_s *) &parameter_report,
                                        sizeof (parameter_report),
                                        NULL)) != MICROTOUCH3M_STATUS_OK)
        return st;

    microtouch3m_log_buffer ("linearization data retrieved", (const uint8_t *)&(parameter_report.data), sizeof (struct internal_linearization_data_s));

    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            uint16_t val;

            val = le16toh (parameter_report.data.items[i][j]);
            data->items[i][j].x_coef = val >> 8;
            data->items[i][j].y_coef = val & 0xff;
        }
    }

    return MICROTOUCH3M_STATUS_OK;
}

microtouch3m_status_t
microtouch3m_device_set_linearization_data (microtouch3m_device_t                                 *dev,
                                            const struct microtouch3m_device_linearization_data_s *data)
{
    struct internal_linearization_data_s internal_data;
    int                                  i, j;

    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            uint16_t val;

            val = (((uint8_t) data->items[i][j].x_coef) << 8) | ((uint8_t)data->items[i][j].y_coef);
            internal_data.items[i][j] = htole16 (val);
        }
    }

    microtouch3m_log_buffer ("setting linearization data...", (const uint8_t *)&internal_data, sizeof (struct internal_linearization_data_s));

    return run_out_request (dev,
                            REQUEST_SET_PARAMETER_BLOCK,
                            PARAMETER_ID_CONTROLLER_NOVRAM,
                            (LINEARIZATION_DATA_BLOCK << 8),
                            (const uint8_t *) &internal_data,
                            sizeof (struct internal_linearization_data_s),
                            NULL);
}

/******************************************************************************/
/* Device async report operation */

#define MAX_INTERRUPT_ENDPOINT_TRANSFER 32

struct report_scope_s {
    uint8_t  report_id;
    uint16_t wtf;
    uint32_t ul_i;
    uint32_t ul_q;
    uint32_t ur_i;
    uint32_t ur_q;
    uint32_t ll_i;
    uint32_t ll_q;
    uint32_t lr_i;
    uint32_t lr_q;
} __attribute__((packed));

microtouch3m_status_t
microtouch3m_device_monitor_async_reports (microtouch3m_device_t                    *dev,
                                           microtouch3m_device_async_report_scope_f *callback,
                                           void                                     *user_data)
{
    microtouch3m_status_t st;
    bool                  continue_loop = true;

    if (libusb_kernel_driver_active (dev->usbhandle, 0)) {
        microtouch3m_log ("kernel driver is active...");
        if (libusb_detach_kernel_driver (dev->usbhandle, 0) == 0)
            microtouch3m_log ("kernel driver now detached");
    }

    if (libusb_claim_interface (dev->usbhandle, 0) < 0) {
        microtouch3m_log ("couldn't claim USB interface");
        return MICROTOUCH3M_STATUS_FAILED;
    }

    microtouch3m_log ("disable coordinate data reports...");
    if ((st = run_out_request (dev,
                               REQUEST_ASYNC_SET_REPORT,
                               ASYNC_SET_REPORT_DISABLE,
                               REPORT_ID_COORDINATE_DATA,
                               NULL,
                               0,
                               NULL)) != MICROTOUCH3M_STATUS_OK) {
        microtouch3m_log ("error: couldn't disable coordinate data reports");
        return st;
    }

    microtouch3m_log ("disable scope data reports...");
    if ((st = run_out_request (dev,
                               REQUEST_ASYNC_SET_REPORT,
                               ASYNC_SET_REPORT_DISABLE,
                               REPORT_ID_SCOPE_DATA,
                               NULL,
                               0,
                               NULL)) != MICROTOUCH3M_STATUS_OK) {
        microtouch3m_log ("error: couldn't disable scope data reports");
        return st;
    }

    microtouch3m_log ("reading current status...");
    {
        struct extended_status_report_s status;

        if ((st = device_get_status_extended (dev, &status)) != MICROTOUCH3M_STATUS_OK)
            return st;

        /* NOTE: we probably want to do something here, like check that all reports are
         * disabled */
    }

    microtouch3m_log ("enable scope data reports...");
    if ((st = run_out_request (dev,
                               REQUEST_ASYNC_SET_REPORT,
                               ASYNC_SET_REPORT_ENABLE,
                               REPORT_ID_SCOPE_DATA,
                               NULL,
                               0,
                               NULL)) != MICROTOUCH3M_STATUS_OK) {
        microtouch3m_log ("error: couldn't enable scope data reports");
        return st;
    }

    microtouch3m_log ("scope mode enabled");

    while (continue_loop) {
        /* Note: we want 35 bytes, but we can only read 32 max at the same time... */
        struct report_scope_s report = { 0 };
        int                   transferred = 0;
        int32_t               ul_i, ul_q;
        int32_t               ur_i, ur_q;
        int32_t               ll_i, ll_q;
        int32_t               lr_i, lr_q;

        assert (sizeof (report) > MAX_INTERRUPT_ENDPOINT_TRANSFER);
        if (libusb_interrupt_transfer (dev->usbhandle,
                                       (LIBUSB_ENDPOINT_IN | 1),
                                       (uint8_t *) &report,
                                       MAX_INTERRUPT_ENDPOINT_TRANSFER,
                                       &transferred,
                                       5000) != 0) {
            goto report_error;
        }
        if (transferred != MAX_INTERRUPT_ENDPOINT_TRANSFER)
            goto report_error;
        microtouch3m_log_buffer ("async report received", (uint8_t *) &report, transferred);

        if (libusb_interrupt_transfer (dev->usbhandle,
                                       (LIBUSB_ENDPOINT_IN | 1),
                                       &(((uint8_t *) &report)[MAX_INTERRUPT_ENDPOINT_TRANSFER]),
                                       sizeof (report) - MAX_INTERRUPT_ENDPOINT_TRANSFER,
                                       &transferred,
                                       5000) != 0) {
            goto report_error;
        }
        if (transferred != sizeof (report) - MAX_INTERRUPT_ENDPOINT_TRANSFER)
            goto report_error;
        microtouch3m_log_buffer ("async report received", &(((uint8_t *) &report)[MAX_INTERRUPT_ENDPOINT_TRANSFER]), transferred);

        ul_i = (int32_t) (le32toh (report.ul_i));
        ul_q = (int32_t) (le32toh (report.ul_q));
        ur_i = (int32_t) (le32toh (report.ur_i));
        ur_q = (int32_t) (le32toh (report.ur_q));
        ll_i = (int32_t) (le32toh (report.ll_i));
        ll_q = (int32_t) (le32toh (report.ll_q));
        lr_i = (int32_t) (le32toh (report.lr_i));
        lr_q = (int32_t) (le32toh (report.lr_q));

#if 0
        microtouch3m_log ("UL(I): %d", ul_i);
        microtouch3m_log ("UL(Q): %d", ul_q);
        microtouch3m_log ("UR(I): %d", ur_i);
        microtouch3m_log ("UR(Q): %d", ur_q);
        microtouch3m_log ("LL(I): %d", ll_i);
        microtouch3m_log ("LL(Q): %d", ll_q);
        microtouch3m_log ("LR(I): %d", lr_i);
        microtouch3m_log ("LR(Q): %d", lr_q);
#endif

        continue_loop = callback (dev,
                                  MICROTOUCH3M_STATUS_OK,
                                  ul_i, ul_q,
                                  ur_i, ur_q,
                                  ll_i, ll_q,
                                  lr_i, lr_q,
                                  user_data);
        continue;

 report_error:
        continue_loop = callback (dev,
                                  MICROTOUCH3M_STATUS_FAILED,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  user_data);
    }

    microtouch3m_log ("operation finished");

    run_out_request (dev,
                     REQUEST_ASYNC_SET_REPORT,
                     ASYNC_SET_REPORT_DISABLE,
                     REPORT_ID_SCOPE_DATA,
                     NULL,
                     0,
                     NULL);

    libusb_release_interface (dev->usbhandle, 0);

    microtouch3m_log ("scope mode disabled");
    return MICROTOUCH3M_STATUS_OK;
}

/******************************************************************************/
/* Firmware common */

void
microtouch3m_device_firmware_progress_register (microtouch3m_device_t                   *dev,
                                                microtouch3m_device_firmware_progress_f *callback,
                                                float                                    freq,
                                                void                                    *user_data)
{
    dev->progress_callback  = callback;
    dev->progress_freq      = freq;
    dev->progress_user_data = user_data;
}

static void
report_progress (microtouch3m_device_t *dev,
                 int                    current_step,
                 int                    total_steps,
                 float                 *progress)
{
    float new_progress;

    if (!dev->progress_callback)
        return;

    new_progress = 100.0 * (((float) current_step) / ((float) total_steps));
    if (!progress || (new_progress > (*progress + dev->progress_freq))) {
        dev->progress_callback (dev, new_progress, dev->progress_user_data);
        if (progress)
            *progress = new_progress;
    }
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
    float        progress = 0.0;

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

        if ((st = run_parameter_in_request (dev,
                                            REQUEST_GET_PARAMETER_BLOCK,
                                            PARAMETER_ID_CONTROLLER_EEPROM,
                                            offset,
                                            (struct parameter_report_s *) &parameter_report,
                                            sizeof (parameter_report),
                                            NULL)) != MICROTOUCH3M_STATUS_OK)
            return st;

        memcpy (&buffer[offset], parameter_report.data, PARAMETER_REPORT_FIRMWARE_DUMP_DATA_SIZE);
        report_progress (dev, offset, MICROTOUCH3M_FW_IMAGE_SIZE, &progress);
    }

    if (progress < 100.0)
        report_progress (dev, MICROTOUCH3M_FW_IMAGE_SIZE, MICROTOUCH3M_FW_IMAGE_SIZE, NULL);

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
    struct microtouch3m_device_linearization_data_s linearization_data;
    uint8_t calibration_data   [CALIBRATION_DATA_SIZE];
    uint8_t orientation_data   [ORIENTATION_DATA_SIZE];
    uint8_t identifier_data    [IDENTIFIER_DATA_SIZE];
};

microtouch3m_status_t
microtouch3m_device_backup_data (microtouch3m_device_t       *dev,
                                 microtouch3m_device_data_t **out_data,
                                 size_t                      *out_data_size)
{
    microtouch3m_status_t       st;
    microtouch3m_device_data_t *data;

    assert (dev);
    assert (out_data || out_data_size);

    /* Only interested in size? */
    if (!out_data && out_data_size) {
        *out_data_size = sizeof (microtouch3m_device_data_t);
        return MICROTOUCH3M_STATUS_OK;
    }

    data = calloc (1, sizeof (struct microtouch3m_device_data_s));
    if (!data)
        return MICROTOUCH3M_STATUS_NO_MEMORY;

    microtouch3m_log ("backing up calibration data...");
    {
        struct parameter_report_calibration_data_s parameter_report;

        if ((st = run_parameter_in_request (dev,
                                            REQUEST_GET_PARAMETER_BLOCK,
                                            PARAMETER_ID_CONTROLLER_NOVRAM,
                                            (CALIBRATION_DATA_BLOCK << 8),
                                            (struct parameter_report_s *) &parameter_report,
                                            sizeof (parameter_report),
                                            NULL)) != MICROTOUCH3M_STATUS_OK)
            goto out;

        memcpy (data->calibration_data, parameter_report.data, CALIBRATION_DATA_SIZE);
        microtouch3m_log_buffer ("calibration data backed up", data->calibration_data, CALIBRATION_DATA_SIZE);
    }

    microtouch3m_log ("backing up linearization data...");
    if ((st = microtouch3m_device_get_linearization_data (dev, &data->linearization_data)) != MICROTOUCH3M_STATUS_OK)
        goto out;
    microtouch3m_log_buffer ("linearization data backed up", (const uint8_t *) &(data->linearization_data), sizeof (data->linearization_data));

    microtouch3m_log ("backing up orientation data...");
    {
        struct parameter_report_orientation_data_s parameter_report;

        if ((st = run_parameter_in_request (dev,
                                            REQUEST_GET_PARAMETER,
                                            ORIENTATION_PARAMETER_NUMBER,
                                            0x0000,
                                            (struct parameter_report_s *) &parameter_report,
                                            sizeof (parameter_report),
                                            NULL)) != MICROTOUCH3M_STATUS_OK)
            goto out;

        memcpy (data->orientation_data, parameter_report.data, ORIENTATION_DATA_SIZE);
        microtouch3m_log_buffer ("orientation data backed up", data->orientation_data, ORIENTATION_DATA_SIZE);
    }

    microtouch3m_log ("backing up identifier data...");
    {
        struct parameter_report_identifier_data_s parameter_report;

        if ((st = run_parameter_in_request (dev,
                                            REQUEST_GET_PARAMETER,
                                            IDENTIFIER_PARAMETER_NUMBER,
                                            0x0000,
                                            (struct parameter_report_s *) &parameter_report,
                                            sizeof (parameter_report),
                                            NULL)) != MICROTOUCH3M_STATUS_OK)
            goto out;

        memcpy (data->identifier_data, parameter_report.data, IDENTIFIER_DATA_SIZE);
        microtouch3m_log_buffer ("identifier data backed up", data->identifier_data, IDENTIFIER_DATA_SIZE);
    }

    /* Success! */
    microtouch3m_log ("successfully backed up controller data");
    st = MICROTOUCH3M_STATUS_OK;

out:
    if (st != MICROTOUCH3M_STATUS_OK)
        free (data);
    else {
        if (out_data_size)
            *out_data_size = sizeof (microtouch3m_device_data_t);
        *out_data = data;
    }
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
                               CALIBRATION_DATA_SIZE,
                               NULL)) != MICROTOUCH3M_STATUS_OK)
        return st;

    microtouch3m_log ("restoring linearization data...");

    if ((st = microtouch3m_device_set_linearization_data (dev, &data->linearization_data)) != MICROTOUCH3M_STATUS_OK)
        return st;

    microtouch3m_log ("restoring orientation data...");
    if ((st = run_out_request (dev,
                               REQUEST_SET_PARAMETER,
                               ORIENTATION_PARAMETER_NUMBER,
                               0x0000,
                               data->orientation_data,
                               ORIENTATION_DATA_SIZE,
                               NULL)) != MICROTOUCH3M_STATUS_OK)
        return st;

    microtouch3m_log ("restoring identifier data...");
    if ((st = run_out_request (dev,
                               REQUEST_SET_PARAMETER,
                               IDENTIFIER_PARAMETER_NUMBER,
                               0x0000,
                               data->identifier_data,
                               IDENTIFIER_DATA_SIZE,
                               NULL)) != MICROTOUCH3M_STATUS_OK)
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
    float        progress = 0.0;

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
                                   FIRMWARE_UPDATE_DATA_SIZE,
                                   NULL)) != MICROTOUCH3M_STATUS_OK)
            return st;

        report_progress (dev, offset, MICROTOUCH3M_FW_IMAGE_SIZE, &progress);
    }

    if (progress < 100.0)
        report_progress (dev, MICROTOUCH3M_FW_IMAGE_SIZE, MICROTOUCH3M_FW_IMAGE_SIZE, NULL);

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
    FILE                  *f = NULL;
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
    FILE                  *f = NULL;
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

/******************************************************************************/
/* Library version info */

unsigned int
microtouch3m_get_major_version (void)
{
    return MICROTOUCH3M_MAJOR_VERSION;
}

unsigned int
microtouch3m_get_minor_version (void)
{
    return MICROTOUCH3M_MINOR_VERSION;
}

unsigned int
microtouch3m_get_micro_version (void)
{
    return MICROTOUCH3M_MICRO_VERSION;
}
