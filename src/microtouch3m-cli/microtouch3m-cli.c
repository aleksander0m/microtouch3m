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

#include <common.h>

#include <microtouch3m.h>

#define PROGRAM_NAME    "microtouch3m-cli"
#define PROGRAM_VERSION PACKAGE_VERSION

/******************************************************************************/
/* Helper: create device based on bus (or first found) */

static microtouch3m_device_t *
create_device_by_usb_bus (microtouch3m_context_t *ctx,
                          uint8_t                 bus_number,
                          uint8_t                 device_address)
{
    microtouch3m_device_t *dev;
    microtouch3m_status_t  st;

    dev = microtouch3m_device_new_by_usb_bus (ctx, bus_number, device_address);
    if (!dev) {
        if (bus_number && device_address)
            fprintf (stderr, "error: couldn't find a microtouch 3m device at %03u:%03u\n", bus_number, device_address);
        else if (device_address)
            fprintf (stderr, "error: couldn't find a microtouch 3m device at address %03u in any USB bus\n", device_address);
        else
            fprintf (stderr, "error: couldn't find any microtouch 3m device\n");
        return NULL;
    }

    printf ("microtouch 3m device found at %03u:%03u\n",
            microtouch3m_device_get_usb_bus_number (dev),
            microtouch3m_device_get_usb_device_address (dev));

    if ((st = microtouch3m_device_open (dev)) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't open microtouch 3m device: %s\n", microtouch3m_status_to_string (st));
        microtouch3m_device_unref (dev);
        return NULL;
    }

    return dev;
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
          uint8_t                 bus_number,
          uint8_t                 device_address)
{
    microtouch3m_device_t *dev;

    if (!(dev = create_device_by_usb_bus (ctx, bus_number, device_address)))
        return EXIT_FAILURE;

    microtouch3m_device_close (dev);
    microtouch3m_device_unref (dev);
    return EXIT_SUCCESS;
}

/******************************************************************************/
/* ACTION: firmware dump */

static int
run_firmware_dump (microtouch3m_context_t *ctx,
                   uint8_t                 bus_number,
                   uint8_t                 device_address,
                   const char             *path)
{
    uint8_t                buffer[MICROTOUCH3M_FW_IMAGE_SIZE];
    microtouch3m_status_t  st;
    microtouch3m_device_t *dev;
    int                    ret = EXIT_FAILURE;

    if (!(dev = create_device_by_usb_bus (ctx, bus_number, device_address)))
        goto out;

    if ((st = microtouch3m_device_firmware_dump (dev, buffer, sizeof (buffer))) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't dump device firmware: %s\n", microtouch3m_status_to_string (st));
        goto out;
    }

    if ((st = microtouch3m_firmware_file_write (path, buffer, sizeof (buffer))) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't write firmware to file: %s\n", microtouch3m_status_to_string (st));
        goto out;
    }

    printf ("successfully dumped device firmware\n");
    ret = EXIT_SUCCESS;

out:
    if (dev) {
        microtouch3m_device_close (dev);
        microtouch3m_device_unref (dev);
    }

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
    char                   *validate_fw_file          = NULL;
    bool                    debug                     = false;
    int                     ret                       = EXIT_FAILURE;

    const struct option longopts[] = {
        { "bus-dev",          required_argument, 0, 's' },
        { "first",            no_argument,       0, 'f' },
        { "info",             no_argument,       0, 'i' },
        { "firmware-dump",    required_argument, 0, 'x' },
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
    n_actions_require_device = info + !!firmware_dump;
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
        ret = run_info (ctx, bus_number, device_address);
    else if (firmware_dump)
        ret = run_firmware_dump (ctx, bus_number, device_address, firmware_dump);
    else
        assert (0);

out:
    microtouch3m_context_unref (ctx);

    free (bus_number_device_address);
    free (firmware_dump);
    free (validate_fw_file);
    return ret;
}
