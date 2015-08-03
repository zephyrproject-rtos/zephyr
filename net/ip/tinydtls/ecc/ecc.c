/*
 * Copyright (c) 2009 Chris K Cockrum <ckc@cockrum.net>
 *
 * Copyright (c) 2013 Jens Trillmann <jtrillma@tzi.de>
 * Copyright (c) 2013 Marc Müller-Weinhardt <muewei@tzi.de>
 * Copyright (c) 2013 Lars Schmertmann <lars@tzi.de>
 * Copyright (c) 2013 Hauke Mehrtens <hauke@hauke-m.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *
 * This implementation is based in part on the paper Implementation of an
 * Elliptic Curve Cryptosystem on an 8-bit Microcontroller [0] by
 * Chris K Cockrum <ckc@cockrum.net>.
 *
 * [0]: http://cockrum.net/Implementation_of_ECC_on_an_8-bit_microcontroller.pdf
 *
 * This is a efficient ECC implementation on the secp256r1 curve for 32 Bit CPU
 * architectures. It provides basic operations on the secp256r1 curve and support
 * for ECDH and ECDSA.
 */

//big number functions
#include "ecc.h"
#include <string.h>

static uint32_t add( const uint32_t *x, const uint32_t *y, uint32_t *result, uint8_t length){
	uint64_t d = 0; //carry
	int v = 0;
	for(v = 0;v<length;v++){
		//printf("%02x + %02x + %01x = ", x[v], y[v], d);
		d += (uint64_t) x[v] + (uint64_t) y[v];
		//printf("%02x\n", d);
		result[v] = d;
		d = d>>32; //save carry
	}
	
	return (uint32_t)d;
}

static uint32_t sub( const uint32_t *x, const uint32_t *y, uint32_t *result, uint8_t length){
	uint64_t d = 0;
	int v;
	for(v = 0;v < length; v++){
		d = (uint64_t) x[v] - (uint64_t) y[v] - d;
		result[v] = d & 0xFFFFFFFF;
		d = d>>32;
		d &= 0x1;
	}	
	return (uint32_t)d;
}

static void rshiftby(const uint32_t *in, uint8_t in_size, uint32_t *out, uint8_t out_size, uint8_t shift) {
	int i;

	for (i = 0; i < (in_size - shift) && i < out_size; i++)
		out[i] = in[i + shift];
	for (/* reuse i */; i < out_size; i++)
		out[i] = 0;
}

//finite field functions
//FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF
static const uint32_t ecc_prime_m[8] = {0xffffffff, 0xffffffff, 0xffffffff, 0x00000000,
					0x00000000, 0x00000000, 0x00000001, 0xffffffff};

							
/* This is added after an static byte addition if the answer has a carry in MSB*/
static const uint32_t ecc_prime_r[8] = {0x00000001, 0x00000000, 0x00000000, 0xffffffff,
					0xffffffff, 0xffffffff, 0xfffffffe, 0x00000000};

// ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551
static const uint32_t ecc_order_m[9] = {0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD,
					0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
					0x00000000};

static const uint32_t ecc_order_r[8] = {0x039CDAAF, 0x0C46353D, 0x58E8617B, 0x43190552,
					0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000};

static const uint32_t ecc_order_mu[9] = {0xEEDF9BFE, 0x012FFD85, 0xDF1A6C21, 0x43190552,
					 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF, 0x00000000,
					 0x00000001};

static const uint8_t ecc_order_k = 8;

const uint32_t ecc_g_point_x[8] = { 0xD898C296, 0xF4A13945, 0x2DEB33A0, 0x77037D81,
				    0x63A440F2, 0xF8BCE6E5, 0xE12C4247, 0x6B17D1F2};
const uint32_t ecc_g_point_y[8] = { 0x37BF51F5, 0xCBB64068, 0x6B315ECE, 0x2BCE3357,
				    0x7C0F9E16, 0x8EE7EB4A, 0xFE1A7F9B, 0x4FE342E2};


static void setZero(uint32_t *A, const int length){
	memset(A, 0x0, length * sizeof(uint32_t));
}

/*
 * copy one array to another
 */
static void copy(const uint32_t *from, uint32_t *to, uint8_t length){
	memcpy(to, from, length * sizeof(uint32_t));
}

static int isSame(const uint32_t *A, const uint32_t *B, uint8_t length){
	return !memcmp(A, B, length * sizeof(uint32_t));
}

//is A greater than B?
static int isGreater(const uint32_t *A, const uint32_t *B, uint8_t length){
	int i;
	for (i = length-1; i >= 0; --i)
	{
		if(A[i] > B[i])
			return 1;
		if(A[i] < B[i])
			return -1;
	}
	return 0;
}


static int fieldAdd(const uint32_t *x, const uint32_t *y, const uint32_t *reducer, uint32_t *result){
	if(add(x, y, result, arrayLength)){ //add prime if carry is still set!
		uint32_t tempas[8];
		setZero(tempas, 8);
		add(result, reducer, tempas, arrayLength);
		copy(tempas, result, arrayLength);
	}
	return 0;
}

static int fieldSub(const uint32_t *x, const uint32_t *y, const uint32_t *modulus, uint32_t *result){
	if(sub(x, y, result, arrayLength)){ //add modulus if carry is set
		uint32_t tempas[8];
		setZero(tempas, 8);
		add(result, modulus, tempas, arrayLength);
		copy(tempas, result, arrayLength);
	}
	return 0;
}

//finite Field multiplication
//32bit * 32bit = 64bit
static int fieldMult(const uint32_t *x, const uint32_t *y, uint32_t *result, uint8_t length){
	uint32_t temp[length * 2];
	setZero(temp, length * 2);
	setZero(result, length * 2);
	uint8_t k, n;
	uint64_t l;
	for (k = 0; k < length; k++){
		for (n = 0; n < length; n++){ 
			l = (uint64_t)x[n]*(uint64_t)y[k];
			temp[n+k] = l&0xFFFFFFFF;
			temp[n+k+1] = l>>32;
			add(&temp[n+k], &result[n+k], &result[n+k], (length * 2) - (n + k));

			setZero(temp, length * 2);
		}
	}
	return 0;
}

//TODO: maximum:
//fffffffe00000002fffffffe0000000100000001fffffffe00000001fffffffe00000001fffffffefffffffffffffffffffffffe000000000000000000000001_16
static void fieldModP(uint32_t *A, const uint32_t *B)
{
	uint32_t tempm[8];
	uint32_t tempm2[8];
	uint8_t n;
	setZero(tempm, 8);
	setZero(tempm2, 8);
	/* A = T */ 
	copy(B,A,arrayLength);

	/* Form S1 */ 
	for(n=0;n<3;n++) tempm[n]=0; 
	for(n=3;n<8;n++) tempm[n]=B[n+8];

	/* tempm2=T+S1 */ 
	fieldAdd(A,tempm,ecc_prime_r,tempm2);
	/* A=T+S1+S1 */ 
	fieldAdd(tempm2,tempm,ecc_prime_r,A);
	/* Form S2 */ 
	for(n=0;n<3;n++) tempm[n]=0; 
	for(n=3;n<7;n++) tempm[n]=B[n+9]; 
	for(n=7;n<8;n++) tempm[n]=0;
	/* tempm2=T+S1+S1+S2 */ 
	fieldAdd(A,tempm,ecc_prime_r,tempm2);
	/* A=T+S1+S1+S2+S2 */ 
	fieldAdd(tempm2,tempm,ecc_prime_r,A);
	/* Form S3 */ 
	for(n=0;n<3;n++) tempm[n]=B[n+8]; 
	for(n=3;n<6;n++) tempm[n]=0; 
	for(n=6;n<8;n++) tempm[n]=B[n+8];
	/* tempm2=T+S1+S1+S2+S2+S3 */ 
	fieldAdd(A,tempm,ecc_prime_r,tempm2);
	/* Form S4 */ 
	for(n=0;n<3;n++) tempm[n]=B[n+9]; 
	for(n=3;n<6;n++) tempm[n]=B[n+10]; 
	for(n=6;n<7;n++) tempm[n]=B[n+7]; 
	for(n=7;n<8;n++) tempm[n]=B[n+1];
	/* A=T+S1+S1+S2+S2+S3+S4 */ 
	fieldAdd(tempm2,tempm,ecc_prime_r,A);
	/* Form D1 */ 
	for(n=0;n<3;n++) tempm[n]=B[n+11]; 
	for(n=3;n<6;n++) tempm[n]=0; 
	for(n=6;n<7;n++) tempm[n]=B[n+2]; 
	for(n=7;n<8;n++) tempm[n]=B[n+3];
	/* tempm2=T+S1+S1+S2+S2+S3+S4-D1 */ 
	fieldSub(A,tempm,ecc_prime_m,tempm2);
	/* Form D2 */ 
	for(n=0;n<4;n++) tempm[n]=B[n+12]; 
	for(n=4;n<6;n++) tempm[n]=0; 
	for(n=6;n<7;n++) tempm[n]=B[n+3]; 
	for(n=7;n<8;n++) tempm[n]=B[n+4];
	/* A=T+S1+S1+S2+S2+S3+S4-D1-D2 */ 
	fieldSub(tempm2,tempm,ecc_prime_m,A);
	/* Form D3 */ 
	for(n=0;n<3;n++) tempm[n]=B[n+13]; 
	for(n=3;n<6;n++) tempm[n]=B[n+5]; 
	for(n=6;n<7;n++) tempm[n]=0; 
	for(n=7;n<8;n++) tempm[n]=B[n+5];
	/* tempm2=T+S1+S1+S2+S2+S3+S4-D1-D2-D3 */ 
	fieldSub(A,tempm,ecc_prime_m,tempm2);
	/* Form D4 */ 
	for(n=0;n<2;n++) tempm[n]=B[n+14]; 
	for(n=2;n<3;n++) tempm[n]=0; 
	for(n=3;n<6;n++) tempm[n]=B[n+6]; 
	for(n=6;n<7;n++) tempm[n]=0; 
	for(n=7;n<8;n++) tempm[n]=B[n+6];
	/* A=T+S1+S1+S2+S2+S3+S4-D1-D2-D3-D4 */ 
	fieldSub(tempm2,tempm,ecc_prime_m,A);
	if(isGreater(A, ecc_prime_m, arrayLength) >= 0){
		fieldSub(A, ecc_prime_m, ecc_prime_m, tempm);
		copy(tempm, A, arrayLength);
	}
}

/**
 * calculate the result = A mod n.
 * n is the order of the eliptic curve.
 * A and result could point to the same value
 *
 * A: input value (max size * 4 bytes)
 * result: result of modulo calculation (max 36 bytes)
 * size: size of A
 *
 * This uses the Barrett modular reduction as described in the Handbook 
 * of Applied Cryptography 14.42 Algorithm Barrett modular reduction, 
 * see http://cacr.uwaterloo.ca/hac/about/chap14.pdf and 
 * http://everything2.com/title/Barrett+Reduction
 *
 * b = 32 (bite size of the processor architecture)
 * mu (ecc_order_mu) was precomputed in a java program
 */
static void fieldModO(const uint32_t *A, uint32_t *result, uint8_t length) {
	// This is used for value q1 and q3
	uint32_t q1_q3[9];
	// This is used for q2 and a temp var
	uint32_t q2_tmp[18];

	// return if the given value is smaller than the modulus
	if (length == arrayLength && isGreater(A, ecc_order_m, arrayLength) <= 0) {
		if (A != result)
		        copy(A, result, length);
		return;
	}

	rshiftby(A, length, q1_q3, 9, ecc_order_k - 1);

	fieldMult(ecc_order_mu, q1_q3, q2_tmp, 9);

	rshiftby(q2_tmp, 18, q1_q3, 8, ecc_order_k + 1);

	// r1 = first 9 blocks of A

	fieldMult(q1_q3, ecc_order_m, q2_tmp, 8);

	// r2 = first 9 blocks of q2_tmp

	sub(A, q2_tmp, result, 9);

	while (isGreater(result, ecc_order_m, 9) >= 0)
		sub(result, ecc_order_m, result, 9);
}

static int isOne(const uint32_t* A){
	uint8_t n; 
	for(n=1;n<8;n++) 
		if (A[n]!=0) 
			break;

	if ((n==8)&&(A[0]==1)) 
		return 1;
	else 
		return 0;
}

static int isZero(const uint32_t* A){
	uint8_t n, r=0;
	for(n=0;n<8;n++){
		if (A[n] == 0) r++;
	}
	return r==8;
}

static void rshift(uint32_t* A){
	int n, i;
	uint32_t nOld = 0;
	for (i = 8; i--;)
	{
		n = A[i]&0x1;
		A[i] = A[i]>>1 | nOld<<31;
		nOld = n;
	}
}

static int fieldAddAndDivide(const uint32_t *x, const uint32_t *modulus, const uint32_t *reducer, uint32_t* result){
	uint32_t n = add(x, modulus, result, arrayLength);
	rshift(result);
	if(n){ //add prime if carry is still set!
		result[7] |= 0x80000000;//add the carry
		if (isGreater(result, modulus, arrayLength) == 1)
		{
			uint32_t tempas[8];
			setZero(tempas, 8);
			add(result, reducer, tempas, 8);
			copy(tempas, result, arrayLength);
		}
		
	}
	return 0;
}

/*
 * Inverse A and output to B
 */
static void fieldInv(const uint32_t *A, const uint32_t *modulus, const uint32_t *reducer, uint32_t *B){
	uint32_t u[8],v[8],x1[8],x2[8];
	uint32_t tempm[8];
	uint32_t tempm2[8];
	setZero(tempm, 8);
	setZero(tempm2, 8);
	setZero(u, 8);
	setZero(v, 8);

	uint8_t t;
	copy(A,u,arrayLength); 
	copy(modulus,v,arrayLength); 
	setZero(x1, 8);
	setZero(x2, 8);
	x1[0]=1; 
	/* While u !=1 and v !=1 */ 
	while ((isOne(u) || isOne(v))==0) {
		while(!(u[0]&1)) { 					/* While u is even */
			rshift(u); 						/* divide by 2 */
			if (!(x1[0]&1))					/*ifx1iseven*/
				rshift(x1);					/* Divide by 2 */
			else {
				fieldAddAndDivide(x1,modulus,reducer,tempm); /* tempm=x1+p */
				copy(tempm,x1,arrayLength); 		/* x1=tempm */
				//rshift(x1);					/* Divide by 2 */
			}
		} 
		while(!(v[0]&1)) {					/* While v is even */
			rshift(v); 						/* divide by 2 */ 
			if (!(x2[0]&1))					/*ifx1iseven*/
				rshift(x2); 				/* Divide by 2 */
			else
			{
				fieldAddAndDivide(x2,modulus,reducer,tempm);	/* tempm=x1+p */
				copy(tempm,x2,arrayLength); 			/* x1=tempm */ 
				//rshift(x2);					/* Divide by 2 */
			}
			
		} 
		t=sub(u,v,tempm,arrayLength); 				/* tempm=u-v */
		if (t==0) {							/* If u > 0 */
			copy(tempm,u,arrayLength); 					/* u=u-v */
			fieldSub(x1,x2,modulus,tempm); 			/* tempm=x1-x2 */
			copy(tempm,x1,arrayLength);					/* x1=x1-x2 */
		} else {
			sub(v,u,tempm,arrayLength); 			/* tempm=v-u */
			copy(tempm,v,arrayLength); 					/* v=v-u */
			fieldSub(x2,x1,modulus,tempm); 			/* tempm=x2-x1 */
			copy(tempm,x2,arrayLength);					/* x2=x2-x1 */
		}
	} 
	if (isOne(u)) {
		copy(x1,B,arrayLength); 
	} else {
		copy(x2,B,arrayLength);
	}
}

void static ec_double(const uint32_t *px, const uint32_t *py, uint32_t *Dx, uint32_t *Dy){
	uint32_t tempA[8];
	uint32_t tempB[8];
	uint32_t tempC[8];
	uint32_t tempD[16];

	if(isZero(px) && isZero(py)){
		copy(px, Dx,arrayLength);
		copy(py, Dy,arrayLength);
		return;
	}

	fieldMult(px, px, tempD, arrayLength);
	fieldModP(tempA, tempD);
	setZero(tempB, 8);
	tempB[0] = 0x00000001;
	fieldSub(tempA, tempB, ecc_prime_m, tempC); //tempC = (qx^2-1)
	tempB[0] = 0x00000003;
	fieldMult(tempC, tempB, tempD, arrayLength);
	fieldModP(tempA, tempD);//tempA = 3*(qx^2-1)
	fieldAdd(py, py, ecc_prime_r, tempB); //tempB = 2*qy
	fieldInv(tempB, ecc_prime_m, ecc_prime_r, tempC); //tempC = 1/(2*qy)
	fieldMult(tempA, tempC, tempD, arrayLength); //tempB = lambda = (3*(qx^2-1))/(2*qy)
	fieldModP(tempB, tempD);

	fieldMult(tempB, tempB, tempD, arrayLength); //tempC = lambda^2
	fieldModP(tempC, tempD);
	fieldSub(tempC, px, ecc_prime_m, tempA); //lambda^2 - Px
	fieldSub(tempA, px, ecc_prime_m, Dx); //lambda^2 - Px - Qx

	fieldSub(px, Dx, ecc_prime_m, tempA); //tempA = qx-dx
	fieldMult(tempB, tempA, tempD, arrayLength); //tempC = lambda * (qx-dx)
	fieldModP(tempC, tempD);
	fieldSub(tempC, py, ecc_prime_m, Dy); //Dy = lambda * (qx-dx) - px
}

void static ec_add(const uint32_t *px, const uint32_t *py, const uint32_t *qx, const uint32_t *qy, uint32_t *Sx, uint32_t *Sy){
	uint32_t tempA[8];
	uint32_t tempB[8];
	uint32_t tempC[8];
	uint32_t tempD[16];

	if(isZero(px) && isZero(py)){
		copy(qx, Sx,arrayLength);
		copy(qy, Sy,arrayLength);
		return;
	} else if(isZero(qx) && isZero(qy)) {
		copy(px, Sx,arrayLength);
		copy(py, Sy,arrayLength);
		return;
	}

	if(isSame(px, qx, arrayLength)){
		if(!isSame(py, qy, arrayLength)){
			setZero(Sx, 8);
			setZero(Sy, 8);
			return;
		} else {
			ec_double(px, py, Sx, Sy);
			return;
		}
	}

	fieldSub(py, qy, ecc_prime_m, tempA);
	fieldSub(px, qx, ecc_prime_m, tempB);
	fieldInv(tempB, ecc_prime_m, ecc_prime_r, tempB);
	fieldMult(tempA, tempB, tempD, arrayLength); 
	fieldModP(tempC, tempD); //tempC = lambda

	fieldMult(tempC, tempC, tempD, arrayLength); //tempA = lambda^2
	fieldModP(tempA, tempD);
	fieldSub(tempA, px, ecc_prime_m, tempB); //lambda^2 - Px
	fieldSub(tempB, qx, ecc_prime_m, Sx); //lambda^2 - Px - Qx

	fieldSub(qx, Sx, ecc_prime_m, tempB);
	fieldMult(tempC, tempB, tempD, arrayLength);
	fieldModP(tempC, tempD);
	fieldSub(tempC, qy, ecc_prime_m, Sy);
}

void ecc_ec_mult(const uint32_t *px, const uint32_t *py, const uint32_t *secret, uint32_t *resultx, uint32_t *resulty){
	uint32_t Qx[8];
	uint32_t Qy[8];
	setZero(Qx, 8);
	setZero(Qy, 8);

	uint32_t tempx[8];
	uint32_t tempy[8];

	int i;
	for (i = 256;i--;){
		ec_double(Qx, Qy, tempx, tempy);
		copy(tempx, Qx,arrayLength);
		copy(tempy, Qy,arrayLength);
		if (((secret[i / 32]) & ((uint32_t)1 << (i % 32)))) {
			ec_add(Qx, Qy, px, py, tempx, tempy); //eccAdd
			copy(tempx, Qx,arrayLength);
			copy(tempy, Qy,arrayLength);
		}
	}
	copy(Qx, resultx,arrayLength);
	copy(Qy, resulty,arrayLength);
}

/**
 * Calculate the ecdsa signature.
 *
 * For a description of this algorithm see
 * https://en.wikipedia.org/wiki/Elliptic_Curve_DSA#Signature_generation_algorithm
 *
 * input:
 *  d: private key on the curve secp256r1 (32 bytes)
 *  e: hash to sign (32 bytes)
 *  k: random data, this must be changed for every signature (32 bytes)
 *
 * output:
 *  r: r value of the signature (36 bytes)
 *  s: s value of the signature (36 bytes)
 *
 * return:
 *   0: everything is ok
 *  -1: can not create signature, try again with different k.
 */
int ecc_ecdsa_sign(const uint32_t *d, const uint32_t *e, const uint32_t *k, uint32_t *r, uint32_t *s)
{
	uint32_t tmp1[16];
	uint32_t tmp2[9];
	uint32_t tmp3[9];

	if (isZero(k))
		return -1;

	// 4. Calculate the curve point (x_1, y_1) = k * G.
	ecc_ec_mult(ecc_g_point_x, ecc_g_point_y, k, r, tmp1);

	// 5. Calculate r = x_1 \pmod{n}.
	fieldModO(r, r, 8);

	// 5. If r = 0, go back to step 3.
	if (isZero(r))
		return -1;

	// 6. Calculate s = k^{-1}(z + r d_A) \pmod{n}.
	// 6. r * d
	fieldMult(r, d, tmp1, arrayLength);
	fieldModO(tmp1, tmp2, 16);

	// 6. z + (r d)
	tmp1[8] = add(e, tmp2, tmp1, 8);
	fieldModO(tmp1, tmp3, 9);

	// 6. k^{-1}
	fieldInv(k, ecc_order_m, ecc_order_r, tmp2);

	// 6. (k^{-1}) (z + (r d))
	fieldMult(tmp2, tmp3, tmp1, arrayLength);
	fieldModO(tmp1, s, 16);

	// 6. If s = 0, go back to step 3.
	if (isZero(s))
		return -1;

	return 0;
}

/**
 * Verifies a ecdsa signature.
 *
 * For a description of this algorithm see
 * https://en.wikipedia.org/wiki/Elliptic_Curve_DSA#Signature_verification_algorithm
 *
 * input:
 *  x: x coordinate of the public key (32 bytes)
 *  y: y coordinate of the public key (32 bytes)
 *  e: hash to verify the signature of (32 bytes)
 *  r: r value of the signature (32 bytes)
 *  s: s value of the signature (32 bytes)
 *
 * return:
 *  0: signature is ok
 *  -1: signature check failed the signature is invalid
 */
int ecc_ecdsa_validate(const uint32_t *x, const uint32_t *y, const uint32_t *e, const uint32_t *r, const uint32_t *s)
{
	uint32_t w[8];
	uint32_t tmp[16];
	uint32_t u1[9];
	uint32_t u2[9];
	uint32_t tmp1_x[8];
	uint32_t tmp1_y[8];
	uint32_t tmp2_x[8];
	uint32_t tmp2_y[8];
	uint32_t tmp3_x[8];
	uint32_t tmp3_y[8];

	// 3. Calculate w = s^{-1} \pmod{n}
	fieldInv(s, ecc_order_m, ecc_order_r, w);

	// 4. Calculate u_1 = zw \pmod{n}
	fieldMult(e, w, tmp, arrayLength);
	fieldModO(tmp, u1, 16);

	// 4. Calculate u_2 = rw \pmod{n}
	fieldMult(r, w, tmp, arrayLength);
	fieldModO(tmp, u2, 16);

	// 5. Calculate the curve point (x_1, y_1) = u_1 * G + u_2 * Q_A.
	// tmp1 = u_1 * G
	ecc_ec_mult(ecc_g_point_x, ecc_g_point_y, u1, tmp1_x, tmp1_y);

	// tmp2 = u_2 * Q_A
	ecc_ec_mult(x, y, u2, tmp2_x, tmp2_y);

	// tmp3 = tmp1 + tmp2
	ec_add(tmp1_x, tmp1_y, tmp2_x, tmp2_y, tmp3_x, tmp3_y);
	// TODO: this u_1 * G + u_2 * Q_A  could be optimiced with Straus's algorithm.

	return isSame(tmp3_x, r, arrayLength) ? 0 : -1;
}

int ecc_is_valid_key(const uint32_t * priv_key)
{
	return isGreater(ecc_order_m, priv_key, arrayLength) == 1;
}

/*
 * This exports the low level functions so the tests can use them.
 * In real use the compiler is now bale to optimice the code better.
 */
#ifdef TEST_INCLUDE
uint32_t ecc_add( const uint32_t *x, const uint32_t *y, uint32_t *result, uint8_t length)
{
	return add(x, y, result, length);
}
uint32_t ecc_sub( const uint32_t *x, const uint32_t *y, uint32_t *result, uint8_t length)
{
	return sub(x, y, result, length);
}
int ecc_fieldAdd(const uint32_t *x, const uint32_t *y, const uint32_t *reducer, uint32_t *result)
{
	return fieldAdd(x, y, reducer, result);
}
int ecc_fieldSub(const uint32_t *x, const uint32_t *y, const uint32_t *modulus, uint32_t *result)
{
	return fieldSub(x, y, modulus, result);
}
int ecc_fieldMult(const uint32_t *x, const uint32_t *y, uint32_t *result, uint8_t length)
{
	return fieldMult(x, y, result, length);
}
void ecc_fieldModP(uint32_t *A, const uint32_t *B)
{
	fieldModP(A, B);
}
void ecc_fieldModO(const uint32_t *A, uint32_t *result, uint8_t length)
{
	fieldModO(A, result, length);
}
void ecc_fieldInv(const uint32_t *A, const uint32_t *modulus, const uint32_t *reducer, uint32_t *B)
{
	fieldInv(A, modulus, reducer, B);
}
void ecc_copy(const uint32_t *from, uint32_t *to, uint8_t length)
{
	copy(from, to, length);
}
int ecc_isSame(const uint32_t *A, const uint32_t *B, uint8_t length)
{
	return isSame(A, B, length);
}
void ecc_setZero(uint32_t *A, const int length)
{
	setZero(A, length);
}
int ecc_isOne(const uint32_t* A)
{
	return isOne(A);
}
void ecc_rshift(uint32_t* A)
{
	rshift(A);
}
int ecc_isGreater(const uint32_t *A, const uint32_t *B, uint8_t length)
{
	return isGreater(A, B , length);
}

void ecc_ec_add(const uint32_t *px, const uint32_t *py, const uint32_t *qx, const uint32_t *qy, uint32_t *Sx, uint32_t *Sy)
{
	ec_add(px, py, qx, qy, Sx, Sy);
}
void ecc_ec_double(const uint32_t *px, const uint32_t *py, uint32_t *Dx, uint32_t *Dy)
{
	ec_double(px, py, Dx, Dy);
}

#endif /* TEST_INCLUDE */
