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
#include <stdint.h>

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
 * microtouch3m_context_new:
 *
 * Initializes the library and creates a newly allocated context.
 *
 * When no longer used, the allocated context should be disposed with
 * microtouch3m_context_unref().
 *
 * Returns: a newly allocated #microtouch3m_context_t.
 */
microtouch3m_context_t *microtouch3m_context_new (void);

/**
 * microtouch3m_context_ref:
 * @ctx: a #microtouch3m_context_t.
 *
 * Increases the reference count of a #microtouch3m_context_t.
 *
 * Returns: the new full reference.
 */
microtouch3m_context_t *microtouch3m_context_ref (microtouch3m_context_t *ctx);

/**
 * microtouch3m_context_unref:
 * @ctx: a #microtouch3m_context_t.
 *
 * Decreases the reference count of a #microtouch3m_context_t. It will be fully
 * disposed when the count reaches 0.
 */
void microtouch3m_context_unref (microtouch3m_context_t *ctx);

/******************************************************************************/
/* Device */

/**
 * microtouch3m_device_t:
 *
 * A opaque type representing a MicroTouch 3M device.
 */
typedef struct microtouch3m_device_s microtouch3m_device_t;

/**
 * microtouch3m_device_new:
 * @ctx: a #microtouch3m_context_t.
 * @busnum: bus number, or 0.
 * @devnum: device number, or 0.
 *
 * Creates a new #microtouch3m_device_t to manage the MicroTouch 3M device
 * available at device address @devnum in bus @busnum.
 *
 * @busnum may be 0 if no explicit bus number is requested. If so, all available
 * buses will be scanned looking for MicroTouch 3M devices.
 *
 * @devnum may be 0 if no explicit device address is requested. If so, the first
 * MicroTouch 3M device found will be considered (if @busnum is given, only the
 * specific bus will be scanned).
 *
 * If more than one MicroTouch 3M device is available in the system, both
 * @busnum and @devnum should be given for a correct operation.
 *
 * When no longer used, the device should be disposed with
 * microtouch3m_device_unref().
 *
 * Returns: a newly allocated #microtouch3m_device_t.
 */
microtouch3m_device_t *microtouch3m_device_new (microtouch3m_context_t *ctx,
                                                uint8_t                 busnum,
                                                uint8_t                 devnum);

/**
 * microtouch3m_device_ref:
 * @dev: a #microtouch3m_device_t.
 *
 * Increases the reference count of a #microtouch3m_device_t.
 *
 * Returns: the new full reference.
 */
microtouch3m_device_t * microtouch3m_device_ref (microtouch3m_device_t *dev);

/**
 * microtouch3m_device_unref:
 * @dev: a #microtouch3m_device_t.
 *
 * Decreases the reference count of a #microtouch3m_device_t. It will be fully
 * disposed when the count reaches 0.
 */
void microtouch3m_device_unref (microtouch3m_device_t *dev);

/**
 * microtouch3m_device_get_usb_bus_number:
 *
 * Get the USB bus number where this device is available.
 *
 * Returns: the USB bus number.
 */
uint8_t microtouch3m_device_get_usb_bus_number (microtouch3m_device_t *dev);

/**
 * microtouch3m_device_get_usb_device_address:
 *
 * Get the USB device address in the bus where this device is available.
 *
 * Returns: the USB device address.
 */
uint8_t microtouch3m_device_get_usb_device_address (microtouch3m_device_t *dev);

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
