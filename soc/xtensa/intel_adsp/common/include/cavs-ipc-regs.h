/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_CAVS_IPC_REGS_H
#define ZEPHYR_SOC_INTEL_ADSP_CAVS_IPC_REGS_H

#include <stdint.h>

/* Inter Processor Communication: a distressingly heavyweight method
 * for directing interrupts at software running on other processors.
 * Used for sending interrupts to and receiving them from another
 * device.  cAVS uses it to talk to the host and the CSME.  In general
 * there is one of these blocks instantiated for each endpoint of a
 * connection.
 *
 * There are actually three (!) interrupt channels on the link, all
 * bidirectional, but they're somewhat nonorthogonal.  Each can be
 * independently masked via the CTL register.
 *
 * 1. When the IDR register is written with the high ("BUSY") bit set,
 *    the IDR value written is copied to the TDR register on the other
 *    end of the connection, where the BUSY bit acts as a
 *    level-triggered interrupt latch (and must therefore be cleared
 *    by the handler by writing a 1).  The high bit ("DONE") also
 *    becomes set on the TDA register of the other endpoint.
 *    Additionally, the IDD register is copied to the TDD register on
 *    the other side.  That data, and the remaining 31 bits of
 *    IDR/TDR, are uninspected by the hardware and are intended to be
 *    used as a "message" (but see below).
 *
 * 2. Writes to TDA on the receiving end of the connection with the
 *    high bit ("DONE") set to zero and (!) when the IDR BUSY bit is
 *    one on the other side of the connection (i.e. "when the other
 *    side has sent a message") cause an interrupt to be triggered on
 *    the other endpoint by setting its IDA high bit (which must be
 *    cleared to acknowledge the interrupt, again by writing a one
 *    bit).  At the same time, the peer's IDR.BUSY is cleared.  No
 *    data is copied with this interrupt mechanism.
 *
 * 3. When the CST register is written with a non-zero value, the
 *    value is copied to the CSR register on other side and an
 *    interrupt is triggered (to be cleared by writing CSR to zero).
 *    This is historically unused and unsupported by simulation tools.
 *
 * Note that while the scheme in #1 may look like an atomic message,
 * there is no queuing in the hardware!  This is just a regular level
 * triggered interrupt with some scratch registers on which multiple
 * contexts can race.  The design intent is that the sender polls the
 * IDR.BUSY bit (or waits for a return interrupt via #2) to be sure
 * the link is available.  And even then two independent initiators
 * (cores, threads, whatever) trying to initiate a message may race if
 * they have access to the same endpoint.  Use with care.
 *
 * Note also that the ancestral cAVS 1.5 version of this interface is
 * very similar (the registers had different names, but that's been
 * unified here), but with a different layout and with DONE signaled
 * via bit 30 of the TDD register when BUSY is cleared in TDR on the
 * other side and not via a separate TDA/IDA.  This means that it's
 * not possible to defer completion of a message on cAVS 1.5 as the
 * interrupt would remain active.
 */
struct cavs_ipc {
#ifdef CONFIG_SOC_SERIES_INTEL_CAVS_V15
	uint32_t tdr;
	uint32_t tdd;
	uint32_t idr;
	uint32_t idd;
	uint32_t ctl;
	uint32_t ida; /* Fakes for source compatibility; these ...  */
	uint32_t tda; /* ... two registers don't exist in hardware. */
#else
	uint32_t tdr;
	uint32_t tda;
	uint32_t tdd;
	uint32_t unused0;
	uint32_t idr;
	uint32_t ida;
	uint32_t idd;
	uint32_t unused1;
	uint32_t cst;
	uint32_t csr;
	uint32_t ctl;
#endif
};

#define CAVS_IPC_BUSY BIT(31)
#define CAVS_IPC_DONE BIT(31)
#define CAVS_IPC_IDD15_DONE BIT(30) /* v1.5 way to signal done */

#define CAVS_IPC_CTL_TBIE BIT(0)
#define CAVS_IPC_CTL_IDIE BIT(1) /* yesthatiswhattheyreallynamedit */

#endif /* ZEPHYR_SOC_INTEL_ADSP_CAVS_IPC_REGS_H */
