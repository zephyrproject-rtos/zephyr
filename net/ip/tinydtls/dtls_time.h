/* dtls -- a very basic DTLS implementation
 *
 * Copyright (C) 2011--2013 Olaf Bergmann <bergmann@tzi.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file dtls_time.h
 * @brief Clock Handling
 */

#ifndef _DTLS_DTLS_TIME_H_
#define _DTLS_DTLS_TIME_H_

#include <stdint.h>
#include <sys/time.h>

#include "tinydtls.h"

/**
 * @defgroup clock Clock Handling
 * Default implementation of internal clock. You should redefine this if
 * you do not have time() and gettimeofday().
 * @{
 */

#ifdef WITH_CONTIKI
#include "clock.h"
#else /* WITH_CONTIKI */
#include <time.h>

#ifndef CLOCK_SECOND
# define CLOCK_SECOND 1000
#endif

typedef uint32_t clock_time_t;
#endif /* WITH_CONTIKI */

typedef clock_time_t dtls_tick_t;

#ifndef DTLS_TICKS_PER_SECOND
#define DTLS_TICKS_PER_SECOND CLOCK_SECOND
#endif /* DTLS_TICKS_PER_SECOND */

void dtls_clock_init(void);
void dtls_ticks(dtls_tick_t *t);

/** @} */

#endif /* _DTLS_DTLS_TIME_H_ */
