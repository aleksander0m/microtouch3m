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
 * @MICROTOUCH3M_STATUS_INVALID_STATE: Invalid state.
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
    MICROTOUCH3M_STATUS_INVALID_STATE,
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
 * microtouch3m_device_new_first:
 * @ctx: a #microtouch3m_context_t.
 *
 * Creates a new #microtouch3m_device_t to manage the first MicroTouch 3M device
 * found in the system. This is particularly useful when just one controller is
 * expected, which is the most usual case.
 *
 * If more than one MicroTouch 3M device is available in the system, either
 * microtouch3m_device_new_by_usb_address() or
 * microtouch3m_device_new_by_usb_location() should be used instead for a
 * correct operation.
 *
 * When no longer used, the device should be disposed with
 * microtouch3m_device_unref().
 *
 * Returns: a newly allocated #microtouch3m_device_t.
 */
microtouch3m_device_t *microtouch3m_device_new_first (microtouch3m_context_t *ctx);

/**
 * microtouch3m_device_new_by_usb_address:
 * @ctx: a #microtouch3m_context_t.
 * @bus_number: bus number.
 * @device_address: device address in the bus.
 *
 * Creates a new #microtouch3m_device_t to manage the MicroTouch 3M device
 * available at device address @device_address in bus @bus_number.
 *
 * When no longer used, the device should be disposed with
 * microtouch3m_device_unref().
 *
 * Returns: a newly allocated #microtouch3m_device_t.
 */
microtouch3m_device_t *microtouch3m_device_new_by_usb_address (microtouch3m_context_t *ctx,
                                                               uint8_t                 bus_number,
                                                               uint8_t                 device_address);

/**
 * microtouch3m_device_new_by_usb_location:
 * @ctx: a #microtouch3m_context_t.
 * @bus_number: bus number.
 * @port_numbers: list of ports.
 * @port_numbers_len: number of items in @port_numbers.
 *
 * Creates a new #microtouch3m_device_t to manage the MicroTouch 3M device
 * available at the USB location specified by the given @bus_number and
 * @port_numbers.
 *
 * The list of @port_numbers is based on the physical position of the USB device
 * and therefore if the device is reseted, when exposed again in the USB tree,
 * it will be available at exactly the same @bus_number and  @port_numbers.
 *
 * When no longer used, the device should be disposed with
 * microtouch3m_device_unref().
 *
 * Returns: a newly allocated #microtouch3m_device_t.
 */
microtouch3m_device_t *microtouch3m_device_new_by_usb_location (microtouch3m_context_t *ctx,
                                                                uint8_t                 bus_number,
                                                                const uint8_t          *port_numbers,
                                                                int                     port_numbers_len);

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
 * @dev: a #microtouch3m_device_t.
 *
 * Get the USB bus number where this device is available.
 *
 * Returns: the USB bus number.
 */
uint8_t microtouch3m_device_get_usb_bus_number (microtouch3m_device_t *dev);

/**
 * microtouch3m_device_get_usb_device_address:
 * @dev: a #microtouch3m_device_t.
 *
 * Get the USB device address in the bus where this device is available.
 *
 * Returns: the USB device address.
 */
uint8_t microtouch3m_device_get_usb_device_address (microtouch3m_device_t *dev);

/**
 * microtouch3m_device_get_usb_location:
 * @dev: a #microtouch3m_device_t.
 * @port_numbers: the array that will contain the port numbers.
 * @port_numbers_len: size of @port_numbers. As per the USB 3.0 specs, the current maximum limit for the depth is 7.
 *
 * Get the list of ports defining the location of this device in the USB tree.
 *
 * Returns: the number of elements in @port_numbers filled, or 0 on error.
 */
uint8_t microtouch3m_device_get_usb_location (microtouch3m_device_t *dev,
                                              uint8_t               *port_numbers,
                                              int                    port_numbers_len);

/******************************************************************************/
/* Open and close */

/**
 * microtouch3m_device_open:
 * @dev: a #microtouch3m_device_t.
 *
 * Open the #microtouch_device_t.
 *
 * Returns: a #microtouch3m_status_t.
 */
microtouch3m_status_t microtouch3m_device_open (microtouch3m_device_t *dev);

/**
 * microtouch3m_device_close:
 * @dev: a #microtouch3m_device_t.
 *
 * Close the #microtouch_device_t.
 */
void microtouch3m_device_close (microtouch3m_device_t *dev);

/******************************************************************************/
/* Query controller ID */

microtouch3m_status_t microtouch3m_device_query_controller_id (microtouch3m_device_t *dev,
                                                               uint16_t              *controller_type,
                                                               uint8_t               *firmware_major,
                                                               uint8_t               *firmware_minor,
                                                               uint8_t               *features,
                                                               uint16_t              *constants_checksum,
                                                               uint16_t              *max_param_write,
                                                               uint32_t              *pc_checksum,
                                                               uint16_t              *asic_type);

/******************************************************************************/
/* Reset */

/**
 * microtouch3m_device_reset_t:
 * @MICROTOUCH3M_DEVICE_RESET_SOFT: Soft reset.
 * @MICROTOUCH3M_DEVICE_RESET_HARD: Hard reset.
 * @MICROTOUCH3M_DEVICE_RESET_REBOOT: Reboot after a firmware update operation.
 *
 * Type of reset operation.
 */
typedef enum {
    MICROTOUCH3M_DEVICE_RESET_SOFT   = 0x0001,
    MICROTOUCH3M_DEVICE_RESET_HARD   = 0x0002,
    MICROTOUCH3M_DEVICE_RESET_REBOOT = 0x0005,
} microtouch3m_device_reset_t;

/**
 * microtouch3m_device_reset_to_string:
 * @reset: a #microtouch3m_device_reset_t.
 *
 * Gets a description for the given #microtouch3m_device_reset_t.
 *
 * Returns: a constant string.
 */
const char *microtouch3m_device_reset_to_string (microtouch3m_device_reset_t reset);

/**
 * microtouch3m_device_reset:
 * @dev: a #microtouch3m_device_t.
 * @reset: type of reset.
 *
 * Reset the device.
 *
 * Returns: a #microtouch3m_status_t.
 */
microtouch3m_status_t microtouch3m_device_reset (microtouch3m_device_t       *dev,
                                                 microtouch3m_device_reset_t  reset);

/******************************************************************************/
/* Device firmware operations */

/**
 * MICROTOUCH3M_FW_IMAGE_SIZE:
 *
 * Size of a firmware image file.
 */
#define MICROTOUCH3M_FW_IMAGE_SIZE (size_t) 0x8000

/**
 * microtouch3m_device_firmware_progress_f:
 * @dev: a #microtouch3m_device_t.
 * @progress: the operation progress, in percentage.
 * @user_data: user provided data when registering the callback.
 *
 * Callback operation that may be registered if the user needs to show the
 * progress of the ongoing operation.
 *
 */
typedef void (microtouch3m_device_firmware_progress_f) (microtouch3m_device_t *dev,
                                                        float                  progress,
                                                        void                  *user_data);

/**
 * microtouch3m_device_firmware_progress_register:
 * @dev: a #microtouch3m_device_t.
 * @callback: the progress callback, or %NULL.
 * @freq: how often to report updates, in percentage.
 * @user_data: user provided data to be used when @callback is called.
 *
 * Registers a callback operation to be called during firmware operations
 * (either dump or update).
 *
 * If %NULL given as @callback, no progress operation will be reported.
 */
void microtouch3m_device_firmware_progress_register (microtouch3m_device_t                   *dev,
                                                     microtouch3m_device_firmware_progress_f *callback,
                                                     float                                    freq,
                                                     void                                    *user_data);

/**
 * microtouch3m_device_firmware_dump:
 * @dev: a #microtouch3m_device_t.
 * @buffer: buffer where the firmware contents will be stored.
 * @buffer_size: size of @buffer (at least #MICROTOUCH3M_FW_IMAGE_SIZE bytes).
 *
 * Instruct the device to dump the firmware and load it in memory.
 *
 * Returns: a #microtouch3m_status_t.
 */
microtouch3m_status_t microtouch3m_device_firmware_dump (microtouch3m_device_t *dev,
                                                         uint8_t               *buffer,
                                                         size_t                 buffer_size);

/**
 * microtouch3m_device_firmware_update:
 * @dev: a #microtouch3m_device_t.
 * @buffer: buffer where the firmware contents are stored.
 * @buffer_size: size of @buffer (at least #MICROTOUCH3M_FW_IMAGE_SIZE bytes).
 *
 * Instruct the device to update the firmware.
 *
 * Returns: a #microtouch3m_status_t.
 */
microtouch3m_status_t microtouch3m_device_firmware_update (microtouch3m_device_t *dev,
                                                           const uint8_t         *buffer,
                                                           size_t                 buffer_size);

/**
 * microtouch3m_device_data_t:
 *
 * Opaque type representing the data that should be backed up before a firmware
 * update and restored after the update has finished.
 *
 * This data includes calibration, linearization, orientation and identifier
 * data.
 */
typedef struct microtouch3m_device_data_s microtouch3m_device_data_t;

/**
 * microtouch3m_device_backup_data:
 * @dev: a #microtouch3m_device_t.
 * @out_data: output location to store a newly allocated #microtouch3m_device_data_t.
 *
 * Backs up the device data.
 *
 * The @out_data will only be set if #MICROTOUCH3M_STATUS_OK is returned, and
 * should be disposed with free() when no longer used.
 *
 * Returns: a #microtouch3m_status_t.
 */
microtouch3m_status_t microtouch3m_device_backup_data (microtouch3m_device_t       *dev,
                                                       microtouch3m_device_data_t **out_data);

/**
 * microtouch3m_device_restore_data:
 * @dev: a #microtouch3m_device_t.
 * @data: a #microtouch3m_device_data_t.
 *
 * Restores the device data.
 *
 * Returns: a #microtouch3m_status_t.
 */
microtouch3m_status_t microtouch3m_device_restore_data (microtouch3m_device_t            *dev,
                                                        const microtouch3m_device_data_t *data);

/******************************************************************************/
/* Firmware files */

/**
 * microtouch3m_firmware_file_read:
 * @path: local path to a firmware file.
 * @buffer: buffer where the firmware file contents will be stored, or %NULL to just validate the contents.
 * @buffer_size: if @buffer given, size of @buffer (at least #MICROTOUCH3M_FW_IMAGE_SIZE bytes).
 *
 * Validate the input firmware file and optionally also load it into memory.
 *
 * Returns: a #microtouch3m_status_t.
 */
microtouch3m_status_t microtouch3m_firmware_file_read (const char *path,
                                                       uint8_t    *buffer,
                                                       size_t      buffer_size);

/**
 * microtouch3m_firmware_file_write:
 * @path: local path to the destination firmware file.
 * @buffer: buffer where the firmware file contents are stored.
 * @buffer_size: size of @buffer (at least #MICROTOUCH3M_FW_IMAGE_SIZE bytes).
 *
 * Write the firmware contents from @buffer into a file.
 *
 * Note that if @buffer_size is bigger than #MICROTOUCH3M_FW_IMAGE_SIZE, the
 * exceeding bytes will be ignored.
 *
 * Returns: a #microtouch3m_status_t.
 */
microtouch3m_status_t microtouch3m_firmware_file_write (const char    *path,
                                                        const uint8_t *buffer,
                                                        size_t         buffer_size);

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
