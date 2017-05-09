/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2017 Zodiac Inflight Innovations, Inc.
 * All rights reserved.
 *
 * Author: Aleksander Morgado <aleksander@aleksander.es>
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
