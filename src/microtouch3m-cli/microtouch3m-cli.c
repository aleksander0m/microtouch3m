/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * All rights reserved.
 *
 * Author: Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>

#include <common.h>

#include <microtouch3m.h>

#define PROGRAM_NAME    "microtouch3m-cli"
#define PROGRAM_VERSION PACKAGE_VERSION

/******************************************************************************/
/* Helper: create device based on bus (or first found) */

#define MAX_PORT_NUMBERS 7

static microtouch3m_device_t *
create_device (microtouch3m_context_t *ctx,
               bool                    first,
               uint8_t                 bus_number,
               uint8_t                 device_address,
               const uint8_t          *port_numbers,
               int                     port_numbers_len)
{
    microtouch3m_device_t *dev;
    microtouch3m_status_t  st;
    char                  *location_str;
    uint8_t                real_port_numbers[MAX_PORT_NUMBERS];
    int                    real_port_numbers_len;

    if (first)
        dev = microtouch3m_device_new_first (ctx);
    else if (bus_number && device_address)
        dev = microtouch3m_device_new_by_usb_address (ctx, bus_number, device_address);
    else if (bus_number && port_numbers && port_numbers_len)
        dev = microtouch3m_device_new_by_usb_location (ctx, bus_number, port_numbers, port_numbers_len);
    else
        assert (0);

    if (!dev) {
        fprintf (stderr, "error: couldn't find microtouch 3m device\n");
        return NULL;
    }

    real_port_numbers_len = microtouch3m_device_get_usb_location (dev, real_port_numbers, MAX_PORT_NUMBERS);
    location_str = str_usb_location (microtouch3m_device_get_usb_bus_number (dev), real_port_numbers, real_port_numbers_len);
    printf ("microtouch 3m device found at:\n"
            "\tbus number:     %u\n"
            "\tdevice address: %u\n"
            "\tlocation:       %s\n",
            microtouch3m_device_get_usb_bus_number (dev),
            microtouch3m_device_get_usb_device_address (dev),
            location_str);
    free (location_str);

    if ((st = microtouch3m_device_open (dev)) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't open microtouch 3m device: %s\n", microtouch3m_status_to_string (st));
        microtouch3m_device_unref (dev);
        return NULL;
    }

    return dev;
}

/******************************************************************************/
/* Helper: firmware progress reporting */

#define CLEAR_LINE "\33[2K\r"

static bool disable_progress;

static void
firmware_progress (microtouch3m_device_t *dev,
                   float                  progress,
                   void                  *user_data)
{
    if (disable_progress)
        return;

    printf (CLEAR_LINE " %.2f%%", progress);
    fflush (stdout);
}

/******************************************************************************/
/* ACTION: validate file */

static int
run_validate_fw_file (const char *path)
{
    microtouch3m_status_t st;

    if ((st = microtouch3m_firmware_file_read (path, NULL, 0)) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't validate firmware file: %s\n", microtouch3m_status_to_string (st));
        return EXIT_FAILURE;
    }

    printf ("successfully validated firmware file\n");
    return EXIT_SUCCESS;
}

/******************************************************************************/
/* ACTION: info */

static int
run_info (microtouch3m_context_t *ctx,
          bool                    first,
          uint8_t                 bus_number,
          uint8_t                 device_address)
{
    microtouch3m_device_t *dev;
    microtouch3m_status_t  st;
    uint16_t               controller_type;
    uint8_t                firmware_major;
    uint8_t                firmware_minor;
    uint8_t                features;
    uint16_t               constants_checksum;
    uint16_t               max_param_write;
    uint32_t               pc_checksum;
    uint16_t               asic_type;
    int                    ret = EXIT_FAILURE;

    if (!(dev = create_device (ctx, first, bus_number, device_address, NULL, 0)))
        goto out;

    if ((st = microtouch3m_device_query_controller_id (dev,
                                                       &controller_type,
                                                       &firmware_major,
                                                       &firmware_minor,
                                                       &features,
                                                       &constants_checksum,
                                                       &max_param_write,
                                                       &pc_checksum,
                                                       &asic_type)) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't query controller id: %s\n", microtouch3m_status_to_string (st));
        goto out;
    }

    printf ("controller id:\n");
    printf ("\treport id:          0x%02x\n", controller_type);
    printf ("\tfirmware major:     0x%02x\n", firmware_major);
    printf ("\tfirmware minor:     0x%02x\n", firmware_minor);
    printf ("\tfeatures:           0x%02x\n", features);
    printf ("\tconstants checksum: 0x%04x\n", constants_checksum);
    printf ("\tmax param write:    0x%04x\n", max_param_write);
    printf ("\tpc checksum:        0x%08x\n", pc_checksum);
    printf ("\tasic type:          0x%04x\n", asic_type);

    ret = EXIT_SUCCESS;

out:
    if (dev)
        microtouch3m_device_unref (dev);
    return ret;
}

/******************************************************************************/
/* ACTION: firmware dump */

static int
run_firmware_dump (microtouch3m_context_t *ctx,
                   bool                    first,
                   uint8_t                 bus_number,
                   uint8_t                 device_address,
                   const char             *path)
{
    uint8_t                buffer[MICROTOUCH3M_FW_IMAGE_SIZE];
    microtouch3m_status_t  st;
    microtouch3m_device_t *dev;
    int                    ret = EXIT_FAILURE;

    if (!(dev = create_device (ctx, first, bus_number, device_address, NULL, 0)))
        goto out;

    microtouch3m_device_firmware_progress_register (dev, firmware_progress, 1.0, NULL);
    if ((st = microtouch3m_device_firmware_dump (dev, buffer, sizeof (buffer))) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't dump device firmware: %s\n", microtouch3m_status_to_string (st));
        goto out;
    }
    printf ("\n");

    if ((st = microtouch3m_firmware_file_write (path, buffer, sizeof (buffer))) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't write firmware to file: %s\n", microtouch3m_status_to_string (st));
        goto out;
    }

    printf ("successfully dumped device firmware\n");
    ret = EXIT_SUCCESS;

out:
    if (dev)
        microtouch3m_device_unref (dev);

    return ret;
}

/******************************************************************************/
/* ACTION: firmware update */

#define REBOOT_WAIT_CHECK_RETRIES      30
#define REBOOT_WAIT_CHECK_TIMEOUT_SECS  2

static int
run_firmware_update (microtouch3m_context_t *ctx,
                     bool                    first,
                     uint8_t                 bus_number,
                     uint8_t                 device_address,
                     const char             *path)
{
    microtouch3m_status_t       st;
    uint8_t                     buffer[MICROTOUCH3M_FW_IMAGE_SIZE];
    microtouch3m_device_data_t *dev_data = NULL;
    microtouch3m_device_t      *dev;
    int                         ret = EXIT_FAILURE;
    uint8_t                     real_bus_number;
    uint8_t                     real_device_address;
    uint8_t                     port_numbers[MAX_PORT_NUMBERS];
    int                         port_numbers_len;
    int                         reboot_wait_check_retries;

    if (!(dev = create_device (ctx, first, bus_number, device_address, NULL, 0)))
        goto out;

    port_numbers_len = microtouch3m_device_get_usb_location (dev, port_numbers, MAX_PORT_NUMBERS);

    if ((st = microtouch3m_firmware_file_read (path, buffer, sizeof (buffer))) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't load firmware file: %s\n", microtouch3m_status_to_string (st));
        goto out;
    }

    printf ("backing up device data...\n");
    if ((st = microtouch3m_device_backup_data (dev, &dev_data)) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't backup device data: %s\n", microtouch3m_status_to_string (st));
        goto out;
    }

    printf ("downloading firmware to device EEPROM...\n");
    microtouch3m_device_firmware_progress_register (dev, firmware_progress, 1.0, NULL);
    if ((st = microtouch3m_device_firmware_update (dev, buffer, sizeof (buffer))) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't download firmware to device EEPROM: %s\n", microtouch3m_status_to_string (st));
        goto out;
    }
    printf ("\n");

    /* The device will change address once rebooted, so we'll monitor that as
     * well to make sure we don't try to use the device before it's rebooted. */
    real_bus_number     = microtouch3m_device_get_usb_bus_number (dev);
    real_device_address = microtouch3m_device_get_usb_device_address (dev);

    printf ("rebooting controller...\n");
    if ((st = microtouch3m_device_reset (dev, MICROTOUCH3M_DEVICE_RESET_REBOOT)) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't reboot controller: %s\n", microtouch3m_status_to_string (st));
        goto out;
    }

    /* Forget about the device */
    microtouch3m_device_unref (dev);
    dev = NULL;

    /* Note: using libusb to wait for the device asynchronously may be more efficient, but really not a big
     * deal as this operation isn't something you'd be doing often. */
    for (reboot_wait_check_retries = 0; reboot_wait_check_retries < REBOOT_WAIT_CHECK_RETRIES; reboot_wait_check_retries++) {
        printf ("[%d/%d] waiting for controller reboot...\n", reboot_wait_check_retries + 1, REBOOT_WAIT_CHECK_RETRIES);
        sleep (REBOOT_WAIT_CHECK_TIMEOUT_SECS);

        dev = create_device (ctx, false, real_bus_number, 0, port_numbers, port_numbers_len);

        /* No device found? */
        if (!dev)
            continue;
        /* Device didn't reboot yet? */
        if (real_device_address == microtouch3m_device_get_usb_device_address (dev))
            continue;
        break;
    }

    if (!dev) {
        fprintf (stderr, "error: controller didn't reboot correctly\n");
        goto out;
    }

    printf ("restoring device data...\n");
    if ((st = microtouch3m_device_restore_data (dev, dev_data)) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't restore device data: %s\n", microtouch3m_status_to_string (st));
        goto out;
    }

    printf ("successfully updated device firmware\n");
    ret = EXIT_SUCCESS;

out:
    if (dev)
        microtouch3m_device_unref (dev);
    free (dev_data);
    return ret;
}

/******************************************************************************/
/* Logging */

static pthread_t main_tid;

static void
log_handler (pthread_t   thread_id,
             const char *message)
{
    flockfile (stdout);
    if (thread_id == main_tid)
        fprintf (stdout, "[microtouch3m] %s\n", message);
    else
        fprintf (stdout, "[microtouch3m,%u] %s\n", (unsigned int) thread_id, message);
    funlockfile (stdout);
}

/******************************************************************************/

static void
print_help (void)
{
    printf ("\n"
            "Usage: " PROGRAM_NAME " <option>\n"
            "\n"
            "Generic device selection options\n"
            "  -s, --bus-dev=[BUS]:[DEV]        Select device by bus and/or device number.\n"
            "  -f, --first                      Select first device found.\n"
            "\n"
            "Device actions:\n"
            "  -i, --info                       Show device information.\n"
            "  -x, --firmware-dump=[PATH]       Dump firmware to a file.\n"
            "  -u, --firmware-update=[PATH]     Update firmware in the device.\n"
            "\n"
            "Firmware actions:\n"
            "  -z, --validate-fw-file=[PATH]    Validate firmware file.\n"
            "\n"
            "Common options:\n"
            "  -d, --debug                      Enable verbose logging.\n"
            "  -h, --help                       Show help.\n"
            "  -v, --version                    Show version.\n"
            "\n");
}

static void
print_version (void)
{
    printf ("\n"
            PROGRAM_NAME " " PROGRAM_VERSION "\n"
            "Copyright (2017) Zodiac Inflight Innovations\n"
            "\n");
}

static bool
parse_bus_number_device_address (char    *str,
                                 uint8_t *bus_number,
                                 uint8_t *device_address)
{
    unsigned long  aux;
    char          *bustmp;
    char          *devtmp;

    assert (bus_number);
    assert (device_address);

    devtmp = strchr (str, ':');
    if (devtmp) {
        bustmp = str;
        *devtmp = '\0';
        devtmp++;
    } else {
        bustmp = NULL;
        devtmp = str;
    }

    aux = strtoul (devtmp, NULL, 10);
    if (!aux || aux > 0xFF) {
        fprintf (stderr, "error: invalid DEV value given: '%s'\n", devtmp);
        return false;
    }
    *device_address = (uint8_t) aux;

    if (bustmp) {
        aux = strtoul (bustmp, NULL, 10);
        if (!aux || aux > 0xFF) {
            fprintf (stderr, "error: invalid BUS value given: '%s'\n", bustmp);
            return false;
        }
        *bus_number = (uint8_t) aux;
    } else
        *bus_number = 0;

    return true;
}

int main (int argc, char **argv)
{
    unsigned int            n_actions;
    unsigned int            n_actions_require_device;
    microtouch3m_context_t *ctx                       = NULL;
    int                     idx, iarg                 = 0;
    char                   *bus_number_device_address = NULL;
    uint8_t                 bus_number                = 0;
    uint8_t                 device_address            = 0;
    bool                    first                     = false;
    bool                    info                      = false;
    char                   *firmware_dump             = NULL;
    char                   *firmware_update           = NULL;
    char                   *validate_fw_file          = NULL;
    bool                    debug                     = false;
    int                     ret                       = EXIT_FAILURE;

    const struct option longopts[] = {
        { "bus-dev",          required_argument, 0, 's' },
        { "first",            no_argument,       0, 'f' },
        { "info",             no_argument,       0, 'i' },
        { "firmware-dump",    required_argument, 0, 'x' },
        { "firmware-update",  required_argument, 0, 'u' },
        { "validate-fw-file", required_argument, 0, 'z' },
        { "debug",            no_argument,       0, 'd' },
        { "version",          no_argument,       0, 'v' },
        { "help",             no_argument,       0, 'h' },
        { 0,                  0,                 0, 0   },
    };

    /* turn off getopt error message */
    opterr = 1;
    while (iarg != -1) {
        iarg = getopt_long (argc, argv, "s:fix:u:z:dhv", longopts, &idx);
        switch (iarg) {
        case 's':
            bus_number_device_address = strdup (optarg);
            break;
        case 'f':
            first = true;
            break;
        case 'i':
            info = true;
            break;
        case 'x':
            firmware_dump = strdup (optarg);
            break;
        case 'u':
            firmware_update = strdup (optarg);
            break;
        case 'z':
            validate_fw_file = strdup (optarg);
            break;
        case 'd':
            debug = true;
            break;
        case 'h':
            print_help ();
            return 0;
        case 'v':
            print_version ();
            return 0;
        }
    }

    /* Track actions */
    n_actions_require_device = info + !!firmware_dump + !!firmware_update;
    n_actions = !!(validate_fw_file) + n_actions_require_device;

    if (n_actions > 1) {
        fprintf (stderr, "error: too many actions requested\n");
        return EXIT_FAILURE;
    }
    if (n_actions == 0) {
        fprintf (stderr, "error: no actions requested\n");
        return EXIT_FAILURE;
    }

    /* Setup library logging */
    if (debug) {
        main_tid = pthread_self ();
        microtouch3m_log_set_handler (log_handler);
        disable_progress = true;
    }

    /* Initialize library */
    ctx = microtouch3m_context_new ();
    if (!ctx) {
        fprintf (stderr, "error: libmicrotouch3m initialization failed\n");
        goto out;
    }

    /* Action requires a valid device */
    if (n_actions_require_device) {
        if (!first && !bus_number_device_address) {
            fprintf (stderr, "error: no device selection options specified\n");
            goto out;
        }
        if (bus_number_device_address && !parse_bus_number_device_address (bus_number_device_address, &bus_number, &device_address)) {
            fprintf (stderr, "error: invalid --bus-dev option given\n");
            goto out;
        }
    }

    /* Run actions */
    if (validate_fw_file)
        ret = run_validate_fw_file (validate_fw_file);
    else if (info)
        ret = run_info (ctx, first, bus_number, device_address);
    else if (firmware_dump)
        ret = run_firmware_dump (ctx, first, bus_number, device_address, firmware_dump);
    else if (firmware_update)
        ret = run_firmware_update (ctx, first, bus_number, device_address, firmware_update);
    else
        assert (0);

out:
    microtouch3m_context_unref (ctx);

    free (bus_number_device_address);
    free (firmware_dump);
    free (firmware_update);
    free (validate_fw_file);
    return ret;
}
