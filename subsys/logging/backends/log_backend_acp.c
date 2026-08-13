/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <string.h>
#include <acp7x_chip_reg.h>
#include <acp7x_fw_scratch_mem.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/cache.h>

/*
 * All mutable backend state lives in this single instance instead of
 * scattered module globals.  The Zephyr primitives that must be file
 * scope (spinlock, work, timer, staging buffer) are kept separate below.
 */
struct acp_log_backend {
	uint32_t write_off;   /* bytes staged by the log producer (wraps at 2^32) */
	/* local-read cursor: bytes drained from staging and DMA'd to the host */
	uint32_t read_off;
	uint32_t format;      /* active log_output format                          */
	bool ready;           /* backend initialized                               */
	atomic_t flushing;    /* a DMA flush is in progress (deferred mode)        */
};

static struct acp_log_backend acp_log;
static struct k_spinlock lock;

#define ACP_LOG_LINE_BUF_SIZE   256
static uint8_t log_buf[ACP_LOG_LINE_BUF_SIZE];

/* Xtensa memory write barrier - commit pending stores to the bus. */
static inline void acp_log_mem_barrier(void)
{
#if defined(__XTENSA__)
	__asm__ volatile("memw" ::: "memory");
#else
	__asm__ volatile("" ::: "memory");
#endif
}

static void acp_log_console(const uint8_t *in, size_t length);

/*
 * Shared window geometry - identical in both modes.  This sizes the
 * host-visible DMA window and must match the host driver's LOG_HOST_BUFFER_SIZE.
 * It is unrelated to CONFIG_LOG_BUFFER_SIZE, which sizes the logging
 * subsystem frontend message buffer.
 */
#define LOG_POS_UPDATE_REG ACP_FUTURE_REG_ACLK_1
#define DSP_LOG_BUFFER_SIZE      0x2000U
#define LOG_HOST_BUFFER_SIZE      65536U
/* ACP NON Cache Window Base and Size registers addresses for the host log mapped buffer. */
#define ACP_LOG_APERTURE_BASE_REG 0x9fd00470U
#define ACP_LOG_APERTURE_SIZE_REG 0x9fd00474U
#define LOG_MAPPED_BUFFER 0x03000000U /* Host log buffer mapped window base. */

/* Publish a cumulative byte count to the pos_update. */
static void acp_log_publish_count(uint32_t count)
{
	io_reg_write(PU_REGISTER_BASE + LOG_POS_UPDATE_REG, count);
}

#if defined CONFIG_LOG_MODE_IMMEDIATE
#define PAGE_ENABLE		0x80000000U
#define ACP_LOG_BUF_START 0x83000000U  /* host log window mapped by DSP non-cache window */
#define ACP_LOG_WINDOW_SIZE (DSP_LOG_BUFFER_SIZE | PAGE_ENABLE)
#define ACP_LOG_WINDOW_BASE  (ACP_SYST_MEM_WINDOW + LOG_MAPPED_BUFFER)

#else /* deferred / minimal */

#define LOG_DMA_CH          9            /* ACP SYSHUB DMA channel          */
#define LOG_DSCR_IDX        (MAX_NUM_DMA_DESC_DSCR - 2U) /* two log descriptors */
#define LOG_HIGH_WATERMARK      ((DSP_LOG_BUFFER_SIZE * 1U) / 2U)  /* 50% full: early flush */
#define LOG_CRITICAL_WATERMARK  ((DSP_LOG_BUFFER_SIZE * 2U) / 3U) /* 66% full: force DMA */

#define LOG_DMA_FLUSH_PERIOD_US    25000U

/*
 * Channel-9 DMA completion interrupt.  The ACP interrupt controller
 * routes the aggregated DMA interrupt to IRQ_NUM_EXT_LEVEL3 (dma_intr_level=0),
 * while IPC is routed to IRQ_NUM_EXT_LEVEL5.
 */
#ifndef IRQ_NUM_EXT_LEVEL3
#define IRQ_NUM_EXT_LEVEL3       3
#endif
#define LOG_DMA_IOC_STAT1_CH9    23    /* ch9 IOC status/clear: INTR_STAT1 bit 23 (W1C) */
#define LOG_DMA_IOC_MASK1_CH9    0x2U  /* ch9 enable within dmaiocmask1[1:0] -> bit 18 */

static K_SEM_DEFINE(acp_log_dma_done, 0, 1);

/* Channel-9 completion ISR: stop the channel, W1C the IOC status, wake waiter. */
static void acp_log_dma_isr(const void *arg)
{
	acp_dma_cntl_0_t cntl;

	ARG_UNUSED(arg);

	/* Stop channel 9 and disable its IOC: removes the interrupt source. */
	cntl = (acp_dma_cntl_0_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CNTL_9);
	cntl.bits.dmachrun = 0;
	cntl.bits.dmachiocen = 0;
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_CNTL_9, cntl.u32all);

	/* Confirm the channel has actually stopped before clearing status. */
	for (uint32_t guard = 10000U;
	     (io_reg_read(PU_REGISTER_BASE + ACP_DMA_CH_STS) & BIT(LOG_DMA_CH)) && guard;
	     guard--) {
	}

	/* Write-1-to-clear the ch9 completion bit in ACP_DSP0_INTR_STAT1. */
	io_reg_write(PU_REGISTER_BASE + ACP_DSP0_INTR_STAT1, BIT(LOG_DMA_IOC_STAT1_CH9));

	k_sem_give(&acp_log_dma_done);
}

static void acp_log_flush_work_handler(struct k_work *work);
static K_WORK_DEFINE(acp_log_flush_work, acp_log_flush_work_handler);
#define ACP_LOG_FLUSH_WQ_PRIORITY  5   /* preemptible, lower than IPC (1) */
#define ACP_LOG_FLUSH_STACK_SIZE   2048
K_THREAD_STACK_DEFINE(acp_log_flush_stack, ACP_LOG_FLUSH_STACK_SIZE);
static struct k_work_q acp_log_flush_wq;

static void acp_log_timer_expiry(struct k_timer *timer);
static K_TIMER_DEFINE(acp_log_flush_timer, acp_log_timer_expiry, NULL);

static uint8_t log_dma_staging[DSP_LOG_BUFFER_SIZE];

/*
 * Protects the ring cursors (write_off / read_off) shared between the log
 * producer (acp_log_console) and the DMA flush path (worker / timer / panic).
 */
static struct k_spinlock ring_lock;

/* Bytes staged but not yet pushed to the host (wrap-safe). */
static inline uint32_t acp_log_pending(void)
{
	k_spinlock_key_t key = k_spin_lock(&ring_lock);
	uint32_t pending = acp_log.write_off - acp_log.read_off;

	k_spin_unlock(&ring_lock, key);
	return pending;
}

/* Program one SYSHUB DMA descriptor: SRAM staging -> host log window. */
static void acp_log_dma_descriptor(uint32_t idx, const uint8_t *src,
		uint32_t dst, uint32_t bytes, bool ioc)
{
	volatile acp_scratch_mem_config_t *scratch =
		(volatile acp_scratch_mem_config_t *)
		(PU_SCRATCH_REG_BASE + SCRATCH_REG_OFFSET);
	volatile acp_cfg_dma_descriptor_t *dscr =
		(volatile acp_cfg_dma_descriptor_t *)(scratch->acp_cfg_dma_descriptor);

	dscr[idx].src_addr = ((uint32_t)src & ACP_DRAM_ADDRESS_MASK) | ACP_SRAM;
	dscr[idx].dest_addr = dst;
	dscr[idx].trns_cnt.u32all = 0;
	dscr[idx].trns_cnt.bits.trns_cnt = bytes;
	dscr[idx].trns_cnt.bits.ioc = ioc ? 1U : 0U;
	dscr[idx].reserved = 0;
	sys_cache_data_flush_range((void *)(uintptr_t)src, bytes);
}

/*
 * Start the log DMA channel.  When can_block is set the caller is in a
 * sleepable context, so wait on the channel-9 completion interrupt; the
 * busy-wait poll then only serves as a fallback.  When can_block is
 * clear (panic / spinlock-held critical flush) IOC is left disabled and
 * completion is detected by polling the channel run-status only.
 */
static int acp_log_dma_run(uint32_t ndesc, bool can_block)
{
	acp_dma_cntl_0_t cntl;
	uint64_t deadline;
	uint32_t stop_guard = 10000U;

	/* Stop the channel, then point it at our descriptor slots. */
	cntl = (acp_dma_cntl_0_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CNTL_9);
	cntl.bits.dmachrun = 0;
	cntl.bits.dmachiocen = can_block ? 1U : 0U;
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_CNTL_9, cntl.u32all);

	/* Wait (bounded) for the channel to actually stop before reprogramming. */
	while ((io_reg_read(PU_REGISTER_BASE + ACP_DMA_CH_STS) & BIT(LOG_DMA_CH)) &&
	       --stop_guard) {
	}

	/* Re-arm the global limit each transfer; a kernel ACP reset can clear this. */
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_DESC_MAX_NUM_DSCR, 1U);
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_DSCR_STRT_IDX_9, LOG_DSCR_IDX);
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_DSCR_CNT_9, ndesc);
	/* Logs are low priority: do not let the log DMA preempt audio DMA. */
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_PRIO_9, 0);

	if (can_block) {
		k_sem_reset(&acp_log_dma_done);
	}

	cntl.bits.dmachrun = 1;
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_CNTL_9, cntl.u32all);

	if (can_block && k_sem_take(&acp_log_dma_done, K_USEC(500)) == 0) {
		return 0;
	}

	deadline = k_cycle_get_64() + k_us_to_cyc_ceil64(500);
	do {
		acp_dma_ch_sts_t sts = (acp_dma_ch_sts_t)io_reg_read(
			PU_REGISTER_BASE + ACP_DMA_CH_STS);

		if (!(sts.u32all & BIT(LOG_DMA_CH))) {
			return 0;
		}
	} while (k_cycle_get_64() <= deadline);
	/* Timed out: abort the in-flight DMA so future flushes don't race it. */
	cntl = (acp_dma_cntl_0_t)io_reg_read(PU_REGISTER_BASE + ACP_DMA_CNTL_9);
	cntl.bits.dmachrun = 0;
	cntl.bits.dmachiocen = 0;
	io_reg_write(PU_REGISTER_BASE + ACP_DMA_CNTL_9, cntl.u32all);
	for (uint32_t guard = 10000U;
	     (io_reg_read(PU_REGISTER_BASE + ACP_DMA_CH_STS) & BIT(LOG_DMA_CH)) && guard;
	     guard--) {
	}
	io_reg_write(PU_REGISTER_BASE + ACP_DSP0_INTR_STAT1, BIT(LOG_DMA_IOC_STAT1_CH9));

	return -ETIMEDOUT;
}

/*
 * Copy nbytes from the staging ring (starting at read_off) to the host
 * log window.  One descriptor is used, or two when the copy wraps past
 * the ring end (tail to the end, remainder from the start).
 */
static int acp_log_dma_copy_to_host(uint32_t staging_off, uint32_t host_off,
				    uint32_t nbytes, bool can_block)
{
	const uint32_t dst_base = ACP_SYST_MEM_WINDOW + LOG_MAPPED_BUFFER;
	uint32_t first, ndesc = 1U;
	uint32_t staging_remain, host_remain;

	/* Bounds check */
	if (staging_off >= DSP_LOG_BUFFER_SIZE || host_off >= LOG_HOST_BUFFER_SIZE ||
		nbytes == 0U || nbytes > DSP_LOG_BUFFER_SIZE) {
		return -EINVAL;
	}

	/* Space available before each buffer wraps */
	staging_remain = DSP_LOG_BUFFER_SIZE - staging_off;
	host_remain = LOG_HOST_BUFFER_SIZE - host_off;

	/* First chunk limited by BOTH buffer boundaries - whichever wraps first */
	first = MIN(nbytes, MIN(staging_remain, host_remain));

	/* DMA first chunk: staging[staging_off] -> host[host_off] */
	acp_log_dma_descriptor(LOG_DSCR_IDX, log_dma_staging + staging_off,
		dst_base + host_off, first, can_block && first >= nbytes);

	/* If more data remains and a buffer wrapped, handle remainder */
	if (first < nbytes) {
		uint32_t remainder = nbytes - first;
		uint32_t new_staging_off = (first == staging_remain) ? 0 : staging_off + first;
		uint32_t new_host_off = (first == host_remain) ? 0 : host_off + first;

		acp_log_dma_descriptor(LOG_DSCR_IDX + 1U, log_dma_staging + new_staging_off,
			dst_base + new_host_off, remainder, can_block);
		ndesc = 2U;
	}
	acp_log_mem_barrier();

	return acp_log_dma_run(ndesc, can_block);
}

/*
 * Push all pending bytes to the host in a single DMA kick.  On overflow
 * the oldest bytes are dropped so at most one ring's worth is sent.
 */
static void acp_log_flush_to_host(bool can_block)
{
	k_spinlock_key_t key;
	uint32_t avail, read_off, buf_off, host_off, orig_read_off;

	/* Snapshot the ring cursors under the ring lock. */
	key = k_spin_lock(&ring_lock);
	orig_read_off = acp_log.read_off;
	read_off = acp_log.read_off;
	avail = acp_log.write_off - read_off;
	if (avail > DSP_LOG_BUFFER_SIZE) {
		/* Overflow: drop the oldest bytes, keep one ring's worth. */
		read_off = acp_log.write_off - DSP_LOG_BUFFER_SIZE;
		acp_log.read_off = read_off;
		avail = DSP_LOG_BUFFER_SIZE;
	}
	k_spin_unlock(&ring_lock, key);

	if (avail == 0U) {
		return;
	}

	/* Paranoid: ensure avail is clamped (should already be from above). */
	if (avail > DSP_LOG_BUFFER_SIZE) {
		avail = DSP_LOG_BUFFER_SIZE;
	}

	buf_off = read_off % DSP_LOG_BUFFER_SIZE;         /* Staging: wraps at 8KB */
	host_off = read_off % LOG_HOST_BUFFER_SIZE;   /* Host: wraps at 64KB */

	if (acp_log_dma_copy_to_host(buf_off, host_off, avail, can_block) != 0) {
		/*
		 * DMA timed out or errored: revert read_off update that happened
		 * in the overflow branch above so these bytes are retried on the
		 * next flush instead of being silently dropped.
		 */
		key = k_spin_lock(&ring_lock);
		/* Only revert if no other flush has advanced it further. */
		if (acp_log.read_off == read_off) {
			acp_log.read_off = orig_read_off;
		}
		k_spin_unlock(&ring_lock, key);
		return;
	}

	key = k_spin_lock(&ring_lock);
	acp_log.read_off = read_off + avail;
	acp_log_publish_count(acp_log.read_off);
	k_spin_unlock(&ring_lock, key);
}

/* Flush pending bytes, serialized so only one DMA runs at a time. */
static void acp_log_flush(bool can_block)
{
	if (atomic_cas(&acp_log.flushing, 0, 1)) {
		acp_log_flush_to_host(can_block);
		atomic_set(&acp_log.flushing, 0);
	}
}

static void acp_log_timer_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	k_work_submit_to_queue(&acp_log_flush_wq, &acp_log_flush_work);
}

static void acp_log_flush_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!acp_log.ready) {
		return;
	}

	acp_log_flush(true);

	/* Drain again if more was staged while the DMA ran. */
	if (acp_log_pending() != 0U) {
		k_work_submit_to_queue(&acp_log_flush_wq, &acp_log_flush_work);
	}
}

#endif /* CONFIG_LOG_MODE_IMMEDIATE */

static void acp_log_console(const uint8_t *in, size_t length)
{
	if (length == 0U || !acp_log.ready) {
		return;
	}

#if defined CONFIG_LOG_MODE_IMMEDIATE
	volatile uint8_t *win = (volatile uint8_t *)ACP_LOG_BUF_START;

	for (size_t i = 0; i < length; i++) {
		/* Write straight into the mapped window with wrap-around. */
		win[acp_log.write_off % DSP_LOG_BUFFER_SIZE] = in[i];
		acp_log.write_off++;
	}

	acp_log_mem_barrier();

	/* Publish the new cumulative count so the host driver picks it up. */
	acp_log_publish_count(acp_log.write_off);
#else
	uint32_t avail;
	k_spinlock_key_t key = k_spin_lock(&ring_lock);

	for (size_t i = 0; i < length; i++) {
		/* Stage locally; the DMA path drains it to the mapped window. */
		log_dma_staging[acp_log.write_off % DSP_LOG_BUFFER_SIZE] = in[i];
		acp_log.write_off++;
	}
	avail = acp_log.write_off - acp_log.read_off;

	k_spin_unlock(&ring_lock, key);

	/*
	 * Watermark checks run outside ring_lock; acp_log_console is called under
	 * the log_output 'lock' spinlock, and the flush path only takes
	 * ring_lock, so there is no recursive locking here.
	 */
	if (avail >= LOG_CRITICAL_WATERMARK) {
		acp_log_flush(false);
	}
	if (avail >= LOG_HIGH_WATERMARK) {
		k_work_submit_to_queue(&acp_log_flush_wq, &acp_log_flush_work);
	}
#endif
}

static void init(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);

	acp_log.write_off = 0U;
	acp_log.format = CONFIG_LOG_BACKEND_ACP_OUTPUT_DEFAULT;
	acp_log.ready = false;

	io_reg_write(PU_REGISTER_BASE + ACP_DMA_DESC_MAX_NUM_DSCR, 1U);
#if defined CONFIG_LOG_MODE_IMMEDIATE
	io_reg_write(ACP_LOG_APERTURE_BASE_REG, ACP_LOG_WINDOW_BASE);
	io_reg_write(ACP_LOG_APERTURE_SIZE_REG, ACP_LOG_WINDOW_SIZE);
	acp_log_publish_count(acp_log.write_off);   /* publish 0 so the host starts in sync */
	acp_log.ready = true;

	const char *msg1 = "ACP Log Backend Initialized (immediate, GRP16 direct)\n";
#else
	acp_log.read_off = 0U;
	memset(log_dma_staging, 0, sizeof(log_dma_staging));
	acp_log_publish_count(acp_log.read_off);   /* publish 0 so the host starts in sync */

	/* Unmask the channel-9 IOC and hook its ISR (routed to EXT_LEVEL3). */
	acp_dsp0_intr_cntl_t intr_cntl =
		(acp_dsp0_intr_cntl_t)io_reg_read(PU_REGISTER_BASE + ACP_DSP0_INTR_CNTL);
	intr_cntl.bits.dmaiocmask1 |= LOG_DMA_IOC_MASK1_CH9;
	io_reg_write(PU_REGISTER_BASE + ACP_DSP0_INTR_CNTL, intr_cntl.u32all);

	IRQ_CONNECT(IRQ_NUM_EXT_LEVEL3, 0, acp_log_dma_isr, NULL, 0);
	irq_enable(IRQ_NUM_EXT_LEVEL3);

	k_work_queue_init(&acp_log_flush_wq);
	k_work_queue_start(&acp_log_flush_wq, acp_log_flush_stack,
		K_THREAD_STACK_SIZEOF(acp_log_flush_stack),
		ACP_LOG_FLUSH_WQ_PRIORITY, NULL);

	k_timer_start(&acp_log_flush_timer,
		K_USEC(LOG_DMA_FLUSH_PERIOD_US),
		K_USEC(LOG_DMA_FLUSH_PERIOD_US));
	/* ready flag set after IRQ init */
	acp_log.ready = true;

	const char *msg1 = "ACP Log Backend Initialized!\n";
#endif
	acp_log_console((const uint8_t *)msg1, strlen(msg1));

#if defined CONFIG_LOG_MODE_IMMEDIATE
	const char *msg2 = "ACP Log Mode: IMMEDIATE\n";
#elif defined(CONFIG_LOG_MODE_DEFERRED)
	const char *msg2 = "ACP Log Mode: DEFERRED\n";
#elif defined(CONFIG_LOG_MODE_MINIMAL)
	const char *msg2 = "ACP Log Mode: MINIMAL\n";
#else
	const char *msg2 = "ACP Log Mode: UNKNOWN\n";
#endif
	acp_log_console((const uint8_t *)msg2, strlen(msg2));
}

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(ctx);
	acp_log_console(data, length);
	return length;
}

LOG_OUTPUT_DEFINE(log_output_acp, char_out, log_buf, sizeof(log_buf));

static uint32_t format_flags(void)
{
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	return flags;
}

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_LOG_MODE_MINIMAL)
static void acp_log_panic_flush(void);
#endif

static void panic(struct log_backend const *const backend)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	log_backend_std_panic(&log_output_acp);

	k_spin_unlock(&lock, key);

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_LOG_MODE_MINIMAL)
	/* Push the just-formatted staging-ring bytes to the host over DMA. */
	acp_log_panic_flush();
#endif
}

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_LOG_MODE_MINIMAL)
/*
 * Force a DMA flush on panic: the work queue is dead and IRQs are off, so
 * push staged log bytes to the host directly (busy-wait DMA is safe on
 * single-core Xtensa).  Bypasses the flushing guard on purpose.  Called
 * from the backend panic() handler after log_backend_std_panic() has
 * formatted any pending messages into the staging ring.
 */
static void acp_log_panic_flush(void)
{
	if (!acp_log.ready) {
		return;
	}

	acp_log_flush_to_host(false);
}

#endif /* !CONFIG_LOG_MODE_IMMEDIATE || CONFIG_LOG_MODE_MINIMAL */

static inline void dropped(const struct log_backend *const backend,
		uint32_t cnt)
{
	log_output_dropped_process(&log_output_acp, cnt);
}

static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	log_format_func_t log_output_func = log_format_func_t_get(acp_log.format);

	k_spinlock_key_t key = k_spin_lock(&lock);

	log_output_func(&log_output_acp, &msg->log, format_flags());

	k_spin_unlock(&lock, key);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	acp_log.format = log_type;
	return 0;
}

const struct log_backend_api log_backend_acp_api = {
	.process = process,
	.dropped = (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ||
		IS_ENABLED(CONFIG_LOG_MODE_MINIMAL)) ? NULL : &dropped,
	.panic = panic,
	.format_set = format_set,
	.init = init,
};

LOG_BACKEND_DEFINE(log_backend_acp, log_backend_acp_api, true);
