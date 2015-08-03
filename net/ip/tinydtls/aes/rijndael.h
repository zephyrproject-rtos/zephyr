/*	$OpenBSD: rijndael.h,v 1.13 2008/06/09 07:49:45 djm Exp $ */

/**
 * rijndael-alg-fst.h
 *
 * @version 3.0 (December 2000)
 *
 * Optimised ANSI C code for the Rijndael cipher (now AES)
 *
 * @author Vincent Rijmen <vincent.rijmen@esat.kuleuven.ac.be>
 * @author Antoon Bosselaers <antoon.bosselaers@esat.kuleuven.ac.be>
 * @author Paulo Barreto <paulo.barreto@terra.com.br>
 *
 * This code is hereby placed in the public domain.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __RIJNDAEL_H
#define __RIJNDAEL_H

#include <stdint.h>

#define AES_MAXKEYBITS	(256)
#define AES_MAXKEYBYTES	(AES_MAXKEYBITS/8)
/* for 256-bit keys we need 14 rounds for a 128 we only need 10 round */
#define AES_MAXROUNDS	10

/* bergmann: to avoid conflicts with typedefs from certain Contiki platforms,
 * the following type names have been prefixed with "aes_": */
typedef unsigned char	u_char;
typedef uint8_t		aes_u8;
typedef uint16_t	aes_u16;
typedef uint32_t	aes_u32;

/*  The structure for key information */
typedef struct {
#ifdef WITH_AES_DECRYPT
	int	enc_only;		/* context contains only encrypt schedule */
#endif
	int	Nr;			/* key-length-dependent number of rounds */
	aes_u32	ek[4*(AES_MAXROUNDS + 1)];	/* encrypt key schedule */
#ifdef WITH_AES_DECRYPT
	aes_u32	dk[4*(AES_MAXROUNDS + 1)];	/* decrypt key schedule */
#endif
} rijndael_ctx;

int	 rijndael_set_key(rijndael_ctx *, const u_char *, int);
int	 rijndael_set_key_enc_only(rijndael_ctx *, const u_char *, int);
void	 rijndael_decrypt(rijndael_ctx *, const u_char *, u_char *);
void	 rijndael_encrypt(rijndael_ctx *, const u_char *, u_char *);

int	rijndaelKeySetupEnc(aes_u32 rk[/*4*(Nr + 1)*/], const aes_u8 cipherKey[], int keyBits);
int	rijndaelKeySetupDec(aes_u32 rk[/*4*(Nr + 1)*/], const aes_u8 cipherKey[], int keyBits);
void	rijndaelEncrypt(const aes_u32 rk[/*4*(Nr + 1)*/], int Nr, const aes_u8 pt[16], aes_u8 ct[16]);

#endif /* __RIJNDAEL_H */
