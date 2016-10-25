/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * SoC Watch - QMSI power profiler
 */

#if (SOC_WATCH_ENABLE) && (!QM_SENSOR)

#include <x86intrin.h>
#include "qm_common.h"
#include "qm_soc_regs.h"
#include "soc_watch.h"

/*
 * Define a macro for exposing some functions and other definitions
 * only when unit testing.  If we're not unit testing, then declare
 * them as static, so that their declarations are hidden to normal
 * code.
 */
#if (UNIT_TEST)
#define NONUTSTATIC
#else
#define NONUTSTATIC static
#endif

/**
 * "Event strings" table, describing message structures.
 * The first character is the event code to write to the data file.
 * The 2nd and subsequent characters describe how to format the record's
 * data.  Note that the ordering here needs to agree with the
 * enumeration list in qmsw_stub.h.
 *
 * Table characters:
 * + First char = event code to write into the result file.
 * + T = TSC Timestamp  (Hi-res timestamp)
 * + t = RTC Timestamp  (lo-res timestamp)
 * + 1 = interpret ev_data as a 1-byte value
 * + 4 = interpret ev_data as a 4-byte value
 * + R = Using ev_data as a register enumeration, read that register,
 * +	   and put that 4-byte value into the data file.
 * + L = Trigger an RTC timestamp Later
 */
NONUTSTATIC const char *ev_strs[] = {
    "HT",   /* Halt event */
    "IT1",  /* Interrupt event */
    "STtL", /* Sleep event */
    "RT1R", /* Register read event: Timestamp, reg enum, reg value*/
    "UTs4", /* User event: timestamp, subtype, data value. */
};

/*
 * This list of registers corresponds to the SoC Watch register ID
 * enumeration in soc_watch.h, and MUST STAY IN AGREEMENT with that
 * list, since that enumeration is used to index this list.
 *
 * To record a register value, the SoC Watch code indexes into this
 * array, and reads the corresponding address found in that slot.
 */
#if (QUARK_D2000)
static const uint32_t *platform_regs[] = {
    (uint32_t *)(&QM_SCSS_CCU->osc0_cfg1),
    (uint32_t *)(&QM_SCSS_CCU->ccu_lp_clk_ctl),
    (uint32_t *)(&QM_SCSS_CCU->ccu_sys_clk_ctl),
    /* Clock Gating Registers */
    (uint32_t *)(&QM_SCSS_CCU->ccu_periph_clk_gate_ctl),
    (uint32_t *)(&QM_SCSS_CCU->ccu_ext_clock_ctl),
    /* Power Consumption regs */
    (uint32_t *)(&QM_SCSS_CMP->cmp_pwr),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_pullup),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_slew),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_in_en)};
#elif(QUARK_SE)
static const uint32_t *platform_regs[] = {
    (uint32_t *)(&QM_SCSS_CCU->osc0_cfg1),
    (uint32_t *)(&QM_SCSS_CCU->ccu_lp_clk_ctl),
    (uint32_t *)(&QM_SCSS_CCU->ccu_sys_clk_ctl),
    /* Clock Gating Registers */
    (uint32_t *)(&QM_SCSS_CCU->ccu_periph_clk_gate_ctl),
    (uint32_t *)(&QM_SCSS_CCU->ccu_ss_periph_clk_gate_ctl),
    (uint32_t *)(&QM_SCSS_CCU->ccu_ext_clock_ctl),
    /* Power Consumption regs */
    (uint32_t *)(&QM_SCSS_CMP->cmp_pwr), (uint32_t *)(&QM_SCSS_PMU->slp_cfg),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_pullup),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_pullup[1]),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_pullup[2]),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_pullup[3]),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_slew),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_slew[1]),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_slew[2]),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_slew[3]),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_in_en),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_in_en[1]),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_in_en[2]),
    (uint32_t *)(&QM_SCSS_PMUX->pmux_in_en[3])};
#endif /* QUARK_SE */

/* Define VERBOSE to turn on printf-based logging */
#ifdef VERBOSE
#define SOC_WATCH_TRACE QM_PRINTF
#else
#define SOC_WATCH_TRACE(...)
#endif

/*
 * mlog routines -- low-level memory debug logging.
 * Only enable if there's a need to debug this module.
 */
#ifdef MLOG_ENABLE
#define MLOG(e) mlog(e)
#define MLOG_BYTE(b) mlog_byte(b)
#define MLOG_SIZE 512 /* Must be a power of 2 */
static uint8_t mlog_events[MLOG_SIZE];
static uint16_t mlog_idx = 0;
void mlog(uint8_t event)
{
	mlog_events[++mlog_idx % (MLOG_SIZE)] = event;
}
void mlog_byte(uint8_t byte)
{
	const char c[] = {"0123456789ABCDEF"};
	MLOG(c[byte >> 4]);
	MLOG(c[byte & 4]);
}
#else /* !MLOG_ENABLE */
#define MLOG(event)
#define MLOG_BYTE(b)
#endif /* !MLOG_ENABLE */

/*
 * CONFIGURABLE: Set this to control the number of bytes of RAM you
 *     want to dedicate to event buffering.  The larger the buffer,
 *     the fewer (expensive) flushes we will have to do.  The smaller,
 *     the lower the memory cost, but the more flushes you will do.
 */
#define SOC_WATCH_EVENT_BUFFER_SIZE (256) /* Measured in bytes */

/**
 * Power profiling event data buffer.  Symbol must be globally
 * visible, so that it can be seen by external tools.
 */
struct sw_profiling_event_buffer {
	uint8_t eb_idx;  /* Index of next byte to be written */
	uint8_t eb_size; /* Buffer size == SOC_WATCH_EVENT_BUFFER_SIZE */
	uint8_t event_data[SOC_WATCH_EVENT_BUFFER_SIZE - 2]; /* Event data -
							   sizeof(idx + size) */
} soc_watch_event_buffer = {0, SOC_WATCH_EVENT_BUFFER_SIZE - 1, {0}};

/* High water mark, i.e. "start trying to flush" point. */
#define SW_EB_HIGH_WATER (((SOC_WATCH_EVENT_BUFFER_SIZE - 2) * 7) >> 3)

NONUTSTATIC int soc_watch_buffer_full(void)
{
	return (soc_watch_event_buffer.eb_idx >= SW_EB_HIGH_WATER);
}

/**
 * Flag used by the JTAG data extraction routine. During setup, a HW
 * watchpoint is placed on this address.  During the flush routine,
 * software writes to it, causing the HW watchpoint to fire, and
 * OpenOCD to extract the data.  This symbol MUST be globally visible
 * in order for JTAG data transfer to work.
 */
volatile uint8_t soc_watch_flush_flag = 0;

/*
 * soc_watch_event_buffer_flush -- Trigger the data buffer flush.
 */
static void soc_watch_event_buffer_flush(void)
{
/* Figure out if we can successfully flush the data out.
 * If we're sleeping, the JTAG query will fail.
 */
#if (QUARK_D2000)
	/**
	 * If the "we're going to sleep" bit is set, parts of the
	 * SOC are already asleep, and transferring data over
	 * the JTAG port is not always reliable. So defer transferring
	 * the data until later.
	 * @TODO: Determine if there is also a sensitivity to the
	 * clock rate.
	 */
	MLOG('F');
	if (QM_SCSS_PMU->aon_vr & QM_AON_VR_VREG_SEL) {
		MLOG('-');
		return; /* We would only send junk, so don't flush.	 */
	}
#endif

	soc_watch_flush_flag = 1; /* Trigger the data extract brkpt */
	soc_watch_event_buffer.eb_idx = 0;
	MLOG('+');
}

/* Store a byte in the event buffer. */
static void eb_write_char(uint8_t data)
{
	SOC_WATCH_TRACE("c%d:0x%x [0]=%x\n", soc_watch_event_buffer.eb_idx,
			data, soc_watch_event_buffer.event_data[0]);
	soc_watch_event_buffer.event_data[soc_watch_event_buffer.eb_idx++] =
	    data;
}

/* Store a word in the event buffer. */
static void eb_write_uint32(uint32_t *data)
{
	uint32_t *uip = (uint32_t *)&soc_watch_event_buffer
			    .event_data[soc_watch_event_buffer.eb_idx];
	*uip = *data;
	SOC_WATCH_TRACE("I%d:0x%x\n", soc_watch_event_buffer.eb_idx, *data);
	soc_watch_event_buffer.eb_idx += sizeof(uint32_t);
}

/* x86 CPU FLAGS.IF register field (Interrupt enable Flag, bit 9), indicating
 * whether or not CPU interrupts are enabled.
 */
#define X86_FLAGS_IF BIT(9)

/**
 * Save interrupt state and disable all interrupts on the CPU.
 * Defined locally for modularity when used in other contexts (i.e. RTOS)
 *
 * @return An architecture-dependent lock-out key representing the "interrupt
 * 	   disable state" prior to the call.
 */
static inline unsigned int soc_watch_irq_lock(void)
{
	unsigned int key = 0;

	/*
	 * Store the CPU FLAGS register into the variable `key' and disable
	 * interrupt delivery to the core.
	 */
	__asm__ __volatile__("pushfl;\n\t"
			     "cli;\n\t"
			     "popl %0;\n\t"
			     : "=g"(key)
			     :
			     : "memory");

	return key;
}

/**
 *
 * Restore previous interrupt state on the CPU saved via soc_watch_irq_lock().
 * Defined locally for modularity when used in other contexts (i.e. RTOS)
 *
 * @param[in] key architecture-dependent lock-out key returned by a previous
 * 		  invocation of soc_watch_irq_lock().
 */
static inline void soc_watch_irq_unlock(unsigned int key)
{
	/*
	 * `key' holds the CPU FLAGS register content at the time when
	 * soc_watch_irq_lock() was called.
	 */
	if (!(key & X86_FLAGS_IF)) {
		/*
		 * Interrupts were disabled when soc_watch_irq_lock() was invoked:
		 * do not re-enable interrupts.
		 */
		return;
	}

	/* Enable interrupts */
	__asm__ __volatile__("sti;\n\t" : :);
}

/* Log an event with one parameter. */
void soc_watch_log_event(soc_watch_event_t event_id, uintptr_t ev_data)
{
	soc_watch_log_app_event(event_id, 0, ev_data);
}

/*
 * Log an event with two parameters, where the subtype comes from
 * the user.  Note that what actually makes this an 'application event' is
 * the event_id, not the fact that it is coming in via this interface.
 */
void soc_watch_log_app_event(soc_watch_event_t event_id, uint8_t ev_subtype,
			     uintptr_t ev_data)
{
	static uint8_t record_rtc = 0;
	const uint32_t *rtc_ctr = (uint32_t *)&QM_RTC->rtc_ccvr;
	const char *cp;
	unsigned int irq_flag = 0;
	uint64_t tsc = __builtin_ia32_rdtsc(); /* Grab hi-res timestamp */
	uint32_t rtc_val = *rtc_ctr;

#define AVG_EVENT_SIZE 8 /* Size of a typical message in bytes. */

	MLOG('[');
	irq_flag = soc_watch_irq_lock();
	/* TODO: We know exactly how many bytes of storage we need,
	 * since we know the event code.  So don't do an "AVG" size thing
	 * here--use the exact size!
	 */
	if ((soc_watch_event_buffer.eb_idx + AVG_EVENT_SIZE) <=
	    soc_watch_event_buffer.eb_size) {

/* Map a halt event to a sleep event where appropriate. */
#if (QUARK_D2000)
		if (event_id == SOCW_EVENT_HALT) {
			if (QM_SCSS_PMU->aon_vr & QM_AON_VR_VREG_SEL) {
				event_id = SOCW_EVENT_SLEEP;
			}
		}
#endif

		/* Record the RTC of the waking event, if it's rousing us from
		 * sleep. */
		if (record_rtc) {
			eb_write_char('t');
			eb_write_uint32((uint32_t *)(&rtc_val)); /* Timestamp */
			record_rtc = 0;
		}

		if (event_id >= SOCW_EVENT_MAX) {
			SOC_WATCH_TRACE("Unknown event id: 0x%x\n", event_id);
			MLOG('?');
			soc_watch_irq_unlock(irq_flag);
			return;
		}
		cp = ev_strs[event_id]; /* Look up event string */
		SOC_WATCH_TRACE("%c", *cp);
		MLOG(*cp);
		eb_write_char(*cp); /* Write event code */
		while (*++cp) {
			switch (*cp) {
			case 'T':
				eb_write_uint32((uint32_t *)(&tsc)); /* Hi-res
						Timestamp */
				break;
			case 't':
				eb_write_uint32(
				    (uint32_t *)(&rtc_val)); /* Lo-res
					    Timestamp */
				break;
			case 'L':
				record_rtc = 1;
				break;
			case 'R': /* Register data value */
				eb_write_uint32(
				    (uint32_t *)platform_regs[ev_data]);
				break;
			case '4': /* 32-bit data value */
				eb_write_uint32((uint32_t *)&ev_data);
				break;
			case '1':
				/* Register ID */
				eb_write_char(((uint32_t)ev_data) & 0xff);
				break;
			case 's':
				/* Event subtype */
				eb_write_char(((uint32_t)ev_subtype) & 0xff);
				break;
			default:
				SOC_WATCH_TRACE(
				    "Unknown string char: 0x%x on string "
				    "0x%x\n",
				    *cp, event_id);
				break;
			}
		}
	}

	/*
	 * If this is an interrupt which roused the CPU out of a sleep state,
	 * don't flush the buffer.  (Due to a bug in OpenOCD, doing so will
	 * clear the HW watchpoint, ensuring no further flushes are seen by
	 * OpenOCD.)
	 */
	if ((soc_watch_buffer_full()) && (event_id != SOCW_EVENT_INTERRUPT)) {
		SOC_WATCH_TRACE("\n --- FLUSH: idx= %d ---\n",
				soc_watch_event_buffer.eb_idx);
		soc_watch_event_buffer_flush();
	}
	MLOG(':');
	MLOG_BYTE(soc_watch_event_buffer.eb_idx);
	soc_watch_irq_unlock(irq_flag);
	MLOG(']');
}

#endif /* !(defined(SOC_WATCH) && (!QM_SENSOR)) */
