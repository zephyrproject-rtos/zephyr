/*  main.c  */

/*
 *   SPDX-License-Identifier: Apache-2.0
 */

/*
 *
 *  Basic example of userspace thread protected memory
 *
 *  NOTE: The encryption algorithm is unverified and
 *  based on a 1930's era piece of hardware.
 *  DO NOT USE THIS CODE FOR SECURITY
 *
 */

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/libc-hooks.h> /* for z_libc_partition */

#include "main.h"
#include "enc.h"
/* the following definition name prefix is to avoid a conflict */
#define SAMP_BLOCKSIZE 50

/*
 *  The memory partitions have been named to simplify
 * the definition of variables.  A possible alternative
 * is using one source file per thread and implementing
 * a objcopy to rename the data and bss section for the
 * thread to the partition name.
 */

/* prepare the memory partition structures  */
FOR_EACH(K_APPMEM_PARTITION_DEFINE, (;), user_part, red_part, enc_part, blk_part, ct_part);
/* prepare the memory domain structures  */
struct k_mem_domain pt_domain, enc_domain;
/* each variable starts with a name defined in main.h
 * the names are symbolic for the memory partitions
 * purpose.
 */
volatile _app_red_b BYTE fBUFIN;
volatile _app_red_b BYTE BUFIN[63];

volatile _app_blk_b BYTE fBUFOUT;
volatile _app_blk_b BYTE BUFOUT[63];

/* declare and set wheel and reflector  */
/* To use add definition ALTMSG */
#ifdef ALTMSG
volatile _app_enc_d BYTE W1[26] = START_WHEEL;
#else
volatile _app_enc_d BYTE W1[26] = START_WHEEL2;
#endif
volatile _app_enc_d BYTE W2[26] = START_WHEEL;
volatile _app_enc_d BYTE W3[26] = START_WHEEL;
volatile _app_enc_d BYTE R[26] = REFLECT;

volatile _app_enc_b int IW1;
volatile _app_enc_b int IW2;
volatile _app_enc_b int IW3;

/*
 *   calculated by the enc thread at init and when the wheels
 *   change.
 */
volatile _app_enc_b BYTE W1R[26];
volatile _app_enc_b BYTE W2R[26];
volatile _app_enc_b BYTE W3R[26];

/*
 *	sync threads
 */
K_SEM_DEFINE(allforone, 0, 3);

struct k_thread enc_thread;
K_THREAD_STACK_DEFINE(enc_stack, STACKSIZE);

struct k_thread pt_thread;
K_THREAD_STACK_DEFINE(pt_stack, STACKSIZE);

struct k_thread ct_thread;
K_THREAD_STACK_DEFINE(ct_stack, STACKSIZE);

_app_enc_d char encMSG[] = "ENC!\n";
volatile _app_enc_b char enc_pt[50];  /* Copy form shared pt */
volatile _app_enc_b char enc_ct[50]; /* Copy to shared ct */

_app_user_d char ptMSG[] = "PT: message to encrypt\n";

/* encrypted message when W1 = START_WHEEL */
/* to use add definition ALTMSG  */
#ifdef ALTMSG
_app_user_d char ptMSG2[] = "nfttbhfspfmdqzos\n";
#else
/* encrypted message when W1 = START_WHEEL2 */
_app_user_d char ptMSG2[] = "ofttbhfspgmeqzos\n";
#endif
_app_ct_d char ctMSG[] = "CT!\n";




void main(void)
{
	struct k_mem_partition *enc_parts[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&enc_part, &red_part, &blk_part
	};
	struct k_mem_partition *pt_parts[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&user_part, &red_part
	};
	k_tid_t tPT, tENC, tCT;
	int ret;

	fBUFIN = 0; /* clear flags */
	fBUFOUT = 0;
	calc_rev_wheel((BYTE *) &W1, (BYTE *)&W1R);
	calc_rev_wheel((BYTE *) &W2, (BYTE *)&W2R);
	calc_rev_wheel((BYTE *) &W3, (BYTE *)&W3R);
	IW1 = 0;
	IW2 = 0;
	IW3 = 0;

	k_thread_access_grant(k_current_get(), &allforone);

	/*
	 * create an enc thread init the memory domain and add partitions
	 * then add the thread to the domain.
	 */
	tENC = k_thread_create(&enc_thread, enc_stack, STACKSIZE,
			(k_thread_entry_t)enc, NULL, NULL, NULL,
			-1, K_USER,
			K_FOREVER);
	k_thread_access_grant(tENC, &allforone);
	/* use K_FOREVER followed by k_thread_start*/
	printk("ENC Thread Created %p\n", tENC);

	ret = k_mem_domain_init(&enc_domain, ARRAY_SIZE(enc_parts), enc_parts);
	__ASSERT(ret == 0, "k_mem_domain_init() on enc_domain failed %d", ret);
	ARG_UNUSED(ret);

	printk("Partitions added to enc_domain\n");
	k_mem_domain_add_thread(&enc_domain, tENC);
	printk("enc_domain Created\n");


	tPT = k_thread_create(&pt_thread, pt_stack, STACKSIZE,
			(k_thread_entry_t)pt, NULL, NULL, NULL,
			-1, K_USER,
			K_FOREVER);
	k_thread_access_grant(tPT, &allforone);
	printk("PT Thread Created %p\n", tPT);

	ret = k_mem_domain_init(&pt_domain, ARRAY_SIZE(pt_parts), pt_parts);
	__ASSERT(ret == 0, "k_mem_domain_init() on pt_domain failed %d", ret);

	k_mem_domain_add_thread(&pt_domain, tPT);
	printk("pt_domain Created\n");

	tCT = k_thread_create(&ct_thread, ct_stack, STACKSIZE,
			(k_thread_entry_t)ct, NULL, NULL, NULL,
			-1, K_USER,
			K_FOREVER);
	k_thread_access_grant(tCT, &allforone);
	printk("CT Thread Created %p\n", tCT);
	/* Re-using the default memory domain for CT */
	ret = k_mem_domain_add_partition(&k_mem_domain_default, &ct_part);
	if (ret != 0) {
		printk("Failed to add ct_part to mem domain (%d)\n", ret);
		k_oops();
	}
	printk("ct partitions installed\n");

	ret = k_mem_domain_add_partition(&k_mem_domain_default, &blk_part);
	if (ret != 0) {
		printk("Failed to add blk_part to mem domain (%d)\n", ret);
		k_oops();
	}
	printk("blk partitions installed\n");

	k_thread_start(&enc_thread);
	/* need to start all three threads.  let enc go first to perform init step */

	printk("ENC thread started\n");
	k_thread_start(&pt_thread);
	printk("PT thread started\n");

	k_thread_start(&ct_thread);
	k_sem_give(&allforone);
	printk("CT thread started\n");
}



/*
 * The enc thread.
 * Function: initialize the the simulation of the wheels.
 * Copy memory from pt thread and encrypt to a local buffer
 * then copy to the ct thread.
 */
void enc(void)
{

	int index, index_out;

	while (1) {
		k_sem_take(&allforone, K_FOREVER);
		if (fBUFIN == 1) { /* 1 is process text */
			printk("ENC Thread Received Data\n");
			/* copy message form shared mem and clear flag */
			memcpy((void *)&enc_pt, (void *)BUFIN, SAMP_BLOCKSIZE);
			printk("ENC PT MSG: %s\n", (char *)&enc_pt);
			fBUFIN = 0;
			/* reset wheel: probably better as a flag option  */
			IW1 = 7;
			IW2 = 2;
			IW3 = 3;
			/* encode */
			memset((void *)&enc_ct, 0, SAMP_BLOCKSIZE);
			for (index = 0, index_out = 0; index < SAMP_BLOCKSIZE; index++) {
				if (enc_pt[index] == '\0') {
					enc_ct[index_out] = '\0';
					break;
				}
				if (enc_pt[index] >= 'a' && enc_pt[index] <= 'z') {
					enc_ct[index_out] = (BYTE)enig_enc((BYTE) enc_pt[index]);
					index_out++;
				}
			}
			/* test for CT flag */
			while (fBUFOUT != 0) {
				k_sleep(K_MSEC(1));
			}
			/* ct thread has cleared the buffer */
			memcpy((void *)&BUFOUT, (void *)&enc_ct,
			       SAMP_BLOCKSIZE);
			fBUFOUT = 1;

		}
		k_sem_give(&allforone);
	}
}

/*
 * the pt function pushes data to the enc thread.
 * It can be extended to receive data from a serial port
 * and pass the data to enc
 */
void pt(void)
{

	k_sleep(K_MSEC(20));
	while (1) {
		k_sem_take(&allforone, K_FOREVER);
		if (fBUFIN == 0) { /* send message to encode */
			printk("\nPT Sending Message 1\n");
			memset((void *)&BUFIN, 0, SAMP_BLOCKSIZE);
			memcpy((void *)&BUFIN, (void *)&ptMSG, sizeof(ptMSG));
/* strlen should not be used if user provided data, needs a max length set  */
			fBUFIN = 1;
		}
		k_sem_give(&allforone);
		k_sem_take(&allforone, K_FOREVER);
		if (fBUFIN == 0) { /* send message to decode  */
			printk("\nPT Sending Message 1'\n");
			memset((void *)&BUFIN, 0, SAMP_BLOCKSIZE);
			memcpy((void *)&BUFIN, (void *)&ptMSG2, sizeof(ptMSG2));
			fBUFIN = 1;
		}
		k_sem_give(&allforone);
		k_sleep(K_MSEC(50));
	}
}

/*
 * CT waits for fBUFOUT = 1 then copies
 * the message clears the flag and prints
 */
void ct(void)
{

	char tbuf[60];

	while (1) {
		k_sem_take(&allforone, K_FOREVER);
		if (fBUFOUT == 1) {
			printk("CT Thread Received Message\n");
			memset((void *)&tbuf, 0, sizeof(tbuf));
			memcpy((void *)&tbuf, (void *)BUFOUT, SAMP_BLOCKSIZE);
			fBUFOUT = 0;
			printk("CT MSG: %s\n", (char *)&tbuf);
		}
		k_sem_give(&allforone);
	}
}
