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

#if !defined __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
# error GCC built-in atomic method support is required
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
};

static libusb_device *
find_usb_device (microtouch3m_context_t *ctx,
                 uint8_t                 busnum,
                 uint8_t                 devnum)
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
        uint8_t                          iter_busnum;
        uint8_t                          iter_devnum;
        struct libusb_device_descriptor  desc;

        iter = list[i];

        iter_busnum = libusb_get_bus_number (iter);
        if (busnum != iter_busnum && busnum != 0)
            continue;

        iter_devnum = libusb_get_device_address (iter);
        if (devnum != iter_devnum && devnum != 0)
            continue;

        if (libusb_get_device_descriptor (iter, &desc) != 0)
            continue;

        if (desc.idVendor != MICROTOUCH3M_VID)
            continue;

        if (desc.idProduct != MICROTOUCH3M_PID)
            continue;

        found = libusb_ref_device (iter);
        microtouch3m_log ("found MicroTouch 3M device at %03u:%03u)", iter_busnum, iter_devnum);
    }

    libusb_free_device_list (list, 1);

    if (!found)
        microtouch3m_log ("error: couldn't find MicroTouch 3M device at %03u:%03u", busnum, devnum);

    return found;
}

microtouch3m_device_t *
microtouch3m_device_new (microtouch3m_context_t *ctx,
                         uint8_t                 busnum,
                         uint8_t                 devnum)
{
    microtouch3m_device_t *dev;
    libusb_device         *usbdev;

    dev = calloc (1, sizeof (microtouch3m_device_t));
    if (!dev)
        goto outerr;

    usbdev = find_usb_device (ctx, busnum, devnum);
    if (!usbdev)
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

/******************************************************************************/
/* Parameter block management */

enum request_e {
    REQUEST_GET_PARAMETER = 0x02,
    REQUEST_SET_PARAMETER = 0x03,
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
read_parameter_block (microtouch3m_device_t     *dev,
                      libusb_device_handle      *handle,
                      uint16_t                   parameter_value,
                      uint16_t                   parameter_index,
                      struct parameter_report_s *parameter_report,
                      size_t                     parameter_report_size)
{
    int desc_size;

    assert (dev);
    assert (handle);
    assert (parameter_report);

    if ((desc_size = libusb_control_transfer (handle,
                                              LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                              REQUEST_GET_PARAMETER,
                                              parameter_value,
                                              parameter_index,
                                              (uint8_t *) parameter_report,
                                              parameter_report_size,
                                              5000)) < 0) {
        microtouch3m_log ("error: couldn't read parameter value 0x%04x index 0x%04x", parameter_value, parameter_index);
        return MICROTOUCH3M_STATUS_INVALID_IO;
    }

    if (desc_size != parameter_report_size) {
        microtouch3m_log ("error: couldn't read parameter value 0x%04x index 0x%04x: invalid data size read (%d != %d)",
                          parameter_value, parameter_index, desc_size, parameter_report_size);
        return MICROTOUCH3M_STATUS_INVALID_DATA;
    }

    if (parameter_report->report_id != REPORT_ID_PARAMETER) {
        microtouch3m_log ("error: couldn't read parameter value 0x%04x index 0x%04x: invalid report id (%d != %d)",
                          parameter_value, parameter_index, parameter_report->report_id, REPORT_ID_PARAMETER);
        return MICROTOUCH3M_STATUS_INVALID_DATA;
    }

    if (parameter_report->data_size != (parameter_report_size - sizeof (struct parameter_report_s))) {
        microtouch3m_log ("error: couldn't read parameter value 0x%04x index 0x%04x: invalid read data size reported (%d != %d)",
                          parameter_value, parameter_index, parameter_report->data_size, (parameter_report_size - sizeof (struct parameter_report_s)));
        return MICROTOUCH3M_STATUS_INVALID_FORMAT;
    }

    microtouch3m_log ("read parameter value 0x%04x index 0x%04x", parameter_value, parameter_index);
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
    libusb_device_handle  *handle = NULL;
    microtouch3m_status_t  st;
    uint16_t               offset;
    unsigned int           i;

    assert (dev);
    assert (buffer);

    if (buffer_size < MICROTOUCH3M_FW_IMAGE_SIZE) {
        microtouch3m_log ("error: not enough space in buffer to contain the full firmware image file (%zu < %zu)",
                          buffer_size, MICROTOUCH3M_FW_IMAGE_SIZE);
        st = MICROTOUCH3M_STATUS_INVALID_ARGUMENTS;
        goto out;
    }

    if (libusb_open (dev->usbdev, &handle) < 0) {
        microtouch3m_log ("error: couldn't open usb device");
        st = MICROTOUCH3M_STATUS_FAILED;
        goto out;
    }

    microtouch3m_log ("reading firmware from controller EEPROM...");

    for (i = 0, offset = 0; offset < MICROTOUCH3M_FW_IMAGE_SIZE; offset += PARAMETER_REPORT_FIRMWARE_DUMP_DATA_SIZE, i++) {
        struct parameter_report_firmware_dump_s parameter_report;

        if ((st = read_parameter_block (dev,
                                        handle,
                                        PARAMETER_ID_CONTROLLER_EEPROM,
                                        offset,
                                        (struct parameter_report_s *) &parameter_report,
                                        sizeof (parameter_report))) != MICROTOUCH3M_STATUS_OK)
            goto out;

        memcpy (&buffer[offset], parameter_report.data, PARAMETER_REPORT_FIRMWARE_DUMP_DATA_SIZE);
    }

    /* Success! */
    microtouch3m_log ("successfully read firmware from controller EEPROM");
    st = MICROTOUCH3M_STATUS_OK;

out:
    if (handle)
        libusb_close (handle);
    return st;
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
