/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * libmicrotouch3m - MicroTouch 3M touchscreen control library
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2017 Zodiac Inflight Innovations
 * Copyright (C) 2017 Aleksander Morgado <aleksander@aleksander.es>
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
