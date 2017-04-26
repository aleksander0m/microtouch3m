/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * All rights reserved.
 *
 * Author: Aleksander Morgado <aleksander@aleksander.es>
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

#define microtouch3m_log(...) microtouch3m_log_full (pthread_self (), ## __VA_ARGS__ )
#define microtouch3m_log_raw(prefix, mem,size) microtouch3m_log_raw_full (pthread_self (), prefix, mem, size)

bool microtouch3m_log_is_enabled (void);

#endif /* MICROTOUCH3M_LOG_H */
