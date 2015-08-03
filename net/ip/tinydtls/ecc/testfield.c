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
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "ecc.h"
#include "test_helper.h"

#ifdef CONTIKI
#include "contiki.h"
#endif /* CONTIKI */

//arbitrary test values and results
uint32_t null[8] = {	0x00000000,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000};
uint32_t null64[16] = {	0x00000000,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000};
uint32_t one[8] = {	0x00000001,0x00000000,0x00000000,0x00000000,
					0x00000000,0x00000000,0x00000000,0x00000000};
uint32_t one64[16] = {	0x00000001,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000};
uint32_t two[8] = {	0x00000002,0x00000000,0x00000000,0x00000000,
					0x00000000,0x00000000,0x00000000,0x00000000};
uint32_t two64[16] = {	0x00000002,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000};
uint32_t three[8] = {	0x00000003,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000};
uint32_t four[8] = {0x00000004,0x00000000,0x00000000,0x00000000,
					0x00000000,0x00000000,0x00000000,0x00000000};
uint32_t four64[16] = {	0x00000004,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000};
uint32_t six[8] = {	0x00000006,0x00000000,0x00000000,0x00000000,
					0x00000000,0x00000000,0x00000000,0x00000000};
uint32_t eight[8] = {	0x00000008,0x00000000,0x00000000,0x00000000,
						0x00000000,0x00000000,0x00000000,0x00000000};
uint32_t full[8] = { 	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
						0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
//00000000fffffffeffffffffffffffffffffffff000000000000000000000001_16
uint32_t resultFullAdd[8] = {	0x00000001,0x00000000,0x00000000,0xFFFFFFFF,
								0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFE,0x00000000};
uint32_t primeMinusOne[8]=	{	0xfffffffe,0xffffffff,0xffffffff,0x00000000,
								0x00000000,0x00000000,0x00000001,0xffffffff};
uint32_t resultDoubleMod[8] = { 0xfffffffd,0xffffffff,0xffffffff,0x00000000,
								0x00000000,0x00000000,0x00000001,0xffffffff};
//fffffffe00000002fffffffe0000000100000001fffffffe00000001fffffffc00000003fffffffcfffffffffffffffffffffffc000000000000000000000004_16
uint32_t resultQuadMod[16] = {	0x00000004,0x00000000,0x00000000,0xFFFFFFFC,
								0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFC,0x00000003,
								0xFFFFFFFC,0x00000001,0xFFFFFFFE,0x00000001,
								0x00000001,0xFFFFFFFE,0x00000002,0xFFFFFFFE};
//00000002fffffffffffffffffffffffefffffffdffffffff0000000000000002_16
uint32_t resultFullMod[8] = { 	0x00000002,0x00000000,0xFFFFFFFF,0xFFFFFFFD,
								0xFFFFFFFE,0xFFFFFFFF,0xFFFFFFFF,0x00000002};

static const uint32_t orderMinusOne[8] = {0xFC632550, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD,
					0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF};
static const uint32_t orderResultDoubleMod[8] = {0xFC63254F, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF};

uint32_t temp[8];
uint32_t temp2[16];

void nullEverything(){
	memset(temp, 0, sizeof(temp));
	memset(temp2, 0, sizeof(temp));
}

void fieldAddTest(){
	assert(ecc_isSame(one, one, arrayLength));
	ecc_fieldAdd(one, null, ecc_prime_r, temp);
	assert(ecc_isSame(temp, one, arrayLength));
	nullEverything();
	ecc_fieldAdd(one, one, ecc_prime_r, temp);
	assert(ecc_isSame(temp, two, arrayLength));
	nullEverything();
	ecc_add(full, one, temp, 32);
	assert(ecc_isSame(null, temp, arrayLength));
	nullEverything();
	ecc_fieldAdd(full, one, ecc_prime_r, temp);
	assert(ecc_isSame(temp, resultFullAdd, arrayLength));
}

void fieldSubTest(){
	assert(ecc_isSame(one, one, arrayLength));
	ecc_fieldSub(one, null, ecc_prime_m, temp);
	assert(ecc_isSame(one, temp, arrayLength));
	nullEverything();
	ecc_fieldSub(one, one, ecc_prime_m, temp);
	assert(ecc_isSame(null, temp, arrayLength));
	nullEverything();
	ecc_fieldSub(null, one, ecc_prime_m, temp);
	assert(ecc_isSame(primeMinusOne, temp, arrayLength));
}

void fieldMultTest(){
	ecc_fieldMult(one, null, temp2, arrayLength);
	assert(ecc_isSame(temp2, null64, arrayLength * 2));
	nullEverything();
	ecc_fieldMult(one, two, temp2, arrayLength);
	assert(ecc_isSame(temp2, two64, arrayLength * 2));
	nullEverything();
	ecc_fieldMult(two, two, temp2, arrayLength);
	assert(ecc_isSame(temp2, four64, arrayLength * 2));
	nullEverything();
	ecc_fieldMult(primeMinusOne, primeMinusOne, temp2, arrayLength);
	assert(ecc_isSame(temp2, resultQuadMod, arrayLength * 2));
	nullEverything();
	ecc_fieldInv(two, ecc_prime_m, ecc_prime_r, temp);
	ecc_fieldMult(temp, two, temp2, arrayLength);
	ecc_fieldModP(temp, temp2);
	assert(ecc_isSame(temp, one, arrayLength));
}

void fieldModPTest(){
	ecc_fieldMult(primeMinusOne, primeMinusOne, temp2, arrayLength);
	ecc_fieldModP(temp, temp2);
	assert(ecc_isSame(temp, one, arrayLength));
	nullEverything();
	ecc_fieldModP(temp, one64);
	assert(ecc_isSame(temp, one, arrayLength));
	nullEverything();
	ecc_fieldMult(two, primeMinusOne, temp2, arrayLength);
	ecc_fieldModP(temp, temp2);
	assert(ecc_isSame(temp, resultDoubleMod, arrayLength));
	nullEverything();
	/*fieldMult(full, full, temp2, arrayLength); //not working, maybe because of the number bigger than p^2?
	fieldModP(temp, temp2);
	assert(ecc_isSame(temp, resultFullMod, arrayLength));*/
}

void fieldModOTest(){
	ecc_fieldMult(orderMinusOne, orderMinusOne, temp2, arrayLength);
	ecc_fieldModO(temp2, temp, arrayLength * 2);
	assert(ecc_isSame(temp, one, arrayLength));
	nullEverything();
	ecc_fieldModO(one64, temp, arrayLength * 2);
	assert(ecc_isSame(temp, one, arrayLength));
	nullEverything();
	ecc_fieldMult(two, orderMinusOne, temp2, arrayLength);
	ecc_fieldModO(temp2, temp, arrayLength * 2);
	assert(ecc_isSame(temp, orderResultDoubleMod, arrayLength));
	nullEverything();
}


// void rShiftTest(){
// 	printNumber(full, 32);
// 	rshift(full);
// 	printNumber(full, 32);
// 	printNumber(two, 32);
// 	rshift(two);
// 	printNumber(two, 32);
// 	printNumber(four, 32);
// 	rshift(four);
// 	printNumber(four, 32);
// }

// void isOneTest(){
// 	printf("%d\n", isone(one));
// 	printf("%d\n", isone(two));
// 	printf("%d\n", isone(four));
// 	printf("%d\n", isone(full));
// 	printf("%d\n", isone(null));
// }

void fieldInvTest(){
	nullEverything();
	ecc_fieldInv(two, ecc_prime_m, ecc_prime_r, temp);
	ecc_fieldMult(temp, two, temp2, arrayLength);
	ecc_fieldModP(temp, temp2);
	assert(ecc_isSame(one, temp, arrayLength));
	nullEverything();
	ecc_fieldInv(eight, ecc_prime_m, ecc_prime_r, temp);
	ecc_fieldMult(temp, eight, temp2, arrayLength);
	ecc_fieldModP(temp, temp2);
	assert(ecc_isSame(one, temp, arrayLength));
	nullEverything();
	ecc_fieldInv(three, ecc_prime_m, ecc_prime_r, temp);
	ecc_fieldMult(temp, three, temp2, arrayLength);
	ecc_fieldModP(temp, temp2);
	assert(ecc_isSame(one, temp, arrayLength));
	nullEverything();
	ecc_fieldInv(six, ecc_prime_m, ecc_prime_r, temp);
	ecc_fieldMult(temp, six, temp2, arrayLength);
	ecc_fieldModP(temp, temp2);
	assert(ecc_isSame(one, temp, arrayLength));
	nullEverything();
	ecc_fieldInv(primeMinusOne, ecc_prime_m, ecc_prime_r, temp);
	ecc_fieldMult(temp, primeMinusOne, temp2, arrayLength);
	ecc_fieldModP(temp, temp2);
	assert(ecc_isSame(one, temp, arrayLength));
}

// void randomStuff(){

// }

#ifdef CONTIKI
PROCESS(ecc_filed_test, "ECC field test");
AUTOSTART_PROCESSES(&ecc_filed_test);
PROCESS_THREAD(ecc_filed_test, ev, d)
{
	PROCESS_BEGIN();

	nullEverything();
	//randomStuff();
	nullEverything();
	fieldAddTest();
	nullEverything();
	fieldSubTest();
	nullEverything();
	fieldMultTest();
	nullEverything();
	fieldModPTest();
	nullEverything();
	fieldModOTest();
	nullEverything();
	fieldInvTest();
	nullEverything();
	//rShiftTest();
	//isOneTest();
	printf("%s\n", "All Tests succesfull!");

	PROCESS_END();
}
#else /* CONTIKI */
int main(int argc, char const *argv[])
{
	nullEverything();
	//randomStuff();
	nullEverything();
	fieldAddTest();
	nullEverything();
	fieldSubTest();
	nullEverything();
	fieldMultTest();
	nullEverything();
	fieldModPTest();
	nullEverything();
	fieldModOTest();
	nullEverything();
	fieldInvTest();
	nullEverything();
	//rShiftTest();
	//isOneTest();
	printf("%s\n", "All Tests succesfull!");
	return 0;
}
#endif /* CONTIKI */
