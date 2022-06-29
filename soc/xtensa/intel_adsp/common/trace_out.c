/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/zephyr.h>
#include <soc.h>
#include <adsp_memory.h>
#include <zephyr/sys/winstream.h>

struct k_spinlock trace_lock;

static struct sys_winstream *winstream;

#define SOF_MTRACE_DESCRIPTOR_SIZE	12 /* 3 x u32 */
#define SOF_MTRACE_PAGE_SIZE		0x1000
#define SOF_MTRACE_SLOT_SIZE		SOF_MTRACE_PAGE_SIZE

static void mtrace_init(void)
{
	uint8_t *buf2 = z_soc_uncached_ptr((void *)HP_SRAM_WIN2_BASE);
	uint8_t *toffset = (uint8_t*)buf2 + 4;
	unsigned int boffset = SOF_MTRACE_SLOT_SIZE + 8;
	volatile uint32_t *type = (uint32_t*) toffset;

	/* FIXME: something zeros window2 after soc_trace_init(), so
	 * this has to be redone here ondemand until issue rootcaused */
	if (*type != 0x474f4c00) {
		*type = 0x474f4c00;

		winstream = sys_winstream_init(buf2 + boffset, HP_SRAM_WIN2_SIZE - boffset);
	}
}

static void mtrace_update_dsp_writeptr(void)
{
	uint8_t *buf = z_soc_uncached_ptr((void *)HP_SRAM_WIN2_BASE);
	unsigned int winstr_offset = SOF_MTRACE_SLOT_SIZE + 2 * sizeof(uint32_t);
	unsigned int dspptr_offset = SOF_MTRACE_SLOT_SIZE + sizeof(uint32_t);
	struct sys_winstream *winstream = (struct sys_winstream *)(buf + winstr_offset);
	uint32_t pos = winstream->end + 4 * sizeof(uint32_t);
	volatile uint32_t *dsp_ptr = (uint32_t*)(buf + dspptr_offset);

	/* copy write pointer maintained by winstream to correct
	 * place (as expected by driver) */
	*dsp_ptr = pos;
}

void intel_adsp_trace_out(int8_t *str, size_t len)
{
	/* FIXME: the WIN2 area gets overwritten/zeroed
	 *        some time after soc_trace_init(), so as
	 *        a stopgap, keep reinitializing the WIN2
	 *        headers */
	mtrace_init();

	if (len == 0) {
		return;
	}

#ifdef CONFIG_ADSP_TRACE_SIMCALL
	register int a2 __asm__("a2") = 4; /* SYS_write */
	register int a3 __asm__("a3") = 1; /* fd 1 == stdout */
	register int a4 __asm__("a4") = (int)str;
	register int a5 __asm__("a5") = len;

	__asm__ volatile("simcall" : "+r"(a2), "+r"(a3)
			 : "r"(a4), "r"(a5)  : "memory");
#endif
	k_spinlock_key_t key = k_spin_lock(&trace_lock);
	sys_winstream_write(winstream, str, len);
	mtrace_update_dsp_writeptr();
	k_spin_unlock(&trace_lock, key);
}


int arch_printk_char_out(int c)
{
	int8_t s = c;

	intel_adsp_trace_out(&s, 1);
	return 0;
}

void soc_trace_init(void)
{
	mtrace_init();
}
