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

#if !defined MICROTOUCH3M_LOG_H
# define MICROTOUCH3M_LOG_H

#include <stdbool.h>

/******************************************************************************/
/* Logging */

void microtouch3m_log_full     (pthread_t   thread_id,
                                const char *fmt,
                                ...);
void microtouch3m_log_raw_full (pthread_t   thread_id,
                                const char *prefix,
                                const void *mem,
                                size_t      size);

void microtouch3m_log_buffer   (const char    *name,
                                const uint8_t *buffer,
                                size_t         buffer_size);

#define microtouch3m_log(...) microtouch3m_log_full (pthread_self (), ## __VA_ARGS__ )
#define microtouch3m_log_raw(prefix, mem,size) microtouch3m_log_raw_full (pthread_self (), prefix, mem, size)

bool microtouch3m_log_is_enabled (void);

#endif /* MICROTOUCH3M_LOG_H */
