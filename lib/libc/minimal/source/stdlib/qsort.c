/****************************************************************************
 * libc/stdlib/lib_qsort.c
 *
 *   Copyright (C) 2007, 2009, 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Leveraged from:
 *
 *  Copyright (c) 1992, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the University of
 *    California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

//#include <nuttx/config.h>

#include <sys/types.h>
#include <stdlib.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define min(a, b)  (a) < (b) ? a : b

#define swapcode(TYPE, parmi, parmj, n) \
  { \
    long i = (n) / sizeof (TYPE); \
    register TYPE *pi = (TYPE *) (parmi); \
    register TYPE *pj = (TYPE *) (parmj); \
    do { \
      register TYPE  t = *pi; \
      *pi++ = *pj; \
      *pj++ = t; \
    } while (--i > 0); \
  }

#define SWAPINIT(a, size) \
  swaptype = ((char *)a - (char *)0) % sizeof(long) || \
  size % sizeof(long) ? 2 : size == sizeof(long)? 0 : 1;

#define swap(a, b) \
  if (swaptype == 0) \
    { \
      long t = *(long *)(a); \
      *(long *)(a) = *(long *)(b); \
      *(long *)(b) = t; \
    } \
  else \
    { \
      swapfunc(a, b, size, swaptype); \
    }

#define vecswap(a, b, n) if ((n) > 0) swapfunc(a, b, n, swaptype)

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static inline void swapfunc(char *a, char *b, int n, int swaptype);
static inline char *med3(char *a, char *b, char *c,
                         int (*compar)(const void *, const void *));

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void swapfunc(char *a, char *b, int n, int swaptype)
{
  if (swaptype <= 1)
    {
      swapcode(long, a, b, n)
    }
  else
    {
      swapcode(char, a, b, n)
    }
}

static inline char *med3(char *a, char *b, char *c,
                         int (*compar)(const void *, const void *))
{
  return compar(a, b) < 0 ?
         (compar(b, c) < 0 ? b : (compar(a, c) < 0 ? c : a ))
              :(compar(b, c) > 0 ? b : (compar(a, c) < 0 ? a : c ));
}

/****************************************************************************
 * Public Function
 ****************************************************************************/

/****************************************************************************
 * Name: qsort
 *
 * Description:
 *   Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".
 *
 ****************************************************************************/

void qsort(void *base, size_t nmemb, size_t size,
           int(*compar)(const void *, const void *))
{
  char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
  int d, r, swaptype, swap_cnt;

loop:
  SWAPINIT(base, size);
  swap_cnt = 0;
  if (nmemb < 7)
    {
      for (pm = (char *) base + size; pm < (char *) base + nmemb * size; pm += size)
        {
          for (pl = pm; pl > (char *) base && compar(pl - size, pl) > 0; pl -= size)
            {
              swap(pl, pl - size);
            }
        }
      return;
  }

  pm = (char *) base + (nmemb / 2) * size;
  if (nmemb > 7)
    {
      pl = base;
      pn = (char *) base + (nmemb - 1) * size;
      if (nmemb > 40)
        {
          d = (nmemb / 8) * size;
          pl = med3(pl, pl + d, pl + 2 * d, compar);
          pm = med3(pm - d, pm, pm + d, compar);
          pn = med3(pn - 2 * d, pn - d, pn, compar);
        }
      pm = med3(pl, pm, pn, compar);
    }
  swap(base, pm);
  pa = pb = (char *) base + size;

  pc = pd = (char *) base + (nmemb - 1) * size;
  for (;;)
    {
      while (pb <= pc && (r = compar(pb, base)) <= 0)
        {
          if (r == 0)
            {
              swap_cnt = 1;
              swap(pa, pb);
              pa += size;
            }
          pb += size;
        }
      while (pb <= pc && (r = compar(pc, base)) >= 0)
        {
          if (r == 0)
            {
              swap_cnt = 1;
              swap(pc, pd);
              pd -= size;
            }
          pc -= size;
        }

      if (pb > pc)
        {
          break;
        }

      swap(pb, pc);
      swap_cnt = 1;
      pb += size;
      pc -= size;
    }

  if (swap_cnt == 0)
    {
      /* Switch to insertion sort */

      for (pm = (char *) base + size; pm < (char *) base + nmemb * size; pm += size)
        {
          for (pl = pm; pl > (char *) base && compar(pl - size, pl) > 0; pl -= size)
            {
              swap(pl, pl - size);
            }
        }
      return;
    }

  pn = (char *) base + nmemb * size;
  r = min(pa - (char *)base, pb - pa);
  vecswap(base, pb - r, r);
  r = min(pd - pc, pn - pd - size);
  vecswap(pb, pn - r, r);

  if ((r = pb - pa) > size)
    {
      qsort(base, r / size, size, compar);
    }

  if ((r = pd - pc) > size)
    {
      /* Iterate rather than recurse to save stack space */
      base = pn - r;
      nmemb = r / size;
      goto loop;
    }
}
