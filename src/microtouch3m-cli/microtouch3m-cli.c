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
run_info (microtouch3m_device_t *dev)
{
    return EXIT_SUCCESS;
}

/******************************************************************************/
/* ACTION: firmware dump */

static int
run_firmware_dump (microtouch3m_device_t *dev,
                   const char            *path)
{
    microtouch3m_status_t st;
    uint8_t buffer[MICROTOUCH3M_FW_IMAGE_SIZE];

    if ((st = microtouch3m_device_firmware_dump (dev, buffer, sizeof (buffer))) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't dump device firmware: %s\n", microtouch3m_status_to_string (st));
        return EXIT_FAILURE;
    }

    if ((st = microtouch3m_firmware_file_write (path, buffer, sizeof (buffer))) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't write firmware to file: %s\n", microtouch3m_status_to_string (st));
        return EXIT_FAILURE;
    }

    printf ("successfully dumped device firmware\n");
    return EXIT_SUCCESS;
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
            "  -s, --busnum-devnum=[BUS:]DEV    Select device by bus and device number.\n"
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
parse_busnum_devnum (char    *str,
                     uint8_t *busnum,
                     uint8_t *devnum)
{
    unsigned long  aux;
    char          *bustmp;
    char          *devtmp;

    assert (busnum);
    assert (devnum);

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
    *devnum = (uint8_t) aux;

    if (bustmp) {
        aux = strtoul (bustmp, NULL, 10);
        if (!aux || aux > 0xFF) {
            fprintf (stderr, "error: invalid BUS value given: '%s'\n", bustmp);
            return false;
        }
        *busnum = (uint8_t) aux;
    } else
        *busnum = 0;

    return true;
}

int main (int argc, char **argv)
{
    unsigned int            n_actions;
    unsigned int            n_actions_require_device;
    microtouch3m_context_t *ctx              = NULL;
    microtouch3m_device_t  *dev              = NULL;
    int                     idx, iarg        = 0;
    char                   *busnum_devnum    = NULL;
    bool                    first            = false;
    bool                    info             = false;
    char                   *firmware_dump    = NULL;
    char                   *validate_fw_file = NULL;
    bool                    debug            = false;
    int                     ret              = EXIT_FAILURE;

    const struct option longopts[] = {
        { "busnum-devnum",    required_argument, 0, 's' },
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
        iarg = getopt_long (argc, argv, "s:fix:z:dhv", longopts, &idx);
        switch (iarg) {
        case 's':
            busnum_devnum = strdup (optarg);
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
    n_actions = !!(validate_fw_file) + info + !!firmware_dump;
    n_actions_require_device = info + !!firmware_dump;

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
        uint8_t busnum = 0;
        uint8_t devnum = 0;

        if (!first && !busnum_devnum) {
            fprintf (stderr, "error: no device selection options specified\n");
            goto out;
        }

        if (busnum_devnum && !parse_busnum_devnum (busnum_devnum, &busnum, &devnum)) {
            fprintf (stderr, "error: invalid --busnum-devnum option given\n");
            goto out;
        }

        dev = microtouch3m_device_new (ctx, busnum, devnum);
        if (!dev) {
            if (first)
                fprintf (stderr, "error: couldn't find any microtouch 3m device\n");
            else if (busnum)
                fprintf (stderr, "error: couldn't find a microtouch 3m device at %03u:%03u\n", busnum, devnum);
            else if (devnum)
                fprintf (stderr, "error: couldn't find a microtouch 3m device at address %03u in any USB bus\n", devnum);
            else
                assert (0);
            goto out;
        }

        printf ("microtouch 3m device found at %03u:%03u\n",
                microtouch3m_device_get_usb_bus_number (dev),
                microtouch3m_device_get_usb_device_address (dev));
    }

    /* Run actions */
    if (validate_fw_file)
        ret = run_validate_fw_file (validate_fw_file);
    else if (info)
        ret = run_info (dev);
    else if (firmware_dump)
        ret = run_firmware_dump (dev, firmware_dump);
    else
        assert (0);

out:
    if (dev)
        microtouch3m_device_unref (dev);
    microtouch3m_context_unref (ctx);

    free (busnum_devnum);
    free (firmware_dump);
    free (validate_fw_file);
    return ret;
}
