/* x86 implementation of k_memset, k_memcpy, etc for other toolchains */

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

/* Diab */
#if defined(__DCC__)
__asm volatile void k_memcpy(void *_d, const void *_s, size_t_n)
{
#ifndef CONFIG_UNALIGNED_WRITE_UNSUPPORTED
% mem _d, _s, _n; lab unaligned, done
! "si", "di", "cx"
	movl	_d, %edi
	movl	_s, %esi
	movl	_n, %ecx
	testl	$3, %ecx
	jnz	 unaligned
	shrl	$2, %ecx
	rep	movsl
	jmp	 done
unaligned:
	rep	movsb
done:
#else  /* CONFIG_UNALIGNED_WRITE_UNSUPPORTED */
% mem _d, _s, _n
! "si", "di", "cx"
	movl	_d, %edi
	movl	_s, %esi
	movl	_n, %ecx
	rep	movsb
#endif /* CONFIG_UNALIGNED_WRITE_UNSUPPORTED */
}

__asm volatile void k_memset(void	*_d, int	_v, size_t	_n)
{
#ifndef CONFIG_UNALIGNED_WRITE_UNSUPPORTED
% mem _d, _v, _n; lab unaligned, done
! "ax", "di", "cx", "dx"
	movl	_d, %edi
	movl	_v, %eax
	movl	_n, %ecx
	andl	$0xff, %eax
	testl	$3, %ecx
	movb	%al, %ah
	jnz	 unaligned
	movl	%eax, %edx
	shll	$16, %eax
	shrl	$2, %ecx
	orl	%edx, %eax
	rep	stosl
	jmp	 done
unaligned:
	rep	stosb
done:
#else  /* CONFIG_UNALIGNED_WRITE_UNSUPPORTED */
% mem _d, _v, _n
! "ax", "di", "cx"
	movl	_d, %edi
	movl	_v, %eax
	movl	_n, %ecx
	rep	stosb
#endif /* CONFIG_UNALIGNED_WRITE_UNSUPPORTED */
}
#endif /* __DCC__ */
