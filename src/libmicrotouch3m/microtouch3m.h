/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * All rights reserved.
 *
 * Author: Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef MICROTOUCH3M_H
#define MICROTOUCH3M_H

#include <pthread.h>

/******************************************************************************/
/* Status */

/**
 * microtouch3m_status_t:
 * @MICROTOUCH3M_STATUS_OK: Operation successful.
 * @MICROTOUCH3M_STATUS_FAILED: Operation failed.
 * @MICROTOUCH3M_STATUS_NO_MEMORY: Not enough memory.
 * @MICROTOUCH3M_STATUS_INVALID_ARGUMENTS: Invalid arguments.
 * @MICROTOUCH3M_STATUS_INVALID_IO: Invalid I/O.
 * @MICROTOUCH3M_STATUS_INVALID_DATA: Invalid data.
 * @MICROTOUCH3M_STATUS_INVALID_FORMAT: Invalid format.
 *
 * Status of an operation performed with the MicroTouch 3M library.
 */
typedef enum {
    MICROTOUCH3M_STATUS_OK,
    MICROTOUCH3M_STATUS_FAILED,
    MICROTOUCH3M_STATUS_NO_MEMORY,
    MICROTOUCH3M_STATUS_INVALID_ARGUMENTS,
    MICROTOUCH3M_STATUS_INVALID_IO,
    MICROTOUCH3M_STATUS_INVALID_DATA,
    MICROTOUCH3M_STATUS_INVALID_FORMAT,
} microtouch3m_status_t;

/**
 * microtouch3m_status_to_string:
 * @st: a #microtouch3m_status_t.
 *
 * Gets a description for the given #microtouch3m_status_t.
 *
 * Returns: a constant string.
 */
const char *microtouch3m_status_to_string (microtouch3m_status_t st);

/******************************************************************************/
/* Library context */

/**
 * microtouch3m_context_t:
 *
 * A opaque type representing the library context.
 */
typedef struct microtouch3m_context_s microtouch3m_context_t;

/**
 * microtouch3m_context_init:
 * @out_ctx: output location to store a newly allocated #microtouch3m_context_t.
 *
 * Initializes the library and creates a newly allocated context.
 *
 * The context is only transferred to the user if the returned status is
 * %MICROTOUCH3M_STATUS_OK.
 *
 * When no longer used, the allocated context should be disposed with
 * microtouch3m_context_free().
 *
 * Returns: a #microtouch3m_status_t.
 */
microtouch3m_status_t microtouch3m_context_init (microtouch3m_context_t **out_ctx);

/**
 * microtouch3m_context_free:
 *
 * Free a #microtouch3m_context_t that was initialized with
 * microtouch3m_context_init().
 */
void microtouch3m_context_free (microtouch3m_context_t *ctx);

/******************************************************************************/
/* Firmware files */

/**
 * MICROTOUCH3M_FW_IMAGE_SIZE:
 *
 * Size of a firmware image file.
 */
#define MICROTOUCH3M_FW_IMAGE_SIZE (size_t) 0x8000

/**
 * microtouch3m_firmware_file_load:
 * @path: local path to a firmware file.
 * @buffer: buffer where the firmware file contents will be stored, or %NULL to just validate the contents.
 * @buffer_size: if @buffer given, size of @buffer (at least #MICROTOUCH3M_FW_IMAGE_SIZE bytes).
 *
 * Validate the input firmware file and optionally also load it into memory.
 *
 * Returns: a #microtouch3m_status_t.
 */
microtouch3m_status_t microtouch3m_firmware_file_load (const char *path,
                                                       uint8_t    *buffer,
                                                       size_t      buffer_size);

/******************************************************************************/
/* Logging */

/**
 * microtouch3m_log_handler_t:
 * @thread_id: thread ID where the log message was generated.
 * @message: the log message.

 * Logging handler.
 */
typedef void (* microtouch3m_log_handler_t) (pthread_t   thread_id,
                                             const char *message);

/**
 * microtouch3m_log_set_handler:
 *
 * Set logging handler.
 *
 * This method is NOT thread-safe, and would be usually called once before
 * any other operation with the library.
 */
void microtouch3m_log_set_handler (microtouch3m_log_handler_t handler);

#endif /* MICROTOUCH3M_H */
