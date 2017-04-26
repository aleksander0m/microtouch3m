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

#include "common.h"

#include "microtouch3m.h"
#include "microtouch3m-log.h"

#include "ihex.h"

/******************************************************************************/
/* Status */

static const char *status_str[] = {
    [MICROTOUCH3M_STATUS_OK]                = "success",
    [MICROTOUCH3M_STATUS_FAILED]            = "failed",
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
microtouch3m_firmware_file_load (const char *path,
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
