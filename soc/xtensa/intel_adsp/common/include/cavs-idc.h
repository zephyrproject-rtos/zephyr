/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_CAVS_IDC_H_
#define ZEPHYR_SOC_INTEL_ADSP_CAVS_IDC_H_

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
 * And the other side (on cpu "dst", generally in the IDC interruupt
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
 * register block, which store a bitmask of cores that are allowed to
 * send that core an interrupt via either ITC (set high "BUSY" bit) or
 * TFC (clear high "DONE" bit).  This masking is in ADDITION to the
 * level 2 bit for IDC in the per-core INTCTRL DSP register AND the
 * Xtensa architectural INTENABLE SR.  You must enable IDC interrupts
 * form core "src" to core "dst" with:
 *
 *     IDC[dst].busy_int |= BIT(src)  // Or disable with "&= ~BIT(src)" of course
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

#define IDC ((volatile struct cavs_idc *)DT_REG_ADDR(DT_NODELABEL(idc)))

extern void soc_idc_init(void);

#endif /* ZEPHYR_SOC_INTEL_ADSP_CAVS_IDC_H_ */
