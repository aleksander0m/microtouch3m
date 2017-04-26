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
            "Common options:\n"
            "  -d, --debug          Enable verbose logging.\n"
            "  -h, --help           Show help.\n"
            "  -v, --version        Show version.\n"
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
    int          idx, iarg = 0;
    unsigned int n_actions;
    bool         debug = false;
    unsigned int ret = 0;

    const struct option longopts[] = {
        { "debug",   no_argument, 0, 'd' },
        { "version", no_argument, 0, 'v' },
        { "help",    no_argument, 0, 'h' },
        { 0,         0,           0, 0   },
    };

    /* turn off getopt error message */
    opterr = 1;
    while (iarg != -1) {
        iarg = getopt_long (argc, argv, "dhv", longopts, &idx);
        switch (iarg) {
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

    /* TODO: track actions */
    n_actions = 0;

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

    /* TODO: run actions */

    return ret;
}
