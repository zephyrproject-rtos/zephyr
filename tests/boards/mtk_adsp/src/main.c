/* Copyright 2023 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <soc.h>

/* Simple test of SOC-specific hardware on the MediaTek Audio DSP
 * family.  Right now just CPU speed and host mailbox interrupts.
 */

#define NOM_HZ CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC

static uint32_t ccount(void)
{
	uint32_t t;

	__asm__ volatile("rsr %0, CCOUNT" : "=r"(t));
	return t;
}

static uint32_t cpu_hz(void)
{
	uint32_t t0 = k_cycle_get_32(), cc0 = ccount();

	k_msleep(100);

	uint32_t t1 = k_cycle_get_32(), cc1 = ccount();
	uint32_t hz = ((uint64_t)cc1 - cc0) * NOM_HZ / (t1 - t0);

	printk("(measured %u Hz CPU clock vs. %d Hz timer)\n", hz, NOM_HZ);
	return hz;
}

ZTEST(mtk_adsp, cpu_freq)
{
#ifdef CONFIG_SOC_SERIES_MT8195
	int freqs[] = { 26, 370, 540, 720 };

	for (int i = 0; i < ARRAY_SIZE(freqs); i++) {
		printk("Checking CPU freq entry %d (expect %d MHz)\n", i, freqs[i]);
		mtk_adsp_set_cpu_freq(freqs[i]);

		/* Compute error as an inverse, i.e. "one part in":
		 * 100 means 1% off, 1000 is 0.1%, etc...
		 */
		uint32_t hz0 = 1000000 * freqs[i], hz = cpu_hz();
		int32_t err = hz0 / abs((int32_t)(hz0 - hz));

		zassert_true(err > 200);
	}
#else
	(void)cpu_hz();
#endif
}

#define MBOX0 DEVICE_DT_GET(DT_INST(0, mediatek_mbox))
#define MBOX1 DEVICE_DT_GET(DT_INST(1, mediatek_mbox))

K_SEM_DEFINE(mbox_sem, 0, 1);
static bool mbox1_fired;

static void mbox_fn(const struct device *mbox, void *arg)
{
	zassert_equal(mbox, MBOX1);
	zassert_equal(arg, NULL);
	mbox1_fired = true;
	k_sem_give(&mbox_sem);
}


/* Test in/out interrupts from the host.  This relies on a SOF driver
 * on the host, which has the behavior of "replying" with an interrupt
 * on mbox1 after receiving a "command" on mbox0 (you can also see it
 * whine about the invalid IPC message in the kernel logs).
 *
 * Note that there's a catch: on older kernels, SOF's "reply" comes
 * after a timeout (it's an invalid command, afterall) which is 165
 * seconds!  But the test does pass.
 */
ZTEST(mtk_adsp, mbox)
{
	mtk_adsp_mbox_set_handler(MBOX1, 1, mbox_fn, NULL);

	/* First signal the host with a reply on the second channel,
	 * that effects a reply to anything it thinks it might have
	 * sent us
	 */
	mtk_adsp_mbox_signal(MBOX1, 1);

	mtk_adsp_mbox_signal(MBOX0, 0);

	printk("Waiting for reply from SOF driver, be patient: long timeout...\n");
	k_sem_take(&mbox_sem, K_FOREVER);
	zassert_true(mbox1_fired);
}

static void *mtk_adsp_setup(void)
{
	return NULL;
}

ZTEST_SUITE(mtk_adsp, NULL, mtk_adsp_setup, NULL, NULL, NULL);
