/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * All rights reserved.
 *
 * Author: Aleksander Morgado <aleksander@aleksander.es>
 */

#define _GNU_SOURCE
#include <malloc.h>
#include <stdarg.h>
#include <stdint.h>

#include <common.h>

#include "microtouch3m.h"
#include "microtouch3m-log.h"

/******************************************************************************/
/* Logging */

static microtouch3m_log_handler_t default_handler;

bool
microtouch3m_log_is_enabled (void)
{
    return !!default_handler;
}

void
microtouch3m_log_set_handler (microtouch3m_log_handler_t handler)
{
    default_handler = handler;
}

void
microtouch3m_log_full (pthread_t   thread_id,
                       const char *fmt,
                       ...)
{
    char    *message;
    va_list  args;

    if (!default_handler)
        return;

    va_start (args, fmt);
    if (vasprintf (&message, fmt, args) == -1)
        return;
    va_end (args);

    default_handler (thread_id, message);

    free (message);
}

void
microtouch3m_log_raw_full (pthread_t   thread_id,
                           const char *prefix,
                           const void *mem,
                           size_t      size)
{
    char *memstr;

    if (!default_handler || !mem || !size)
        return;

    memstr = strhex (mem, size, ":");
    if (!memstr)
        return;

    microtouch3m_log_full (thread_id, "%s (%zu bytes) %s", prefix, size, memstr);
    free (memstr);
}

void
microtouch3m_log_buffer (const char    *name,
                         const uint8_t *buffer,
                         size_t         buffer_size)
{
    char *hex;

    if (!default_handler)
        return;

    hex = strhex (buffer, buffer_size, ":");
    microtouch3m_log ("%s (%d bytes): %s", name, buffer_size, hex);
    free (hex);
}
