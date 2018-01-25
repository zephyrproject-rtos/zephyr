/*
 * Copyright 2014-2015, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _M32C0_H_
#define _M32C0_H_

#ifndef _M64C0_H_
/* MIPS32-specific MMU interface */
#include <mips/m32tlb.h>
#endif
/*
 * MIPS32 Exception Codes
 */
#define EXC_INTR	0	/* interrupt */
#define EXC_MOD		1	/* tlb modification */
#define EXC_TLBL	2	/* tlb miss (load/i-fetch) */
#define EXC_TLBS	3	/* tlb miss (store) */
#define EXC_ADEL	4	/* address error (load/i-fetch) */
#define EXC_ADES	5	/* address error (store) */
#define EXC_IBE		6	/* bus error (i-fetch) */
#define EXC_DBE		7	/* data bus error (load/store) */
#define EXC_SYS		8	/* system call */
#define EXC_BP		9	/* breakpoint */
#define EXC_RI		10	/* reserved instruction */
#define EXC_CPU		11	/* coprocessor unusable */
#define EXC_OVF		12	/* integer overflow */
#define EXC_TRAP	13	/* trap exception */
#define EXC_MSAFPE	14	/* MSA floating point exception */
#define EXC_FPE		15	/* floating point exception */
#define EXC_IS1		16	/* implementation-specific 1 */
#define EXC_IS2		17	/* implementation-specific 2 */
#define EXC_C2E		18	/* coprocessor 2 exception */
#define EXC_TLBRI	19	/* TLB read inhibit */
#define EXC_TLBXI	20	/* TLB execute inhibit */
#define EXC_MSAU	21	/* MSA unusable exception */
#define EXC_MDMX	22	/* mdmx unusable */
#define EXC_WATCH	23	/* watchpoint */
#define EXC_MCHECK	24	/* machine check */
#define EXC_THREAD	25	/* thread */
#define EXC_DSPU	26	/* dsp unusable */
#define EXC_RES27	27
#define EXC_RES28	28
#define EXC_RES29	29
#define EXC_RES30	30
#define EXC_RES31	31


/*
 * MIPS32 Cause Register (CP0 Register 13, Select 0)
 */
#define CR_BD		0x80000000	/* branch delay */
#define  CR_BD_SHIFT		31
#define CR_TI		0x40000000	/* timer interrupt (r2) */
#define  CR_TI_SHIFT		30
#define CR_CEMASK	0x30000000      /* coprocessor used */
#define  CR_CESHIFT		28
#define  CR_CE_SHIFT		28
#define  CR_CE_BITS		 2
#define CR_DC		0x08000000	/* disable count (r2) */
#define  CR_DC_SHIFT		27
#define CR_PCI		0x04000000	/* performance counter i/u (r2) */
#define  CR_PCI_SHIFT		26
#define CR_IV		0x00800000	/* use special i/u vec */
#define  CR_IV_SHIFT		23
#define CR_WP		0x00400000	/* deferred watchpoint */
#define  CR_WP_SHIFT		22
#define CR_FDCI		0x00200000	/* fast debug channned i/u (r2) */
#define  CR_FDCI_SHIFT		21

#define CR_IMASK	0x0000ff00 	/* interrupt pending mask */
#define CR_IP_MASK	0x0000ff00
#define  CR_IP_SHIFT		 8
#define  CR_IP_BITS		 8
#define CR_RIPL		0x0000fc00
#define  CR_RIPL_SHIFT		10
#define  CR_RIPL_BITS		 6

/* interrupt pending bits */
#define CR_HINT5	0x00008000	/* h/w interrupt 5 */
#define CR_HINT4	0x00004000	/* h/w interrupt 4 */
#define CR_HINT3	0x00002000	/* h/w interrupt 3 */
#define CR_HINT2	0x00001000	/* h/w interrupt 2 */
#define CR_HINT1	0x00000800	/* h/w interrupt 1 */
#define CR_HINT0	0x00000400	/* h/w interrupt 0 */
#define CR_SINT1	0x00000200	/* s/w interrupt 1 */
#define CR_SINT0	0x00000100 	/* s/w interrupt 0 */

/* alternative interrupt pending bit naming */
#define CR_IP7		0x00008000
#define CR_IP6		0x00004000
#define CR_IP5		0x00002000
#define CR_IP4		0x00001000
#define CR_IP3		0x00000800
#define CR_IP2		0x00000400
#define CR_IP1		0x00000200
#define CR_IP0		0x00000100

#define CR_XMASK	0x0000007c 	/* exception code mask */
#define CR_X_MASK	0x0000007c
#define  CR_X_SHIFT		 2
#define  CR_X_BITS		 5
#define CR_XCPT(x)	((x)<<2)

/*
 * MIPS32 Status Register  (CP0 Register 12, Select 0)
 */
#define SR_CU3		0x80000000	/* coprocessor 3 enable */
#define  SR_CU3_SHIFT		31
#define SR_CU2		0x40000000	/* coprocessor 2 enable */
#define  SR_CU2_SHIFT		30
#define SR_CU1		0x20000000	/* coprocessor 1 enable */
#define  SR_CU1_SHIFT		29
#define SR_CU0		0x10000000	/* coprocessor 0 enable */

#define SR_RP		0x08000000	/* reduce power */
#define  SR_RP_SHIFT		27
#define SR_FR		0x04000000	/* 64-bit fpu registers */
#define  SR_FR_SHIFT		26
#define SR_RE		0x02000000	/* reverse endian (user mode) */
#define  SR_RE_SHIFT		25
#define SR_MX		0x01000000	/* enable MDMX/DSP ASE */
#define  SR_MX_SHIFT		24
#define SR_PX		0x00800000	/* user 64-bit reg / 32-bit addr */
#define  SR_PX_SHIFT		23
#define SR_BEV		0x00400000	/* boot exception vectors */
#define  SR_BEV_SHIFT		22
#define SR_TS		0x00200000	/* TLB shutdown */
#define  SR_TS_SHIFT		21
#define SR_SR		0x00100000	/* soft reset occurred */
#define SR_PE		0x00100000	/* soft reset (clear parity error) */
#define  SR_SR_SHIFT		20
#define SR_NMI		0x00080000 	/* NMI occurred */
#define  SR_NMI_SHIFT		19
#define SR_MCU		0x00040000 	/* MCU ASE implemented */
#define  SR_MCU_SHIFT		18

#define SR_IPL_MASK	0x0000fc00
#define  SR_IPL_SHIFT		10
#define  SR_IPL_BITS		 6
#define SR_IMASK	0x0000ff00

/* interrupt mask bits */
#define SR_HINT5	0x00008000	/* enable h/w interrupt 6 */
#define SR_HINT4	0x00004000	/* enable h/w interrupt 5 */
#define SR_HINT3	0x00002000	/* enable h/w interrupt 4 */
#define SR_HINT2	0x00001000	/* enable h/w interrupt 3 */
#define SR_HINT1	0x00000800	/* enable h/w interrupt 2 */
#define SR_HINT0	0x00000400	/* enable h/w interrupt 1 */
#define SR_SINT1	0x00000200	/* enable s/w interrupt 1 */
#define SR_SINT0	0x00000100	/* enable s/w interrupt 0 */

/* alternative interrupt mask naming */
#define SR_IM7		0x00008000
#define SR_IM6		0x00004000
#define SR_IM5		0x00002000
#define SR_IM4		0x00001000
#define SR_IM3		0x00000800
#define SR_IM2		0x00000400
#define SR_IM1		0x00000200
#define SR_IM0		0x00000100

#define SR_KX		0x00000080	/* 64-bit kernel mode */
#define SR_KX_SHIFT		 7
#define SR_SX		0x00000040	/* 64-bit supervisor mode */
#define SR_SX_SHIFT		 6
#define SR_UX		0x00000020	/* 64-bit user mode */
#define SR_UX_SHIFT		 5

#define SR_UM		0x00000010	/* user mode */
#define SR_KSU_MASK	0x00000018	/* ksu mode mask */
#define SR_KSU_SHIFT		 3
#define SR_KSU_BITS		 2
#define SR_KSU_USER	0x00000010	/* user mode */
#define SR_KSU_SPVS	0x00000008	/* supervisor mode */
#define SR_KSU_KERN	0x00000000	/* kernel mode */

#define SR_ERL		0x00000004	/* error level */
#define  SR_ERL_SHIFT		 2
#define SR_EXL		0x00000002	/* exception level */
#define  SR_EXL_SHIFT		 1
#define SR_IE		0x00000001 	/* interrupt enable */
#define  SR_IE_SHIFT		 0

/*
 * MIPS32r6 VPControl (CP0 Register 0, Select 4)
 */
#define VPCONTROL_DIS	0x00000001
#define VPCONTROL_SHIFT		 0

/*
 * MIPS32r2 HWREna Register  (CP0 Register 7, Select 0)
 */
#define HWRENA_ULR	0x20000000
#define HWRENA_XNP	0x00000020
#define HWRENA_PERFCNT	0x00000010
#define HWRENA_CCRES	0x00000008
#define HWRENA_CC	0x00000004
#define HWRENA_SYNCSTEP	0x00000002
#define HWRENA_CPUNUM	0x00000001

/*
 * MIPS32r2 IntCtl Register  (CP0 Register 12, Select 1)
 */
#define INTCTL_IPTI	0xe0000000	/* timer i/u pending bit */
#define  INTCTL_IPTI_SHIFT	29
#define  INTCTL_IPTI_BITS	 3
#define INTCTL_IPPCI	0x1c000000	/* perfctr i/u pending bit */
#define  INTCTL_IPPCI_SHIFT	26
#define  INTCTL_IPPCI_BITS	 3
#define INTCTL_IPFDC	0x03800000	/* fast debug chan i/u pending bit */
#define  INTCTL_IPFDC_SHIFT	23
#define  INTCTL_IPFDC_BITS	 3
#define INTCTL_VS	0x000003e0	/* vector spacing */
#define  INTCTL_VS_SHIFT	 5
#define  INTCTL_VS_BITS		 5
#define  INTCTL_VS_0	(0x00 << INTCTL_VS_SHIFT)
#define  INTCTL_VS_32	(0x01 << INTCTL_VS_SHIFT)
#define  INTCTL_VS_64	(0x02 << INTCTL_VS_SHIFT)
#define  INTCTL_VS_128	(0x04 << INTCTL_VS_SHIFT)
#define  INTCTL_VS_256	(0x08 << INTCTL_VS_SHIFT)
#define  INTCTL_VS_512	(0x10 << INTCTL_VS_SHIFT)

/*
 * MIPS32r2 SRSCtl Register  (CP0 Register 12, Select 2)
 */
#define SRSCTL_HSS	0x3c000000	/* highest shadow set */
#define SRSCTL_HSS_SHIFT	26
#define SRSCTL_EICSS	0x003c0000	/* EIC shadow set */
#define SRSCTL_EICSS_SHIFT	18
#define SRSCTL_ESS	0x0000f000	/* exception shadow set */
#define SRSCTL_ESS_SHIFT	12
#define SRSCTL_PSS	0x000003c0	/* previous shadow set */
#define SRSCTL_PSS_SHIFT	 6
#define SRSCTL_CSS	0x0000000f	/* current shadow set */
#define SRSCTL_CSS_SHIFT	 0

/*
 * MIPS32 BEVVA Register (CP0 Register 15, Select 4)
 */
#define BEVVA_BASE_MASK	0xfffff000
#define BEVVA_BASE_SHIFT	12
#define BEVVA_MASK_MASK	0x00000fff
#define BEVVA_MASK_SHIFT	 0

/*
 * MIPS32 Config0 Register  (CP0 Register 16, Select 0)
 */
#define CFG0_M		0x80000000	/* Config1 implemented */
#define  CFG0_M_SHIFT		31
#define CFG0_K23_MASK	0x70000000	/* Fixed MMU kseg2/3 CCA */
#define  CFG0_K23_SHIFT		28
#define CFG0_KU_MASK	0x0e000000	/* Fixed MMU kuseg CCA */
#define  CFG0_KU_SHIFT		25
#define CFG0_BE		0x00008000	/* Big Endian */
#define  CFG0_BE_SHIFT		15
#define CFG0_AT_MASK	0x00006000	/* Architecture type: */
#define CFG0_ATMASK	0x00006000
#define  CFG0_AT_SHIFT		13
#define  CFG0_AT_BITS		 2
#define  CFG0_AT_M32	 (0 << CFG0_AT_SHIFT) /* MIPS32 */
#define  CFG0_AT_M64_A32 (1 << CFG0_AT_SHIFT) /* MIPS64, 32-bit addresses */
#define  CFG0_AT_M64_A64 (2 << CFG0_AT_SHIFT) /* MIPS64, 64-bit addresses */
#define  CFG0_AT_RES	 (3 << CFG0_AT_SHIFT)
#define CFG0_AR_MASK	0x00001c00
#define CFG0_ARMASK	0x00001c00
#define  CFG0_AR_SHIFT		10
#define  CFG0_ARSHIFT		10
#define  CFG0_AR_BITS		 3
#define  CFG0_AR_R1	 (0 << CFG0_AR_SHIFT) /* Release 1 */
#define  CFG0_AR_R2	 (1 << CFG0_AR_SHIFT) /* Release 2,3,5 */
#define  CFG0_AR_R6	 (2 << CFG0_AR_SHIFT) /* Release 6 */
#define CFG0_MT_MASK	0x00000380	/* MMU Type: */
#define CFG0_MTMASK	0x00000380
#define  CFG0_MT_SHIFT		 7
#define  CFG0_MT_BITS		 3
#define  CFG0_MT_NONE	 (0 << CFG0_MT_SHIFT) /* No MMU */
#define  CFG0_MT_TLB	 (1 << CFG0_MT_SHIFT) /* Standard TLB */
#define  CFG0_MT_BAT	 (2 << CFG0_MT_SHIFT) /* BAT */
#define  CFG0_MT_FIXED	 (3 << CFG0_MT_SHIFT) /* Fixed mapping */
#define  CFG0_MT_NONSTD	 (3 << CFG0_MT_SHIFT) /* Legacy */
#define  CFG0_MT_DUAL	 (4 << CFG0_MT_SHIFT) /* Dual VTLB and FTLB */
#define CFG0_VI		0x00000008	/* Icache is virtual */
#define  CFG0_VI_SHIFT		 3
#define CFG0_K0_MASK	0x00000007	/* KSEG0 coherency algorithm */
#define CFG0_K0MASK	0x00000007
#define  CFG0_K0_SHIFT		 0
#define  CFG0_K0_BITS		 3

/*
 * MIPS32 Config1 Register (CP0 Register 16, Select 1)
 */
#define CFG1_M		0x80000000	/* Config2 implemented */
#define CFG1_M_SHIFT		31
#define CFG1_MMUS_MASK	0x7e000000	/* mmu size - 1 */
#define CFG1_MMUSMASK	0x7e000000
#define  CFG1_MMUS_SHIFT	25
#define  CFG1_MMUSSHIFT		25
#define  CFG1_MMUS_BITS		 6
#define CFG1_IS_MASK	0x01c00000	/* icache lines 64<<n */
#define CFG1_ISMASK	0x01c00000
#define  CFG1_IS_SHIFT		22	/* Unless n==7, then 32 */
#define  CFG1_ISSHIFT		22
#define  CFG1_IS_BITS		 3
#define CFG1_IL_MASK	0x00380000	/* icache line size 2<<n */
#define CFG1_ILMASK	0x00380000
#define  CFG1_IL_SHIFT		19
#define  CFG1_ILSHIFT		19
#define  CFG1_IL_BITS		 3
#define CFG1_IA_MASK	0x00070000	/* icache ways - 1 */
#define CFG1_IAMASK	0x00070000
#define  CFG1_IA_SHIFT		16
#define  CFG1_IASHIFT		16
#define  CFG1_IA_BITS		 3
#define CFG1_DS_MASK	0x0000e000	/* dcache lines 64<<n */
#define CFG1_DSMASK	0x0000e000
#define  CFG1_DS_SHIFT		13
#define  CFG1_DSSHIFT		13
#define  CFG1_DS_BITS		 3
#define CFG1_DL_MASK	0x00001c00	/* dcache line size 2<<n */
#define CFG1_DLMASK	0x00001c00
#define  CFG1_DL_SHIFT		10
#define  CFG1_DLSHIFT		10
#define  CFG1_DL_BITS		 3
#define CFG1_DA_MASK	0x00000380	/* dcache ways - 1 */
#define CFG1_DAMASK	0x00000380
#define  CFG1_DA_SHIFT		 7
#define  CFG1_DASHIFT		 7
#define  CFG1_DA_BITS		 3
#define CFG1_C2		0x00000040	/* Coprocessor 2 present */
#define  CFG1_C2_SHIFT		 6
#define CFG1_MD		0x00000020	/* MDMX implemented */
#define  CFG1_MD_SHIFT		 5
#define CFG1_PC		0x00000010	/* performance counters implemented */
#define  CFG1_PC_SHIFT		 4
#define CFG1_WR		0x00000008	/* watch registers implemented */
#define  CFG1_WR_SHIFT		 3
#define CFG1_CA		0x00000004	/* compression (mips16) implemented */
#define  CFG1_CA_SHIFT		 2
#define CFG1_EP		0x00000002	/* ejtag implemented */
#define  CFG1_EP_SHIFT		 1
#define CFG1_FP		0x00000001	/* fpu implemented */
#define  CFG1_FP_SHIFT		 0


/*
 * MIPS32r2 Config2 Register (CP0 Register 16, Select 2)
 */
#define CFG2_M		0x80000000	/* Config3 implemented */
#define  CFG1_M_SHIFT		31
#define CFG2_TU_MASK	0x70000000	/* tertiary cache control */
#define CFG2_TUMASK	0x70000000
#define  CFG2_TU_SHIFT		28
#define  CFG2_TUSHIFT		28
#define  CFG2_TU_BITS		 3
#define CFG2_TS_MASK	0x0f000000	/* tcache sets per wway 64<<n */
#define CFG2_TSMASK	0x0f000000
#define  CFG2_TS_SHIFT		24
#define  CFG2_TSSHIFT		24
#define  CFG2_TS_BITS		 4
#define CFG2_TL_MASK	0x00f00000	/* tcache line size 2<<n */
#define CFG2_TLMASK	0x00f00000
#define  CFG2_TL_SHIFT		20
#define  CFG2_TLSHIFT		20
#define  CFG2_TL_BITS		 4
#define CFG2_TA_MASK	0x000f0000	/* tcache ways - 1 */
#define CFG2_TAMASK	0x000f0000
#define  CFG2_TA_SHIFT		16
#define  CFG2_TASHIFT		16
#define  CFG2_TA_BITS		 4
#define CFG2_SU_MASK	0x0000f000	/* secondary cache control */
#define CFG2_SUMASK	0x0000f000
#define  CFG2_SU_SHIFT		12
#define  CFG2_SUSHIFT		12
#define  CFG2_SU_BITS		 4
#define CFG2_SS_MASK	0x00000f00	/* scache sets per wway 64<<n */
#define CFG2_SSMASK	0x00000f00
#define  CFG2_SS_SHIFT		 8
#define  CFG2_SSSHIFT		 8
#define  CFG2_SS_BITS		 4
#define CFG2_SL_MASK	0x000000f0	/* scache line size 2<<n */
#define CFG2_SLMASK	0x000000f0
#define  CFG2_SL_SHIFT		 4
#define  CFG2_SLSHIFT		 4
#define  CFG2_SL_BITS		 4
#define CFG2_SA_MASK	0x0000000f	/* scache ways - 1 */
#define CFG2_SAMASK	0x0000000f
#define  CFG2_SA_SHIFT		 0
#define  CFG2_SASHIFT		 0
#define  CFG2_SA_BITS		 4

/*
 * MIPS32r2 Config3 Register (CP0 Register 16, Select 3)
 */
#define CFG3_M		0x80000000	/* Config4 implemented */
#define  CFG3_M_SHIFT		31
#define CFG3_BPG	0x40000000	/* Big pages implemented */
#define  CFG3_BPG_SHIFT		30
#define CFG3_CMGCR	0x20000000	/* Coherency manager implemented */
#define  CFG3_CMGCR_SHIFT	29
#define CFG3_MSAP	0x10000000	/* MSA implemented */
#define  CFG3_MSAP_SHIFT	28
#define CFG3_BP		0x08000000	/* BadInstrP implemented */
#define  CFG3_BP_SHIFT		27
#define CFG3_BI		0x04000000	/* BadInstr implemented */
#define  CFG3_BI_SHIFT		26
#define CFG3_SC		0x02000000	/* Segment control implemented */
#define  CFG3_SC_SHIFT		25
#define CFG3_PW		0x01000000	/* HW page table walk implemented */
#define  CFG3_PW_SHIFT		24
#define CFG3_VZ		0x00800000	/* Virtualization module implemented */
#define  CFG3_VZ_SHIFT		23
#define CFG3_IPLW_MASK	0x00600000	/* IPL field width */
#define  CFG3_IPLW_SHIFT	21
#define  CFG3_IPLW_BITS		 2
#define  CFG3_IPLW_6BIT	(0 << CFG3_IPLW_SHIFT) /* IPL/RIPL are 6-bits wide */
#define  CFG3_IPLW_8BIT	(1 << CFG3_IPLW_SHIFT) /* IPL/RIPL are 8-bits wide */
#define CFG3_MMAR_MASK	0x001c0000	/* MicroMIPS64 architecture revision */
#define  CFG3_MMAR_SHIFT	18
#define  CFG3_MMAR_BITS		 3
#define  CFG3_MMAR_R3	(0 << CFG3_MMAR_SHIFT) /* MicroMIPS64 Release 3 */
#define CFG3_MCU	0x00020000	/* MCU ASE is implemented */
#define  CFG3_MCU_SHIFT		17
#define CFG3_IOE	0x00010000	/* MicroMIPS exception entry points */
#define  CFG3_IOE_SHIFT		16
#define CFG3_ISA_MASK	0x0000c000	/* ISA availability */
#define  CFG3_ISA_SHIFT		14
#define  CFG3_ISA_BITS		 2
#define  CFG3_ISA_MIPS	(0 << CFG3_ISA_SHIFT)  /* MIPS only */
#define  CFG3_ISA_UMIPS (1 << CFG3_ISA_SHIFT)  /* MicroMIPS only */
#define  CFG3_ISA_BOTH	(2 << CFG3_ISA_SHIFT)  /* MIPS (boot) and MicroMIPS */
#define  CFG3_ISA_UBOTH	(3 << CFG3_ISA_SHIFT)  /* MIPS and MicroMIPS (boot) */
#define CFG3_ULRI	0x00002000	/* UserLocal register is available */
#define  CFG3_ULRI_SHIFT	13
#define CFG3_RXI	0x00001000	/* RIE and XIE exist in pagegrain */
#define  CFG3_RXI_SHIFT		12
#define CFG3_DSP2P	0x00000800	/* DSPR2 ASE present */
#define  CFG3_DSP2P_SHIFT	11
#define CFG3_DSPP	0x00000400	/* DSP ASE present */
#define  CFG3_DSPP_SHIFT	10
#define CFG3_CTXTC	0x00000200	/* Context Config regs are present */
#define  CFG3_CTXTC_SHIFT	 9
#define CFG3_ITL	0x00000100	/* IFlowtrace mechanism implemented */
#define  CFG3_ITL_SHIFT		 8
#define CFG3_LPA	0x00000080	/* Large physical addresses */
#define  CFG3_LPA_SHIFT		 7
#define CFG3_VEIC	0x00000040	/* Vectored external i/u controller */
#define  CFG3_VEIC_SHIFT      	 6
#define CFG3_VI		0x00000020	/* Vectored i/us */
#define  CFG3_VI_SHIFT		 5
#define CFG3_SP		0x00000010	/* Small page support */
#define  CFG3_SP_SHIFT		 4
#define CFG3_CDMM	0x00000008	/* Common Device Memory Map support */
#define  CFG3_CDMM_SHIFT	 3
#define CFG3_MT		0x00000004	/* MT ASE present */
#define  CFG3_MT_SHIFT		 2
#define CFG3_SM		0x00000002	/* SmartMIPS ASE */
#define  CFG3_SM_SHIFT		 1
#define CFG3_TL		0x00000001	/* Trace Logic */
#define  CFG3_TL_SHIFT		 0

/*
 * MIPS32r2 Config4 Register (CP0 Register 16, Select 4)
 */
#define CFG4_M		0x80000000	/* Config5 implemented */
#define  CFG4_M_SHIFT		31
#define CFG4_IE_MASK	0x60000000	/* TLB invalidate insn support */
#define  CFG4_IE_SHIFT		29
#define  CFG4_IE_BITS		 2
#define  CFG4_IE_NONE	(0 << CFG4_IE_SHIFT) /* TLB invalidate not available */
#define  CFG4_IE_EHINV	(1 << CFG4_IE_SHIFT) /* TLB invalidate with EHINV */
#define  CFG4_IE_INV	(2 << CFG4_IE_SHIFT) /* TLB invalidate per entry */
#define  CFG4_IE_INVALL	(3 << CFG4_IE_SHIFT) /* TLB invalidate entire MMU */
#define CFG4_AE		0x10000000	/* EntryHI.ASID is 10-bits */
#define  CFG4_AE_SHIFT		28
#define CFG4_VTLBSEXT_MASK   0x0f000000	/* VTLB size extension field */
#define  CFG4_VTLBSEXT_SHIFT	     24
#define  CFG4_VTLBSEXT_BITS	      4
#define CFG4_KSCREXIST_MASK  0x00ff0000	/* Indicates presence of KScratch */
#define  CFG4_KSCREXIST_SHIFT	     16
#define  CFG4_KSCREXIST_BITS	      8
#define CFG4_MMUED	     0x0000c000	/* MMU Extension definition */
#define  CFG4_MMUED_SHIFT	     14
#define  CFG4_MMUED_BITS	      2
#define  CFG4_MMUED_SIZEEXT  (1 << CFG4_MMUED_SHIFT) /* MMUSizeExt */
#define  CFG4_MMUED_FTLB     (2 << CFG4_MMUED_SHIFT) /* FTLB fields */
#define  CFG4_MMUED_FTLBVEXT (3 << CFG4_MMUED_SHIFT) /* FTLB and Vext */
#define CFG4_FTLBPS_MASK     0x00001f00	/* FTLB Page Size */
#define  CFG4_FTLBPS_SHIFT	      8
#define  CFG4_FTLBPS_BITS	      5
#define CFG4_FTLBW_MASK	     0x000000f0	/* FTLB Ways mask */
#define  CFG4_FTLBW_SHIFT	      4
#define  CFG4_FTLBW_BITS	      4
#define CFG4_FTLBS_MASK	     0x0000000f	/* FTLB Sets mask */
#define  CFG4_FTLBS_SHIFT	      0
#define  CFG4_FTLBS_BITS	      4
#define CFG4_MMUSE_MASK	     0x000000ff	/* MMU size extension */
#define  CFG4_MMUSE_SHIFT	      0
#define  CFG4_MMUSE_BITS	      8

/*
 * MIPS32r2 Config5 Register (CP0 Register 16, Select 5)
 */
#define CFG5_M		0x80000000	/* Undefined extension */
#define  CFG5_M_SHIFT		31
#define CFG5_K		0x40000000	/* Disable CCA control */
#define  CFG5_K_SHIFT		30
#define CFG5_CV		0x20000000	/* Disable CacheException in KSEG1 */
#define  CFG5_CV_SHIFT		29
#define CFG5_EVA	0x10000000	/* EVA is implemented */
#define  CFG5_EVA_SHIFT		28
#define CFG5_MSAEN	0x08000000	/* Enable MSA ASE */
#define  CFG5_MSAEN_SHIFT	27
#define CFG_XNP		0x00002000
#define CFG_XNP_SHIFT		13
#define CFG5_CES	0x00001000	/* Current endian state */
#define  CFG5_CES_SHIFT		12
#define CFG5_DEC	0x00000800	/* Dual endian control */
#define  CFG5_DEC_SHIFT		11
#define CFG5_L2C	0x00000400	/* Config 2 is memory mapped */
#define  CFG5_L2C_SHIFT		10
#define CFG5_UFE	0x00000200	/* Usermode FRE control */
#define  CFG5_LUFE_SHIFT	 9
#define CFG5_FRE	0x00000100	/* Emulation support for FR=0 */
#define  CFG5_FRE_SHIFT		 8
#define CFG5_VP		0x00000080	/* Multiple virtual processors */
#define  CFG5_VP_SHIFT		 7
#define CFG5_SBRI	0x00000040	/* Force RI on SDBBP */
#define  CFG5_SBRI_SHIFT	 6
#define CFG5_MVH	0x00000020	/* XPA instruction support */
#define  CFG5_MVH_SHIFT		 5
#define CFG5_LLB	0x00000010	/* Load-Linked Bit present */
#define  CFG5_LLB_SHIFT		 4
#define CFG5_MRP	0x00000008	/* MAAR and MAARI are present */
#define  CFG5_MRP_SHIFT		 3
#define CFG5_UFR	0x00000004	/* Usermode FR control */
#define  CFG5_UFR_SHIFT		 2
#define CFG5_NF		0x00000001	/* Nested fault support */
#define  CFG5_NF_SHIFT		 0

/*
 * Primary cache mode
 */
#define CFG_CBITS		3
#define CFG_C_UNCACHED		2
#define CFG_C_WBACK		3
#define CFG_C_NONCOHERENT	3

#if 0
/* These cache modes are CPU specific */
#define CFG_C_WTHRU_NOALLOC	0
#define CFG_C_WTHRU_ALLOC	1
#define CFG_C_COHERENTXCL	4
#define CFG_C_COHERENTXCLW	5
#define CFG_C_COHERENTUPD	6
#define CFG_C_UNCACHED_ACCEL	7
#endif


/*
 * Primary Cache TagLo (CP0 Register 28, Select 0/2)
 */
#define TAG_PTAG_MASK           0xffffff00      /* Primary Tag */
#define TAG_PTAG_SHIFT          8
#define TAG_PSTATE_MASK         0x000000c0      /* Primary Cache State */
#define TAG_PSTATE_SHIFT        6
#define TAG_PSTATE_LOCK		0x00000020
#define TAG_PARITY_MASK         0x00000001      /* Primary Tag Parity */
#define TAG_PARITY_SHIFT        0

/* primary cache state (XXX actually implementation specific) */
#define PSTATE_INVAL		0
#define PSTATE_SHARED		1
#define PSTATE_CLEAN_EXCL	2
#define PSTATE_DIRTY_EXCL	3


/*
 * Cache operations
 */
#define Index_Invalidate_I               0x00        /* 0       0 */
#define Index_Writeback_Inv_D            0x01        /* 0       1 */
#define Index_Writeback_Inv_T            0x02        /* 0       2 */
#define Index_Writeback_Inv_S            0x03        /* 0       3 */
#define Index_Load_Tag_I                 0x04        /* 1       0 */
#define Index_Load_Tag_D                 0x05        /* 1       1 */
#define Index_Load_Tag_T                 0x06        /* 1       2 */
#define Index_Load_Tag_S                 0x07        /* 1       3 */
#define Index_Store_Tag_I                0x08        /* 2       0 */
#define Index_Store_Tag_D                0x09        /* 2       1 */
#define Index_Store_Tag_T                0x0A        /* 2       2 */
#define Index_Store_Tag_S                0x0B        /* 2       3 */
#define Hit_Invalidate_I                 0x10        /* 4       0 */
#define Hit_Invalidate_D                 0x11        /* 4       1 */
#define Hit_Invalidate_T                 0x12        /* 4       2 */
#define Hit_Invalidate_S                 0x13        /* 4       3 */
#define Fill_I                           0x14        /* 5       0 */
#define Hit_Writeback_Inv_D              0x15        /* 5       1 */
#define Hit_Writeback_Inv_T              0x16        /* 5       2 */
#define Hit_Writeback_Inv_S              0x17        /* 5       3 */
#define Hit_Writeback_D                  0x19        /* 6       1 */
#define Hit_Writeback_T                  0x1A        /* 6       2 */
#define Hit_Writeback_S                  0x1B        /* 6       3 */
#define Fetch_Lock_I                 	 0x1C        /* 7       0 */
#define Fetch_Lock_D                 	 0x1D        /* 7       1 */
#define Fetch_Lock_S                     0x1F        /* 7       3 */

/* MIPS32 WatchLo Register (CP0 Register 18) */
#define WATCHLO_VA		0xfffffff8
#define WATCHLO_I		0x00000004
#define WATCHLO_R		0x00000002
#define WATCHLO_W		0x00000001

/* MIPS32 WatchHi Register (CP0 Register 19) */
#define WATCHHI_M		0x80000000
#define  WATCHHI_M_SHIFT		31
#define WATCHHI_G		0x40000000
#define  WATCHHI_G_SHIFT		30
#define WATCHHI_ASID_MASK	0x00ff0000
#define WATCHHI_ASIDMASK	0x00ff0000
#define  WATCHHI_ASID_SHIFT		16
#define  WATCHHI_ASIDSHIFT		16
#define  WATCHHI_ASID_BITS		 8
#define WATCHHI_MASK		0x00000ffc
#define WATCHHI_I		0x00000004
#define WATCHHI_R		0x00000002
#define WATCHHI_W		0x00000001

/* MIPS32 PerfCnt Register (CP0 Register 25) */
#define PERFCNT_M		0x80000000
#define PERFCNT_EVENT_MASK	0x000007e0
#define PERFCNT_EVENTMASK	0x000007e0
#define PERFCNT_EVENT_SHIFT		 5
#define PERFCNT_EVENTSHIFT		 5
#define PERFCNT_EVENT_BITS		 6
#define PERFCNT_IE		0x00000010
#define PERFCNT_U		0x00000008
#define PERFCNT_S		0x00000004
#define PERFCNT_K		0x00000002
#define PERFCNT_EXL		0x00000001

/* MIPS32r2 PageGrain  Register (CP0 Register 5, Select 1) */
#define PAGEGRAIN_ELPA	0x20000000	/* Enable large physical addresses */
#define PAGEGRAIN_ELPA_SHIFT	29
#define PAGEGRAIN_ELPA_BITS	1

#define PAGEGRAIN_ESP	0x10000000	/* Enable small (1KB) page support */

/* MIPS32r2 EBase  Register (CP0 Register 15, Select 1) */
#define EBASE_BASE	0xfffff000	/* Exception base */
#define EBASE_WG	0x00000800	/* Write Gate */
#define  EBASE_WG_SHIFT		11
#define EBASE_CPU	0x000003ff	/* CPU number */
#define  EBASE_CPU_SHIFT	 0
#define  EBASE_CPU_BITS		10

/* MIPS32r2 EntryHi register (CP0 Register 10, Select 0) */
#define C0_ENTRYHI64_R_MASK	0xC000000000000000
#define C0_ENTRYHI64_R_BITS	2
#define C0_ENTRYHI64_R_SHIFT	61
#define C0_ENTRYHI64_VPN2_MASK	0xFFFFFFE000
#define C0_ENTRYHI64_VPN2_BITS	27
#define C0_ENTRYHI64_VPN2_SHIFT	13
#define C0_ENTRYHI_VPN2_MASK	0xFFFFE000
#define C0_ENTRYHI_VPN2_BITS	19
#define C0_ENTRYHI_VPN2_SHIFT	13
#define C0_ENTRYHI_VPN2X_MASK	0x3
#define C0_ENTRYHI_VPN2X_BITS	2
#define C0_ENTRYHI_VPN2X_SHIFT	11
#define C0_ENTRYHI_EHINV_MASK	0x400
#define C0_ENTRYHI_EHINV_BITS	1
#define C0_ENTRYHI_EHINV_SHIFT	10
#define C0_ENTRYHI_ASIDX_MASK	0x300
#define C0_ENTRYHI_ASIDX_BITS	2
#define C0_ENTRYHI_ASIDX_SHIFT	8
#define C0_ENTRYHI_ASID_MASK	0xFF
#define C0_ENTRYHI_ASID_BITS	8
#define C0_ENTRYHI_ASID_SHIFT	0

/* MIPS32 EntryLo0 register (CP0 Register 2, select 0) */
#define ENTRYLO064_RI_MASK	0x8000000000000000
#define ENTRYLO064_RI_BITS	1
#define ENTRYLO064_RI_SHIFT	63
#define ENTRYLO064_XI_MASK	0x4000000000000000
#define ENTRYLO064_XI_BITS	1
#define ENTRYLO064_XI_SHIFT	62
#define ENTRYLO064_PFN_MASK	0x3FFFFFFFFFFFFFE0
#define ENTRYLO064_PFN_BITS	56
#define ENTRYLO064_PFN_SHIFT	6
#define ENTRYLO064_RI_SHIFT	63
#define ENTRYLO064_RI_BITS	1
#define ENTRYLO064_RI_SHIFT	63
#define ENTRYLO0_PFNX_MASK	0x7FFFFF
#define ENTRYLO0_PFNX_BITS	23
#define ENTRYLO0_PFNX_SHIFT	0
#define ENTRYLO0_RI_MASK	0x80000000
#define ENTRYLO0_RI_BITS	1
#define ENTRYLO0_RI_SHIFT	31
#define ENTRYLO0_XI_MASK	0x40000000
#define ENTRYLO0_XI_BITS	1
#define ENTRYLO0_XI_SHIFT	30
#define ENTRYLO0_PFN_MASK	0x3FFFFFE0
#define ENTRYLO0_PFN_BITS	23
#define ENTRYLO0_PFN_SHIFT	6
#define ENTRYLO0_C_MASK		0x38
#define ENTRYLO0_C_BITS		3
#define ENTRYLO0_C_SHIFT	3
#define ENTRYLO0_D_MASK		0x4
#define ENTRYLO0_D_BITS		1
#define ENTRYLO0_D_SHIFT	2
#define ENTRYLO0_V_MASK		0x2
#define ENTRYLO0_V_BITS		1
#define ENTRYLO0_V_SHIFT	1
#define ENTRYLO0_G_MASK		0x1
#define ENTRYLO0_G_BITS		1
#define ENTRYLO0_G_SHIFT	0

/* MIPS32 EntryLo1 register (CP0 Register 3, select 0) */
#define ENTRYLO164_RI_MASK	ENTRYLO064_RI_MASK
#define ENTRYLO164_RI_BITS	ENTRYLO064_RI_BITS
#define ENTRYLO164_RI_SHIFT	ENTRYLO064_RI_SHIFT
#define ENTRYLO164_XI_MASK	ENTRYLO064_XI_MASK
#define ENTRYLO164_XI_BITS	ENTRYLO064_XI_BITS
#define ENTRYLO164_XI_SHIFT	ENTRYLO064_XI_SHIFT
#define ENTRYLO164_PFN_MASK	ENTRYLO064_PFN_MASK
#define ENTRYLO164_PFN_BITS	ENTRYLO064_PFN_BITS
#define ENTRYLO164_PFN_SHIFT	ENTRYLO064_PFN_SHIFT
#define ENTRYLO164_RI_SHIFT	ENTRYLO064_RI_SHIFT
#define ENTRYLO164_RI_BITS	ENTRYLO064_RI_BITS
#define ENTRYLO164_RI_SHIFT	ENTRYLO064_RI_SHIFT
#define ENTRYLO1_PFNX_MASK	ENTRYLO0_PFNX_MASK
#define ENTRYLO1_PFNX_BITS	ENTRYLO0_PFNX_BITS
#define ENTRYLO1_PFNX_SHIFT	ENTRYLO0_PFNX_SHIFT
#define ENTRYLO1_RI_MASK	ENTRYLO0_RI_MASK
#define ENTRYLO1_RI_BITS	ENTRYLO0_RI_BITS
#define ENTRYLO1_RI_SHIFT	ENTRYLO0_RI_SHIFT
#define ENTRYLO1_XI_MASK	ENTRYLO0_XI_MASK
#define ENTRYLO1_XI_BITS	ENTRYLO0_XI_BITS
#define ENTRYLO1_XI_SHIFT	ENTRYLO0_XI_SHIFT
#define ENTRYLO1_PFN_MASK	ENTRYLO0_PFN_MASK
#define ENTRYLO1_PFN_BITS	ENTRYLO0_PFN_BITS
#define ENTRYLO1_PFN_SHIFT	ENTRYLO0_PFN_SHIFT
#define ENTRYLO1_C_MASK		ENTRYLO0_C_MASK
#define ENTRYLO1_C_BITS		ENTRYLO0_C_BITS
#define ENTRYLO1_C_SHIFT	ENTRYLO0_C_SHIFT
#define ENTRYLO1_D_MASK		ENTRYLO0_D_MASK
#define ENTRYLO1_D_BITS		ENTRYLO0_D_BITS
#define ENTRYLO1_D_SHIFT	ENTRYLO0_D_SHIFT
#define ENTRYLO1_V_MASK		ENTRYLO0_V_MASK
#define ENTRYLO1_V_BITS		ENTRYLO0_V_BITS
#define ENTRYLO1_V_SHIFT	ENTRYLO0_V_SHIFT
#define ENTRYLO1_G_MASK		ENTRYLO0_G_MASK
#define ENTRYLO1_G_BITS		ENTRYLO0_G_BITS
#define ENTRYLO1_G_SHIFT	ENTRYLO0_G_SHIFT

#ifdef __ASSEMBLER__

/*
 * MIPS32 Coprocessor 0 register numbers
 */
#define C0_INDEX	$0
#define C0_INX		$0
#define C0_RANDOM	$1
#define C0_RAND		$1
#define C0_ENTRYLO0	$2
#define C0_TLBLO0	$2
#define C0_ENTRYLO1	$3
#define C0_TLBLO1	$3
#define C0_GLOBAL	$3,1
#define C0_CONTEXT	$4
#define C0_CTXT		$4
#define C0_CONTEXTCONF	$4,1
#define C0_USERLOCAL	$4,2
#define C0_XCONTEXTCONF	$4,3
#define C0_DEBUGCTXTID	$4,4
#define C0_PAGEMASK	$5
#define C0_PAGEGRAIN	$5,1
#define C0_SEGCTL0	$5,2
#define C0_SEGCTL1	$5,3
#define C0_SEGCTL2	$5,4
#define C0_PWBASE	$5,5
#define C0_PWFIELD	$5,6
#define C0_PWSIZE	$5,7
#define C0_WIRED	$6
#define C0_PWCTL	$6,6
#define C0_HWRENA	$7
#define C0_BADVADDR 	$8
#define C0_VADDR 	$8
#define C0_BADINSTR 	$8,1
#define C0_BADPINSTR 	$8,2
#define C0_COUNT 	$9
#define C0_ENTRYHI	$10
#define C0_TLBHI	$10
#define C0_COMPARE	$11
#define C0_STATUS	$12
#define C0_SR		$12
#define C0_INTCTL	$12,1
#define C0_SRSCTL	$12,2
#define C0_SRSMAP	$12,3
#define C0_CAUSE	$13
#define C0_CR		$13
#define C0_NESTEDEXC	$13,5
#define C0_EPC 		$14
#define C0_NEPC		$14,2
#define C0_PRID		$15
#define C0_EBASE	$15,1
#define C0_CDMMBASE	$15,2
#define C0_CMGCRBASE	$15,3
#define C0_BEVVA	$15,4
#define C0_CONFIG	$16
#define C0_CONFIG0	$16,0
#define C0_CONFIG1	$16,1
#define C0_CONFIG2	$16,2
#define C0_CONFIG3	$16,3
#define C0_CONFIG4	$16,4
#define C0_CONFIG5	$16,5
#define C0_LLADDR	$17
#define C0_MAAR		$17,1
#define C0_MAARI	$17,2
#define C0_WATCHLO	$18
#define C0_WATCHHI	$19
#define C0_XCONTEXT	$20
#define C0_DEBUG	$23
#define C0_DEBUG2	$23,6
#define C0_DEPC		$24
#define C0_PERFCNT	$25
#define C0_ERRCTL	$26
#define C0_CACHEERR	$27
#define C0_TAGLO	$28
#define C0_ITAGLO	$28
#define C0_DTAGLO	$28,2
#define C0_TAGLO2	$28,4
#define C0_DATALO	$28,1
#define C0_IDATALO	$28,1
#define C0_DDATALO	$28,3
#define C0_DATALO2	$28,5
#define C0_TAGHI	$29
#define C0_ITAGHI	$29
#define C0_DTAGHI	$29,2
#define C0_DATAHI	$29,1
#define C0_IDATAHI	$29,1
#define C0_DDATAHI	$29,3
#define C0_ERRPC	$30
#define C0_DESAVE	$31
#define C0_KSCRATCH1	$31,2
#define C0_KSCRATCH2	$31,3
#define C0_KSCRATCH3	$31,4
#define C0_KSCRATCH4	$31,5
#define C0_KSCRATCH5	$31,6
#define C0_KSCRATCH6	$31,7

$index		=	$0
$random		=	$1
$entrylo0	=	$2
$entrylo1	=	$3
$context	=	$4
$pagemask	=	$5
$wired		=	$6
$hwrena		=	$7
$vaddr 		=	$8
$badvaddr	=	$8
$count 		=	$9
$entryhi	=	$10
$compare	=	$11
$sr		=	$12
$cr		=	$13
$epc 		=	$14
$prid		=	$15
$config		=	$16
$lladdr		=	$17
$watchlo	=	$18
$watchhi	=	$19
$debug		= 	$23
$depc		= 	$24
$perfcnt	= 	$25
$errctl		=	$26
$cacheerr	=	$27
$taglo		=	$28
$taghi		=	$29
$errpc		=	$30
$desave		=	$31


#else /* !__ASSEMBLER__ */

/*
 * Standard types
 */
typedef unsigned int		reg32_t;	/* a 32-bit register */
typedef unsigned long long	reg64_t;	/* a 64-bit register */
#if _MIPS_SIM==_ABIO32
typedef unsigned int		reg_t;
typedef signed int		sreg_t;
#else
typedef unsigned long long	reg_t;
typedef signed long long	sreg_t;
#endif

/*
 * MIPS32 Coprocessor 0 register encodings for C use.
 * These encodings are implementation specific.
 */
#define C0_INDEX	0
#define C0_INX		0
#define C0_RANDOM	1
#define C0_RAND		1
#define C0_ENTRYLO0	2
#define C0_TLBLO0	2
#define C0_ENTRYLO1	3
#define C0_TLBLO1	3
#define C0_GLOBAL	0x103
#define C0_CONTEXT	4
#define C0_CTXT		4
#define C0_CONTEXTCONF	0x104
#define C0_USERLOCAL	0x204
#define C0_XCONTEXTCONF	0x304
#define C0_DEBUGCTXTID	0x404
#define C0_PAGEMASK	5
#define C0_PAGEGRAIN	0x105
#define C0_SEGCTL0	0x205
#define C0_SEGCTL1	0x305
#define C0_SEGCTL2	0x405
#define C0_PWBASE	0x505
#define C0_PWFIELD	0x605
#define C0_PWSIZE	0x705
#define C0_WIRED	6
#define C0_PWCTL	0x606
#define C0_HWRENA	7
#define C0_BADVADDR 	8
#define C0_VADDR 	8
#define C0_BADINSTR 	0x108
#define C0_BADINSTRP 	0x208
#define C0_COUNT 	9
#define C0_ENTRYHI	10
#define C0_TLBHI	10
#define C0_COMPARE	11
#define C0_STATUS	12
#define C0_SR		12
#define C0_INTCTL	0x10C
#define C0_SRSCTL	0x20C
#define C0_SRSMAP	0x30C
#define C0_CAUSE	13
#define C0_CR		13
#define C0_NESTEDEXC	0x50D
#define C0_EPC 		14
#define C0_NEPC		0x20E
#define C0_PRID		15
#define C0_EBASE	0x10F
#define C0_CDMMBASE	0x20F
#define C0_CMGCRBASE	0x30F
#define C0_BEVVA	0x40F
#define C0_CONFIG	16
#define C0_CONFIG0	16
#define C0_CONFIG1	0x110
#define C0_CONFIG2	0x210
#define C0_CONFIG3	0x310
#define C0_CONFIG4	0x410
#define C0_CONFIG5	0x510
#define C0_LLADDR	17
#define C0_MAAR		0x111
#define C0_MAARI	0x111
#define C0_WATCHLO	18
#define C0_WATCHHI	19
#define C0_XCONTEXT	20
#define C0_DEBUG	23
#define C0_DEBUG2	0x617
#define C0_DEPC		24
#define C0_PERFCNT	25
#define C0_ERRCTL	26
#define C0_CACHEERR	27
#define C0_TAGLO	28
#define C0_ITAGLO	28
#define C0_DTAGLO	0x21C
#define C0_TAGLO2	0x41C
#define C0_DATALO	0x11C
#define C0_IDATALO	0x11C
#define C0_DDATALO	0x31C
#define C0_DATALO2	0x51C
#define C0_TAGHI	29
#define C0_ITAGHI	29
#define C0_DTAGHI	0x21D
#define C0_DATAHI	0x11D
#define C0_IDATAHI	0x11D
#define C0_DDATAHI	0x31D
#define C0_ERRPC	30
#define C0_DESAVE	31
#define C0_KSCRATCH1	0x21F
#define C0_KSCRATCH2	0x31F
#define C0_KSCRATCH3	0x41F
#define C0_KSCRATCH4	0x51F
#define C0_KSCRATCH5	0x61F
#define C0_KSCRATCH6	0x71F

#ifdef __cplusplus
extern "C" {
#endif

#define _mips_sync() __asm__ __volatile__ ("sync" : : : "memory")

/* wait for unmasked interrupt */
#define _mips_wait() \
  __asm__ __volatile ("wait")

/*
 * Define macros for accessing the MIPS32 coprocessor 0 registers. Most apart
 * from "set" return the original register value. These macros take an encoded
 * (register, select) combination, so they can use the textual names above.
 */

#define MIPS_C0_REGNAME(regno, sel) ((sel << 8) + regno)

#define mips32_get_c0(selreg) \
__extension__ ({ \
  register unsigned long __r; \
  __asm__ __volatile ("mfc0 %0,$%1,%2" \
		      : "=d" (__r) \
		      : "JK" (selreg & 0x1F), "JK" (selreg >> 8)); \
  __r; \
})

#define mips32_set_c0(selreg, val) \
do { \
    __asm__ __volatile (".set push \n"\
			".set noreorder\n"\
			"mtc0 %z0,$%1,%2\n"\
			"ehb\n" \
			".set pop" \
			: \
			: "dJ" ((reg32_t)(val)), "JK" (selreg & 0x1F),\
			  "JK" (selreg >> 8) \
			: "memory"); \
} while (0)

#define mips32_xch_c0(selreg, val) \
__extension__ ({ \
    register reg32_t __o; \
    __o = mips32_get_c0 (selreg); \
    mips32_set_c0 (selreg, val); \
    __o; \
})

#define mips32_bc_c0(selreg, clr) \
__extension__ ({ \
    register reg32_t __o; \
    __o = mips32_get_c0 (selreg); \
    mips32_set_c0 (selreg, __o & ~(clr)); \
    __o; \
})

#define mips32_bs_c0(selreg, set) \
__extension__ ({ \
    register reg32_t __o; \
    __o = mips32_get_c0 (selreg); \
    mips32_set_c0 (selreg, __o | (set)); \
    __o; \
})

#define mips32_bcs_c0(selreg, clr, set) \
__extension__ ({ \
    register reg32_t __o; \
    __o = mips32_get_c0 (selreg); \
    mips32_set_c0 (selreg, (__o & ~(clr)) | (set)); \
    __o; \
})

/* generic equivalents for mips/cpu.h */
#define _mips_mfc0(r)		mips32_get_c0(r)
#define _mips_mtc0(r,v)		mips32_set_c0(r,v)

/* MIPS32 Config0 register */
#define mips32_getconfig0()	mips32_get_c0(C0_CONFIG0)
#define mips32_setconfig0(v)	mips32_set_c0(C0_CONFIG0,v)
#define mips32_xchconfig0(v)	mips32_xch_c0(C0_CONFIG0,v)
#define mips32_bicconfig0(clr)	mips32_bc_c0(C0_CONFIG0,clr)
#define mips32_bisconfig0(set)	mips32_bs_c0(C0_CONFIG0,set)
#define mips32_bcsconfig0(c,s)	mips32_bcs_c0(C0_CONFIG0,c,s)

/* MIPS32 Config1, 2, 3, 4, 5 register */
#define mips32_getconfig1()	mips32_get_c0(C0_CONFIG1)
#define mips32_setconfig1(v)	mips32_set_c0(C0_CONFIG1,v)
#define mips32_xchconfig1(v)	mips32_xch_c0(C0_CONFIG1,v)
#define mips32_bicconfig1(clr)	mips32_bc_c0(C0_CONFIG1,clr)
#define mips32_bisconfig1(set)	mips32_bs_c0(C0_CONFIG1,set)
#define mips32_bcsconfig1(c,s)	mips32_bcs_c0(C0_CONFIG1,c,s)

#define mips32_getconfig2()	mips32_get_c0(C0_CONFIG2)
#define mips32_setconfig2(v)	mips32_set_c0(C0_CONFIG2,v)
#define mips32_xchconfig2(v)	mips32_xch_c0(C0_CONFIG2,v)
#define mips32_bicconfig2(clr)	mips32_bc_c0(C0_CONFIG2,clr)
#define mips32_bisconfig2(set)	mips32_bs_c0(C0_CONFIG2,set)
#define mips32_bcsconfig2(c,s)	mips32_bcs_c0(C0_CONFIG2,c,s)

#define mips32_getconfig3()	mips32_get_c0(C0_CONFIG3)
#define mips32_setconfig3(v)	mips32_set_c0(C0_CONFIG3,v)
#define mips32_xchconfig3(v)	mips32_xch_c0(C0_CONFIG3,v)
#define mips32_bicconfig3(clr)	mips32_bc_c0(C0_CONFIG3,clr)
#define mips32_bisconfig3(set)	mips32_bs_c0(C0_CONFIG3,set)
#define mips32_bcsconfig3(c,s)	mips32_bcs_c0(C0_CONFIG3,c,s)

#define mips32_getconfig4()	mips32_get_c0(C0_CONFIG4)
#define mips32_setconfig4(v)	mips32_set_c0(C0_CONFIG4,v)
#define mips32_xchconfig4(v)	mips32_xch_c0(C0_CONFIG4,v)
#define mips32_bicconfig4(clr)	mips32_bc_c0(C0_CONFIG4,clr)
#define mips32_bisconfig4(set)	mips32_bs_c0(C0_CONFIG4,set)
#define mips32_bcsconfig4(c,s)	mips32_bcs_c0(C0_CONFIG4,c,s)

#define mips32_getconfig5()	mips32_get_c0(C0_CONFIG5)
#define mips32_setconfig5(v)	mips32_set_c0(C0_CONFIG5,v)
#define mips32_xchconfig5(v)	mips32_xch_c0(C0_CONFIG5,v)
#define mips32_bicconfig5(clr)	mips32_bc_c0(C0_CONFIG5,clr)
#define mips32_bisconfig5(set)	mips32_bs_c0(C0_CONFIG5,set)
#define mips32_bcsconfig5(c,s)	mips32_bcs_c0(C0_CONFIG5,c,s)

/* MIPS32 Debug register */
#define mips32_getdebug()	mips32_get_c0(C0_DEBUG)
#define mips32_setdebug(v)	mips32_set_c0(C0_DEBUG,v)
#define mips32_xchdebug(v)	mips32_xch_c0(C0_DEBUG,v)
#define mips32_bicdebug(clr)	mips32_bc_c0(C0_DEBUG,clr)
#define mips32_bisdebug(set)	mips32_bs_c0(C0_DEBUG,set)
#define mips32_bcsdebug(c,s)	mips32_bcs_c0(C0_DEBUG,c,s)

/* MIPS32 ErrCtl register */
#define mips32_geterrctl()	mips32_get_c0(C0_ERRCTL)
#define mips32_seterrctl(x)	mips32_set_c0(C0_ERRCTL,x)
#define mips32_xcherrctl(x)	mips32_xch_c0(C0_ERRCTL,x)
#define mips32_bicerrctl(clr)	mips32_bc_c0(C0_ERRCTL,clr)
#define mips32_biserrctl(set)	mips32_bs_c0(C0_ERRCTL,set)
#define mips32_bcserrctl(c,s)	mips32_bcs_c0(C0_ERRCTL,c,s)

/* MIPS32 TagLo register */
#define mips32_getitaglo()	mips32_get_c0(C0_TAGLO)		/* alias define */
#define mips32_setitaglo(x)	mips32_set_c0(C0_TAGLO,x)	/* alias define */
#define mips32_xchitaglo(x)	mips32_xch_c0(C0_TAGLO,x)	/* alias define */
#define mips32_getdtaglo()	mips32_get_c0(MIPS_C0_REGNAME(C0_TAGLO, 2))
#define mips32_setdtaglo(x)	mips32_set_c0(MIPS_C0_REGNAME(C0_TAGLO, 2),x)
#define mips32_xchdtaglo(x)	mips32_xch_c0(MIPS_C0_REGNAME(C0_TAGLO, 2),x)
#define mips32_gettaglo2()	mips32_get_c0(MIPS_C0_REGNAME(C0_TAGLO, 4))
#define mips32_settaglo2(x)	mips32_set_c0(MIPS_C0_REGNAME(C0_TAGLO, 4),x)
#define mips32_xchtaglo2(x)	mips32_xch_c0(MIPS_C0_REGNAME(C0_TAGLO, 4),x)

/* MIPS32 DataLo register */
#define mips32_getdatalo()	mips32_get_c0(MIPS_C0_REGNAME(C0_TAGLO, 1))
#define mips32_setdatalo(x)	mips32_set_c0(MIPS_C0_REGNAME(C0_TAGLO, 1),x)
#define mips32_xchdatalo(x)	mips32_xch_c0(MIPS_C0_REGNAME(C0_TAGLO, 1),x)
#define mips32_getidatalo()	mips32_getdatalo()	/* alias define */
#define mips32_setidatalo(x)	mips32_setdatalo(x)	/* alias define */
#define mips32_xchidatalo(x)	mips32_xchdatalo(x)	/* alias define */
#define mips32_getddatalo()	mips32_get_c0(MIPS_C0_REGNAME(C0_TAGLO, 3))
#define mips32_setddatalo(x)	mips32_set_c0(MIPS_C0_REGNAME(C0_TAGLO, 3),x)
#define mips32_xchddatalo(x)	mips32_xch_c0(MIPS_C0_REGNAME(C0_TAGLO, 3),x)
#define mips32_getdatalo2()	mips32_get_c0(MIPS_C0_REGNAME(C0_TAGLO, 5))
#define mips32_setdatalo2(x)	mips32_set_c0(MIPS_C0_REGNAME(C0_TAGLO, 5),x)
#define mips32_xchdatalo2(x)	mips32_xch_c0(MIPS_C0_REGNAME(C0_TAGLO, 5),x)

/* MIPS32r2 IntCtl register */
#define mips32_getintctl()	mips32_get_c0(C0_INTCTL)
#define mips32_setintctl(x)	mips32_set_c0(C0_INTCTL,x)
#define mips32_xchintctl(x)	mips32_xch_c0(C0_INTCTL,x)

/* MIPS32r2 SRSCtl register */
#define mips32_getsrsctl()	mips32_get_c0(C0_SRSCTL)
#define mips32_setsrsctl(x)	mips32_set_c0(C0_SRSCTL,x)
#define mips32_xchsrsctl(x)	mips32_xch_c0(C0_SRSCTL,x)

/* MIPS32r2 SRSMap register */
#define mips32_getsrsmapl()	mips32_get_c0(C0_SRSMAP)
#define mips32_setsrsmap(x)	mips32_set_c0(C0_SRSMAP,x)
#define mips32_xchsrsmap(x)	mips32_xch_c0(C0_SRSMAP,x)

/* MIPS32r2/SmartMIPS PageGrain register */
#define mips32_getpagegrain()	mips32_get_c0(C0_PAGEGRAIN)
#define mips32_setpagegrain(x)	mips32_set_c0(C0_PAGEGRAIN,x)
#define mips32_xchpagegrain(x)	mips32_xch_c0(C0_PAGEGRAIN,x)

/* MIPS32r2 HWREna register */
#define mips32_gethwrena()	mips32_get_c0(C0_HWRENA)
#define mips32_sethwrena(v)	mips32_set_c0(C0_HWRENA,v)
#define mips32_xchhwrena(v)	mips32_xch_c0(C0_HWRENA,v)
#define mips32_bichwrena(clr)	mips32_bc_c0(C0_HWRENA,clr)
#define mips32_bishwrena(set)	mips32_bs_c0(C0_HWRENA,set)
#define mips32_bcshwrena(c,s)	mips32_bcs_c0(C0_HWRENA,c,s)

/* MIPS32r2 EBase register */
#define mips32_getebase()	mips32_get_c0(C0_EBASE)
#define mips32_setebase(x)	mips32_set_c0(C0_EBASE,x)
#define mips32_xchebase(x)	mips32_xch_c0(C0_EBASE,x)

/* CP0 Status register (NOTE: not atomic operations) */
#define mips32_getsr()		mips32_get_c0(C0_SR)
#define mips32_setsr(v)		mips32_set_c0(C0_SR,v)
#define mips32_xchsr(v)		mips32_xch_c0(C0_SR,v)
#define mips32_bicsr(clr)	mips32_bc_c0(C0_SR,clr)
#define mips32_bissr(set)	mips32_bs_c0(C0_SR,set)
#define mips32_bcssr(c,s)	mips32_bcs_c0(C0_SR,c,s)

/* CP0 Cause register (NOTE: not atomic operations) */
#define mips32_getcr()		mips32_get_c0(C0_CR)
#define mips32_setcr(v)		mips32_set_c0(C0_CR,v)
#define mips32_xchcr(v)		mips32_xch_c0(C0_CR,v)
#define mips32_biccr(clr)	mips32_bc_c0(C0_CR,clr)
#define mips32_biscr(set)	mips32_bs_c0(C0_CR,set)
#define mips32_bcscr(c,s)	mips32_bcs_c0(C0_CR,c,s)

/* CP0 PrID register */
#define mips32_getprid()	mips32_get_c0(C0_PRID)

#ifdef C0_COUNT
/* CP0 Count register */
#define mips32_getcount()	mips32_get_c0(C0_COUNT)
#define mips32_setcount(v)	mips32_set_c0(C0_COUNT,v)
#define mips32_xchcount(v)	mips32_xch_c0(C0_COUNT,v)
#endif

#ifdef C0_COMPARE
/* CP0 Compare register*/
#define mips32_getcompare()	mips32_get_c0(C0_COMPARE)
#define mips32_setcompare(v)	mips32_set_c0(C0_COMPARE,v)
#define mips32_xchcompare(v)	mips32_xch_c0(C0_COMPARE,v)
#endif

#ifdef C0_CONFIG
/* CP0 Config register */
#define mips32_getconfig()	mips32_get_c0(C0_CONFIG)
#define mips32_setconfig(v)	mips32_set_c0(C0_CONFIG,v)
#define mips32_xchconfig(v)	mips32_xch_c0(C0_CONFIG,v)
#define mips32_bicconfig(c)	mips32_bc_c0(C0_CONFIG,c)
#define mips32_bisconfig(s)	mips32_bs_c0(C0_CONFIG,s)
#define mips32_bcsconfig(c,s)	mips32_bcs_c0(C0_CONFIG,c,s)
#endif

#ifdef C0_ECC
/* CP0 ECC register */
#define mips32_getecc()		mips32_get_c0(C0_ECC)
#define mips32_setecc(x)	mips32_set_c0(C0_ECC, x)
#define mips32_xchecc(x)	mips32_xch_c0(C0_ECC, x)
#endif

#ifdef C0_TAGHI
/* CP0 TagHi register */
#define mips32_gettaghi()	mips32_get_c0(C0_TAGHI)
#define mips32_settaghi(x)	mips32_set_c0(C0_TAGHI, x)
#define mips32_xchtaghi(x)	mips32_xch_c0(C0_TAGHI, x)
#endif

#ifdef C0_WATCHLO
/* CP0 WatchLo register */
#define mips32_getwatchlo()	mips32_get_c0(C0_WATCHLO)
#define mips32_setwatchlo(x)	mips32_set_c0(C0_WATCHLO, x)
#define mips32_xchwatchlo(x)	mips32_xch_c0(C0_WATCHLO, x)
#endif

#ifdef C0_WATCHHI
/* CP0 WatchHi register */
#define mips32_getwatchhi()	mips32_get_c0(C0_WATCHHI)
#define mips32_setwatchhi(x)	mips32_set_c0(C0_WATCHHI, x)
#define mips32_xchwatchhi(x)	mips32_xch_c0(C0_WATCHHI, x)
#endif

/*
 * Define macros for accessing the MIPS32 coprocessor 0 registers.  Most
 * apart from "set" return the original register value.  These particular
 * macros take (reg, sel) as separate paramters, so they can't be used with
 * the coprocessor 0 register names above.
 */
#define _m32c0_mfc0(reg, sel) \
__extension__ ({ \
  register unsigned long __r; \
  __asm__ __volatile ("mfc0 %0,$%1,%2" \
		      : "=d" (__r) \
      		      : "JK" (reg), "JK" (sel)); \
  __r; \
})

#define _m32c0_mtc0(reg, sel, val) \
do { \
    __asm__ __volatile (".set push \n"\
			".set noreorder\n"\
			"mtc0 %z0,$%1,%2\n"\
			"ehb\n" \
			".set pop" \
			: \
			: "dJ" ((reg32_t)(val)), "JK" (reg), "JK" (sel) \
			: "memory"); \
} while (0)

#define _m32c0_mxc0(reg, sel, val) \
__extension__ ({ \
    register reg32_t __o; \
    __o = _m32c0_mfc0 (reg, sel); \
    _m32c0_mtc0 (reg, sel, val); \
    __o; \
})

#define _m32c0_bcc0(reg, sel, clr) \
__extension__ ({ \
    register reg32_t __o; \
    __o = _m32c0_mfc0 (reg, sel); \
    _m32c0_mtc0 (reg, sel, __o & ~(clr)); \
    __o; \
})

#define _m32c0_bsc0(reg, sel, set) \
__extension__ ({ \
    register reg32_t __o; \
    __o = _m32c0_mfc0 (reg, sel); \
    _m32c0_mtc0 (reg, sel, __o | (set)); \
    __o; \
})

#define _m32c0_bcsc0(reg, sel, clr, set) \
__extension__ ({ \
    register reg32_t __o; \
    __o = _m32c0_mfc0 (reg, sel); \
    _m32c0_mtc0 (reg, sel, (__o & ~(clr)) | (set)); \
    __o; \
})

#ifdef __cplusplus
}
#endif

/* Define MIPS32 user-level intrinsics */
#include <mips/mips32.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CP0 intrinsics */

/* MIPS32r2 atomic interrupt disable */
#define _mips_intdisable() __extension__({ \
    unsigned int __v; \
    __asm__ __volatile__ ("di %0; ehb" : "=d" (__v)); \
    __v; \
})

/* MIPS32r2 atomic interrupt restore */
#define _mips_intrestore(x) \
    mips_setsr (x)

/* MIPS32r2 set SRSCtl.PSS (previous shadow set), returning old value */
extern unsigned int _mips32r2_xchsrspss (unsigned int);

/* MIPS32r2 write previous gpr */
#define _mips32r2_wrpgpr(regno, val) \
do { \
    __asm __volatile ("wrpgpr $%0,%z1" \
        	      : /* no outputs */ \
 		      : "JK" (regno), "dJ" (val)); \
} while (0)

/* MIPS32r2 read previous gpr */
#define _mips32r2_rdpgpr(regno) \
__extension__({ \
    reg_t __val; \
    __asm __volatile ("rdpgpr %0,$%1" \
        	      : "=d" (__val) \
 		      : "JK" (regno)); \
    __val; \
})

#endif /* __ASSEMBLER__ */

/* MIPS32 PREF instruction hint codes */
#define PREF_LOAD		0
#define PREF_STORE		1
#define PREF_LOAD_STREAMED	4
#define PREF_STORE_STREAMED	5
#define PREF_LOAD_RETAINED	6
#define PREF_STORE_RETAINED	7
#define PREF_WRITEBACK_INVAL	25
#define PREF_PREPAREFORSTORE	30

#ifdef __cplusplus
}
#endif
#endif /* _M32C0_H_ */
