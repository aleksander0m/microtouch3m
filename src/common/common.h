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

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <sys/types.h>

char *strhex (const void *mem,
              size_t      size,
              const char *delimiter);

char *strhex_multiline (const void *mem,
                        size_t      size,
                        size_t      max_bytes_per_line,
                        const char *line_prefix,
                        const char *delimiter);

ssize_t strbin (const char *str,
                uint8_t    *buffer,
                size_t      buffer_size);

char *strascii (const void *mem,
                size_t      size);

char *str_usb_location (uint8_t        bus,
                        const uint8_t *port_numbers,
                        int            port_numbers_len);

#endif /* COMMON_H */
