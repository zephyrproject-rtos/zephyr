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
 * @file dtls_time.c
 * @brief Clock Handling
 */

#include "tinydtls.h"
#include "dtls_config.h"
#include "dtls_time.h"

#ifdef WITH_CONTIKI
clock_time_t dtls_clock_offset;

void
dtls_clock_init(void) {
  dtls_clock_offset = clock_time();
}

void
dtls_ticks(dtls_tick_t *t) {
  *t = clock_time();
}

#else /* WITH_CONTIKI */

time_t dtls_clock_offset;

void
dtls_clock_init(void) {
#ifdef HAVE_TIME_H
  dtls_clock_offset = time(NULL);
#else
#  ifdef __GNUC__
  /* Issue a warning when using gcc. Other prepropressors do 
   *  not seem to have a similar feature. */ 
#   warning "cannot initialize clock"
#  endif
  dtls_clock_offset = 0;
#endif
}

void dtls_ticks(dtls_tick_t *t) {
#ifdef HAVE_SYS_TIME_H
  struct timeval tv;
  gettimeofday(&tv, NULL);
  *t = (tv.tv_sec - dtls_clock_offset) * DTLS_TICKS_PER_SECOND 
    + (tv.tv_usec * DTLS_TICKS_PER_SECOND / 1000000);
#else
#error "clock not implemented"
#endif
}

#endif /* WITH_CONTIKI */


