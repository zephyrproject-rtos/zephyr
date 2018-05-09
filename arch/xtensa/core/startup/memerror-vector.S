/* memerror-vector.S  --  Memory Error Exception Vector and Handler */

/* $Id: //depot/rel/Foxhill/dot.5/Xtensa/OS/xtos/memerror-vector.S#1 $ */

/*
 * Copyright (c) 2006-2013 Tensilica Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>

// This file just contains this one symbol, used by the reset code.
// It is here rather than in reset-vector.S because we want the symbol
// to be external, so resolution is delayed until link time.
//
// To define your own value to override this default, redefine the
// symbol _MemErrorHandler to the desired value, e.g. -
//
//    xt-xcc test.c -g -o test -Wl,--defsym=_MemErrorHandler=0x08080808
//

	.global	_MemErrorHandler
	.weak   _MemErrorHandler
	.equ    _MemErrorHandler, XCHAL_CACHE_MEMCTL_DEFAULT

