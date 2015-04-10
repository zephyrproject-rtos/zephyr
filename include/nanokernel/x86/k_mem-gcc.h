/* x86 GCC implementation of k_memset, k_memcpy, etc */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

static inline void k_memcpy(void *_d, const void *_s, size_t _n)
{
	/* _d & _s must be aligned to use movsl. */
#ifndef CONFIG_UNALIGNED_WRITE_UNSUPPORTED
	if ((_n&3) == 0) {
	/* _n is multiple of words, much more efficient to do word moves */
	_n >>= 2;
	__asm__ volatile ("rep movsl" :
			  "+D" (_d), "+S" (_s), "+c" (_n) :
			  :
			  "cc", "memory");
	} else
#endif /* CONFIG_UNALIGNED_WRITE_UNSUPPORTED */
	{
	__asm__ volatile ("rep movsb" :
			  "+D" (_d), "+S" (_s), "+c" (_n) :
			  :
			  "cc", "memory");
	}
}

static inline void k_memset(void *_d, int _v, size_t _n)
{
	/* _d must be aligned to use stosl. */
#ifndef CONFIG_UNALIGNED_WRITE_UNSUPPORTED
	if ((_n&3) == 0) {
	/* _n is multiple of words, much more efficient to do word stores */
	_n >>= 2;
	_v |= _v<<8;
	_v |= _v<<16;
	__asm__ volatile ("rep stosl" :
			  "+D" (_d), "+c" (_n) :
			  "a" (_v) :
			  "cc", "memory");
	} else
#endif /* CONFIG_UNALIGNED_WRITE_UNSUPPORTED */
	{
		__asm__ volatile ("rep stosb" :
			  "+D" (_d), "+c" (_n) :
			  "a" (_v) :
			  "cc", "memory");
	}
}
