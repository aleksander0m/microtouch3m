/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * All rights reserved.
 *
 * Author: Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef MICROTOUCH3M_H
#define MICROTOUCH3M_H

#include <pthread.h>

/******************************************************************************/
/* Status */

typedef enum {
    MICROTOUCH3M_STATUS_OK,
    MICROTOUCH3M_STATUS_FAILED,
} microtouch3m_status_t;

const char *microtouch3m_status_to_string (microtouch3m_status_t st);

/******************************************************************************/
/* Logging */

/**
 * microtouch3m_log_handler_t:
 * @thread_id: thread ID where the log message was generated.
 * @message: the log message.

 * Logging handler.
 */
typedef void (* microtouch3m_log_handler_t) (pthread_t   thread_id,
                                             const char *message);

/**
 * microtouch3m_log_set_handler:
 *
 * Set logging handler.
 *
 * This method is NOT thread-safe, and would be usually called once before
 * any other operation with the library.
 */
void microtouch3m_log_set_handler (microtouch3m_log_handler_t handler);

#endif /* MICROTOUCH3M_H */
