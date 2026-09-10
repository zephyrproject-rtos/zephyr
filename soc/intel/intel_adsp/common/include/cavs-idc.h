/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_CAVS_IDC_H_
#define ZEPHYR_SOC_INTEL_ADSP_CAVS_IDC_H_

#include <intel_adsp_ipc_devtree.h>

/*
 * (I)ntra (D)SP (C)ommunication is the facility for sending
 * interrupts directly between DSP cores.  The interface
 * is... somewhat needlessly complicated.
 *
 * Each core has a set of registers its is supposed to use, but all
 * registers seem to behave symmetrically regardless of which CPU does
 * the access.
 *
 * Each core has a "ITC" register associated with each other core in
 * the system (including itself).  When the high bit becomes 1 in an
 * ITC register, an IDC interrupt is latched for the target core.
 * Data in other bits is stored but otherwise ignored, it's merely
 * data to be transmitted along with the interrupt.
 *
 * On the target core, there is a "TFC" register for each core that
 * reflects the same value written to ITC.  In fact experimentally
 * these seem to be the same register at different addresses.  When
 * the high bit of TFC is written with a 1, the value becomes ZERO,
 * indicating an acknowledgment of the interrupt.  This action can
 * also latch an interrupt to send back to the originator if unmasked
 * (see below).
 *
 * (There is also an IETC/TEFC register pair that stores 30 bits of
 * data but otherwise has no hardware behavior.  This is probably best
 * ignored for new protocols, as experimentally it seems to provide no
 * performance benefit vs. storing a message in RAM.  The cAVS 1.5/1.8
 * ROM boot protocol uses it to store an entry point address, though.)
 *
 * So you can send a synchronous message from core "src" (where src is
 * the PRID of the CPU, equal to arch_curr_cpu()->id in Zephyr) to
 * core "dst" with:
 *
 *     IDC[src].core[dst].itc = BIT(31) | message;
 *     while (IDC[src].core[dst].itc & BIT(31)) {}
 *
 * And the other side (on cpu "dst", generally in the IDC interrupt
 * handler) will read and acknowledge those same values via:
 *
 *     uint32_t my_msg = IDC[dst].core[src].tfc & 0x7fffffff;
 *     IDC[dst].core[src].tfc = BIT(31); // clear high bit to signal completion
 *
 * And for clarity, at all times and for all cores and all pairs of src/dst:
 *
 *     IDC[src].core[dst].itc == IDC[dst].core[src].tfc
 *
 * Finally note the two control registers at the end of each core's
 * register block, which store a bitmask of cores that it is allowed
 * to signal with an interrupt via either ITC (set high "BUSY" bit) or
 * TFC (clear high "DONE" bit).  This masking is in ADDITION to the
 * level 2 bit for IDC in the per-core INTCTRL DSP register AND the
 * Xtensa architectural INTENABLE SR.  You must enable IDC interrupts
 * form core "src" to core "dst" with:
 *
 *     IDC[src].busy_int |= BIT(dst)  // Or disable with "&= ~BIT(dst)" of course
 */
struct cavs_idc {
	struct {
		uint32_t tfc;  /* (T)arget (F)rom (C)ore  */
		uint32_t tefc; /*  ^^ + (E)xtension       */
		uint32_t itc;  /* (I)nitiator (T)o (C)ore */
		uint32_t ietc; /*  ^^ + (E)xtension       */
	} core[4];
	uint32_t unused0[4];
	uint8_t  busy_int;     /* bitmask of cores that can IDC via ITC */
	uint8_t  done_int;     /* bitmask of cores that can IDC via TFC */
	uint8_t  unused1;
	uint8_t  unused2;
	uint32_t unused3[11];
};

#define IDC ((volatile struct cavs_idc *)INTEL_ADSP_IDC_REG_ADDRESS)

extern void soc_idc_init(void);

/* cAVS interrupt mask bits.  Each core has one of these structs
 * indexed in the intctrl[] array.  Each external interrupt source
 * indexes one bit in one of the state substructs (one each for Xtensa
 * level 2-5 interrupts).  The "mask" field shows the current masking
 * state, with a 1 representing "interrupt disabled".  The "status"
 * field indicates interrupts that are currently latched and awaiting
 * delivery.  Write bits to "set" to set the mask bit to 1 and disable
 * interrupts.  Write a 1 bit to "clear" to force the mask bit to 0
 * and enable them.  For example, for core "c":
 *
 *     INTCTRL[c].l2.clear = 0x10;     // unmask IDC interrupt
 *
 *     INTCTRL[c].l3.set = 0xffffffff; // Mask all L3 interrupts
 *
 * Note that this interrupt controller is separate from the Xtensa
 * architectural interrupt hardware controlled by the
 * INTENABLE/INTERRUPT/INTSET/INTCLEAR special registers on each core
 * which much also be configured for interrupts to arrive.  Note also
 * that some hardware (like IDC, see above) implements a third (!)
 * layer of interrupt masking.
 */
struct cavs_intctrl {
	struct {
		uint32_t set, clear, mask, status;
	} l2, l3, l4, l5;
};

/* Named interrupt bits in the above registers */
#define CAVS_L2_HPGPDMA    BIT(24)	/* HP General Purpose DMA */
#define CAVS_L2_DWCT1      BIT(23)	/* DSP Wall Clock Timer 1 */
#define CAVS_L2_DWCT0      BIT(22)	/* DSP Wall Clock Timer 0 */
#define CAVS_L2_L2ME       BIT(21)	/* L2 Memory Error */
#define CAVS_L2_DTS        BIT(20)	/* DSP Timestamping */
#define CAVS_L2_SHA        BIT(16)	/* SHA-256 */
#define CAVS_L2_DCLC       BIT(15)	/* Demand Cache Line Command */
#define CAVS_L2_IDC        BIT(8)	/* IDC */
#define CAVS_L2_HIPC       BIT(7)	/* Host IPC */
#define CAVS_L2_MIPC       BIT(6)	/* CSME IPC */
#define CAVS_L2_PIPC       BIT(5)	/* PMC IPC */
#define CAVS_L2_SIPC       BIT(4)	/* Sensor Hub IPC */

#define CAVS_L3_DSPGCL     BIT(31)	/* DSP Gateway Code Loader */
#define CAVS_L3_DSPGHOS(n) BIT(16 + n)	/* DSP Gateway Host Output Stream */
#define CAVS_L3_HPGPDMA    BIT(15)	/* HP General Purpose DMA */
#define CAVS_L3_DSPGHIS(n) BIT(n)	/* DSP Gateway Host Input Stream */

#define CAVS_L4_DSPGLOS(n) BIT(16 + n)	/* DSP Gateway Link Output Stream */
#define CAVS_L4_LPGPGMA    BIT(15)	/* LP General Purpose DMA */
#define CAVS_L4_DSPGLIS(n) BIT(n)	/* DSP Gateway Link Input Stream */

#define CAVS_L5_LPGPDMA    BIT(16)	/* LP General Purpose DMA */
#define CAVS_L5_DWCT1      BIT(15)	/* DSP Wall CLock Timer 1 */
#define CAVS_L5_DWCT0      BIT(14)	/* DSP Wall Clock Timer 0 */
#define CAVS_L5_DMIX       BIT(13)	/* Digital Mixer */
#define CAVS_L5_ANC        BIT(12)	/* Active Noise Cancellation */
#define CAVS_L5_SNDW       BIT(11)	/* SoundWire */
#define CAVS_L5_SLIM       BIT(10)	/* Slimbus */
#define CAVS_L5_DSPK       BIT(9)	/* Digital Speaker */
#define CAVS_L5_DMIC       BIT(8)	/* Digital Mic */
#define CAVS_L5_I2S(n)     BIT(n)	/* I2S */

#define CAVS_INTCTRL \
	((volatile struct cavs_intctrl *)DT_REG_ADDR(DT_NODELABEL(cavs_intc0)))

#endif /* ZEPHYR_SOC_INTEL_ADSP_CAVS_IDC_H_ */
