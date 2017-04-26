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

    if ((st = microtouch3m_firmware_file_load (path, NULL, 0)) != MICROTOUCH3M_STATUS_OK) {
        fprintf (stderr, "error: couldn't validate firmware file: %s\n", microtouch3m_status_to_string (st));
        return EXIT_FAILURE;
    }

    printf ("successfully validated firmware file\n");
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
            "Firmware options:\n"
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

int main (int argc, char **argv)
{
    int           idx, iarg = 0;
    unsigned int  n_actions;
    char         *validate_fw_file = NULL;
    bool          debug = false;
    unsigned int  ret = 0;

    const struct option longopts[] = {
        { "validate-fw-file", required_argument, 0, 'z' },
        { "debug",            no_argument,       0, 'd' },
        { "version",          no_argument,       0, 'v' },
        { "help",             no_argument,       0, 'h' },
        { 0,                  0,                 0, 0   },
    };

    /* turn off getopt error message */
    opterr = 1;
    while (iarg != -1) {
        iarg = getopt_long (argc, argv, "z:dhv", longopts, &idx);
        switch (iarg) {
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
    n_actions = !!(validate_fw_file);

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

    /* Run actions */
    if (validate_fw_file)
        ret = run_validate_fw_file (validate_fw_file);


    free (validate_fw_file);
    return ret;
}
