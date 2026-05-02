/*
 * enc.c
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is a model of the enigma hardware as remembered from
 * a description read around ten years prior.  The model has
 * not been tested or validated as cryptographically secure.
 * Furthermore the author intentionally did not validate the
 * model against published descriptions of the enigma.  The
 * only test completed was to set the wheels to a known
 * position and insert a pt string of text into the function.
 * The output was recorded as ct.  The wheels were reset to the
 * original start position, and the recorded ct was inserted
 * into the function.  The output was compared to the original
 * pt text and matched.  The result of the test matched the
 * expected outcome, but it does not validate the algorithm
 * meets any security requirements.
 *
 * **********************************************************
 *
 * DO NOT USE IN PRODUCTION CODE AS A SECURITY FEATURE
 *
 * **********************************************************
 */

#include "enc.h"


/*
 * for a pt character encrypt a ct character and update wheels
 * Each wheel must be indexed into and out of the wheel arrays
 * this process is based on a absolute index of input characters
 * and the reflector.
 * Note: the output of wheel arrays are added to WHEEL_SIZE
 * to prevent a negative index when subtracting the iw value.
 * The printk lines have been left in to inspect operations
 * of the enigma simulation. simply add the definition DBUG
 * to enable the detailed messages.
 */

char enig_enc(char pt)
{
	short tmpIndex;
	char ct;
#ifdef DBUG
	printk("\nEE PT: %c, %02x\n", pt, pt);
	printk("Index: %d, %d, %d\n", IW1, IW2, IW3);
#endif
	tmpIndex = char_to_index(pt);
#ifdef DBUG
	printk("EE   : %02x\n", tmpIndex);
#endif
	/* if error return  */
	if (tmpIndex == -1) {
		return (char)0xFF;
	}

	tmpIndex = (W1[IMOD(IW1, tmpIndex)] + WHEEL_SIZE - IW1) % WHEEL_SIZE;
#ifdef DBUG
	printk("EE i1: %02x\n", tmpIndex);
#endif
	tmpIndex = (W2[IMOD(IW2, tmpIndex)] +  WHEEL_SIZE - IW2) % WHEEL_SIZE;
#ifdef DBUG
	printk("EE i2: %02x\n", tmpIndex);
#endif
	tmpIndex = (W3[IMOD(IW3, tmpIndex)] + WHEEL_SIZE - IW3) % WHEEL_SIZE;
#ifdef DBUG
	printk("EE i3: %02x\n", tmpIndex);
#endif
	tmpIndex = R[tmpIndex];
#ifdef DBUG
	printk("EE  r: %02x\n", tmpIndex);
#endif
	tmpIndex = (W3R[IMOD(IW3, tmpIndex)] + WHEEL_SIZE - IW3) % WHEEL_SIZE;
#ifdef DBUG
	printk("EE i3: %02x\n", tmpIndex);
#endif
	tmpIndex = (W2R[IMOD(IW2, tmpIndex)] + WHEEL_SIZE - IW2) % WHEEL_SIZE;
#ifdef DBUG
	printk("EE i2: %02x\n", tmpIndex);
#endif
	tmpIndex = (W1R[IMOD(IW1, tmpIndex)] + WHEEL_SIZE - IW1) % WHEEL_SIZE;
#ifdef DBUG
	printk("EE i1: %02x\n", tmpIndex);
#endif

	ct = index_to_char(tmpIndex);
#ifdef DBUG
	printk("EE CT: %02x\n", ct);
#endif
	/* test ct value or just return error ? */
	update_wheel_index();
	return ct;
}

/*
 * calc reverse path for wheel
 *  this simplifies the reverse path calculation
 * Return: 1:ok -1 error
 */
int calc_rev_wheel(BYTE *wheel, BYTE *backpath)
{

	int i;

	for (i = 0; i < WHEEL_SIZE; i++) {
		if (wheel[i] >= WHEEL_SIZE) {
			return -1;
		}
		backpath[wheel[i]] = i;
	}
	return 1;
}


/*
 * convert a-z to 0-25
 */
short char_to_index(char c)
{
	if (c < 'a' || c > 'z') {
		return -1;
	}
	return (short)(c - 'a');
}

/*
 *  convert from a index 0-25 to a-z
 */
char index_to_char(short i)
{
	if (i < 0 || i > 25) {
		return 0xFF;
	}
	return (char)((short)'a' + i);
}

/*
 *  basic update to wheels based on full rotation
 *  of prior wheel.  This could be modified to change
 *  the direction of rotation or order of updates
 */
void update_wheel_index(void)
{
	IW1++;
	if (IW1 >= WHEEL_SIZE) {
		IW1 %= WHEEL_SIZE;
		IW2++;
	}
	if (IW2 >= WHEEL_SIZE) {
		IW2 %= WHEEL_SIZE;
		IW3++;
	}
	if (IW3 >= WHEEL_SIZE) {
		IW3 %= WHEEL_SIZE;
	}

}
