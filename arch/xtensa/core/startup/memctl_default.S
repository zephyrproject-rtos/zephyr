// memctl_default.S  - Default startup value for MEMCTL register.

// Copyright (c) 1998-2015 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include <xtensa/coreasm.h>
#include <xtensa/config/system.h>


// This file just contains this one symbol, used by the reset code.
// It is here rather than in reset-vector.S because we want the symbol
// to be external, so resolution is delayed until link time.
//
// To define your own value to override this default, redefine the
// symbol __memctl_default to the desired value, e.g. -
//
//    xt-xcc test.c -g -o test -Wl,--defsym=__memctl_default=0x08080808
//

	.global	__memctl_default
	.weak   __memctl_default
	.equ    __memctl_default, XCHAL_CACHE_MEMCTL_DEFAULT

