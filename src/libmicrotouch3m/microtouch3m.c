/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * All rights reserved.
 *
 * Author: Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>

#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>

#include "common.h"

#include "microtouch3m.h"
#include "microtouch3m-log.h"

/******************************************************************************/
/* Status */

static const char *status_str[] = {
    [MICROTOUCH3M_STATUS_OK]     = "success",
    [MICROTOUCH3M_STATUS_FAILED] = "failed",
};

const char *
microtouch3m_status_to_string (microtouch3m_status_t st)
{
    return ((st < (sizeof (status_str) / sizeof (status_str[0]))) ? status_str[st] : "unknown");
}
