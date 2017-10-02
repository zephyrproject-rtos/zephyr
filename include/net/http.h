/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HTTP_H__
#define __HTTP_H__

#if defined(CONFIG_HTTP_APP)
#include <net/http_app.h>
#else
#include <net/http_legacy.h>
#endif

#endif /* __HTTP_H__ */
