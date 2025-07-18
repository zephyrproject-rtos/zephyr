/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sw_isr_table.h>
#include <zephyr/irq.h>
#include <kswap.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/drivers/clock_control/renesas_rx_cgc.h>

typedef void (*fp)(void);
extern void _start(void);
extern void z_rx_irq_exit(void);

/* this is mainly to give Visual Studio Code peace of mind */
#ifndef CONFIG_GEN_IRQ_START_VECTOR
#define CONFIG_GEN_IRQ_START_VECTOR 0
#endif

#define EXVECT_SECT __attribute__((section(".exvectors")))
#define RVECT_SECT  __attribute__((section(".rvectors")))
#define FVECT_SECT  __attribute__((section(".fvectors")))

#define __ISR__ __attribute__((interrupt, naked))

#define SET_OFS1_HOCO_BITS(reg, freq)                                                              \
	((reg) & ~(0b11 << 12)) | ((((freq) == 24000000   ? 0b10                                   \
				     : (freq) == 32000000 ? 0b11                                   \
				     : (freq) == 48000000 ? 0b01                                   \
				     : (freq) == 64000000 ? 0b00                                   \
							  : 0b11)                                  \
				    << 12))

static ALWAYS_INLINE void REGISTER_SAVE(void)
{
	__asm volatile(
		/* Save the Registers to ISP at the top of ISR.					*/
		/* This code is relate on arch_new_thread() at thread.c				*/
		/* You should store the registers at the same registers arch_new_thread()	*/
		/* except PC and PSW.								*/
		"PUSHM		R1-R15\n"

		"MVFACHI	R15\n"
		"PUSH.L		R15\n"
		"MVFACMI	R15\n"
		"SHLL		#16, R15\n"
		"PUSH.L		R15\n");
}

static ALWAYS_INLINE void REGISTER_RESTORE_EXIT(void)
{
	__asm volatile(
		/* Restore the registers and do the RTE at the End of ISR. */
		"POP		R15\n"
		"MVTACLO	R15\n"
		"POP		R15\n"
		"MVTACHI	R15\n"

		"POPM		R1-R15\n"
		"RTE\n");
}

/* Privileged instruction execption */
static void __ISR__ INT_Excep_SuperVisorInst(void)
{
	REGISTER_SAVE();
	ISR_DIRECT_HEADER();
	z_rx_fatal_error(K_ERR_CPU_EXCEPTION, NULL);
	ISR_DIRECT_FOOTER(1);
	REGISTER_RESTORE_EXIT();
}

/* Access exception */
static void __ISR__ INT_Excep_AccessInst(void)
{
	REGISTER_SAVE();
	ISR_DIRECT_HEADER();
	z_rx_fatal_error(K_ERR_CPU_EXCEPTION, NULL);
	ISR_DIRECT_FOOTER(1);
	REGISTER_RESTORE_EXIT();
}

/* Undefined instruction exception */
static void __ISR__ INT_Excep_UndefinedInst(void)
{
	REGISTER_SAVE();
	ISR_DIRECT_HEADER();
	z_rx_fatal_error(K_ERR_CPU_EXCEPTION, NULL);
	ISR_DIRECT_FOOTER(1);
	REGISTER_RESTORE_EXIT();
}

/* floating point exception */
static void __ISR__ INT_Excep_FloatingPoint(void)
{
	REGISTER_SAVE();
	ISR_DIRECT_HEADER();
	z_rx_fatal_error(K_ERR_CPU_EXCEPTION, NULL);
	ISR_DIRECT_FOOTER(1);
	REGISTER_RESTORE_EXIT();
}

/* Non-maskable interrupt */
static void __ISR__ INT_NonMaskableInterrupt(void)
{
	REGISTER_SAVE();
	ISR_DIRECT_HEADER();
	z_fatal_error(K_ERR_CPU_EXCEPTION, NULL);
	ISR_DIRECT_FOOTER(1);
	REGISTER_RESTORE_EXIT();
}

/* dummy function */
static void __ISR__ Dummy(void)
{
	REGISTER_SAVE();
	ISR_DIRECT_HEADER();
	ISR_DIRECT_FOOTER(1);
	REGISTER_RESTORE_EXIT();
}

/**
 * @brief select Zephyr ISR and argument from software ISR table and call
 * function
 *
 * @param irq    interrupt to handle
 */
static ALWAYS_INLINE void handle_interrupt(uint8_t irq)
{
	ISR_DIRECT_HEADER();
	_sw_isr_table[irq].isr(_sw_isr_table[irq].arg);
	ISR_DIRECT_FOOTER(1);
}

/**
 * @brief isr for reserved interrupts (0-15) that are not handled through
 *        the zephyr sw isr table
 */
static void __ISR__ reserved_isr(void)
{
	REGISTER_SAVE();
	ISR_DIRECT_HEADER();
	z_fatal_error(K_ERR_CPU_EXCEPTION, NULL);
	ISR_DIRECT_FOOTER(1);
	REGISTER_RESTORE_EXIT();
}

static void __ISR__ INT_RuntimeFatalInterrupt(void)
{
	REGISTER_SAVE();
	ISR_DIRECT_HEADER();

	uint32_t reason;
	const struct arch_esf *esf;

	/* Read the current values of CPU registers r1 and r0 into C variables
	 * 'reason' is expected to contain the exception reason (from r1)
	 * 'esf' is expected to contain a pointer to the exception stack frame (from r0)
	 */
	__asm__ volatile("mov r1, %0\n\t"
			 "mov r0, %1\n\t"
			 : "=r"(reason), "=r"(esf));

	z_rx_fatal_error(reason, esf);

	ISR_DIRECT_FOOTER(1);
	REGISTER_RESTORE_EXIT();
}

/* wrapper for z_rx_context_switch_isr, defined in switch.S */
extern void __ISR__ switch_isr_wrapper(void);

/* this macro is used to define "demuxing" ISRs for all interrupts that are
 * handled through Zephyr's software isr table.
 */

#define INT_DEMUX(irq)                                                                             \
	static __attribute__((interrupt, naked)) void int_demux_##irq(void)                        \
	{                                                                                          \
		REGISTER_SAVE();                                                                   \
		handle_interrupt(irq - CONFIG_GEN_IRQ_START_VECTOR);                               \
		REGISTER_RESTORE_EXIT();                                                           \
	}

INT_DEMUX(16);
INT_DEMUX(17);
INT_DEMUX(18);
INT_DEMUX(19);
INT_DEMUX(20);
INT_DEMUX(21);
INT_DEMUX(22);
INT_DEMUX(23);
INT_DEMUX(24);
INT_DEMUX(25);
INT_DEMUX(27);
INT_DEMUX(26);
INT_DEMUX(28);
INT_DEMUX(29);
INT_DEMUX(30);
INT_DEMUX(31);
INT_DEMUX(32);
INT_DEMUX(33);
INT_DEMUX(34);
INT_DEMUX(35);
INT_DEMUX(36);
INT_DEMUX(37);
INT_DEMUX(38);
INT_DEMUX(39);
INT_DEMUX(40);
INT_DEMUX(41);
INT_DEMUX(42);
INT_DEMUX(43);
INT_DEMUX(44);
INT_DEMUX(45);
INT_DEMUX(46);
INT_DEMUX(47);
INT_DEMUX(48);
INT_DEMUX(49);
INT_DEMUX(50);
INT_DEMUX(51);
INT_DEMUX(52);
INT_DEMUX(53);
INT_DEMUX(54);
INT_DEMUX(55);
INT_DEMUX(56);
INT_DEMUX(57);
INT_DEMUX(58);
INT_DEMUX(59);
INT_DEMUX(60);
INT_DEMUX(61);
INT_DEMUX(62);
INT_DEMUX(63);
INT_DEMUX(64);
INT_DEMUX(65);
INT_DEMUX(66);
INT_DEMUX(67);
INT_DEMUX(68);
INT_DEMUX(69);
INT_DEMUX(70);
INT_DEMUX(71);
INT_DEMUX(72);
INT_DEMUX(73);
INT_DEMUX(74);
INT_DEMUX(75);
INT_DEMUX(76);
INT_DEMUX(77);
INT_DEMUX(78);
INT_DEMUX(79);
INT_DEMUX(80);
INT_DEMUX(81);
INT_DEMUX(82);
INT_DEMUX(83);
INT_DEMUX(84);
INT_DEMUX(85);
INT_DEMUX(86);
INT_DEMUX(87);
INT_DEMUX(88);
INT_DEMUX(89);
INT_DEMUX(90);
INT_DEMUX(91);
INT_DEMUX(92);
INT_DEMUX(93);
INT_DEMUX(94);
INT_DEMUX(95);
INT_DEMUX(96);
INT_DEMUX(97);
INT_DEMUX(98);
INT_DEMUX(99);
INT_DEMUX(100)
INT_DEMUX(101);
INT_DEMUX(102);
INT_DEMUX(103);
INT_DEMUX(104);
INT_DEMUX(105);
INT_DEMUX(106);
INT_DEMUX(107);
INT_DEMUX(108);
INT_DEMUX(109);
INT_DEMUX(110);
INT_DEMUX(111);
INT_DEMUX(112);
INT_DEMUX(113);
INT_DEMUX(114);
INT_DEMUX(115);
INT_DEMUX(116);
INT_DEMUX(117);
INT_DEMUX(118);
INT_DEMUX(119);
INT_DEMUX(120);
INT_DEMUX(121);
INT_DEMUX(122);
INT_DEMUX(123);
INT_DEMUX(124);
INT_DEMUX(125);
INT_DEMUX(126);
INT_DEMUX(127);
INT_DEMUX(128);
INT_DEMUX(129);
INT_DEMUX(130);
INT_DEMUX(131);
INT_DEMUX(132);
INT_DEMUX(133);
INT_DEMUX(134);
INT_DEMUX(135);
INT_DEMUX(136);
INT_DEMUX(137);
INT_DEMUX(138);
INT_DEMUX(139);
INT_DEMUX(140);
INT_DEMUX(141);
INT_DEMUX(142);
INT_DEMUX(143);
INT_DEMUX(144);
INT_DEMUX(145);
INT_DEMUX(146);
INT_DEMUX(147);
INT_DEMUX(148);
INT_DEMUX(149);
INT_DEMUX(150);
INT_DEMUX(151);
INT_DEMUX(152);
INT_DEMUX(153);
INT_DEMUX(154);
INT_DEMUX(155);
INT_DEMUX(156);
INT_DEMUX(157);
INT_DEMUX(158);
INT_DEMUX(159);
INT_DEMUX(160);
INT_DEMUX(161);
INT_DEMUX(162);
INT_DEMUX(163);
INT_DEMUX(164);
INT_DEMUX(165);
INT_DEMUX(166);
INT_DEMUX(167);
INT_DEMUX(168);
INT_DEMUX(169);
INT_DEMUX(170);
INT_DEMUX(171);
INT_DEMUX(172);
INT_DEMUX(173);
INT_DEMUX(174);
INT_DEMUX(175);
INT_DEMUX(176);
INT_DEMUX(177);
INT_DEMUX(178);
INT_DEMUX(179);
INT_DEMUX(180);
INT_DEMUX(181);
INT_DEMUX(182);
INT_DEMUX(183);
INT_DEMUX(184);
INT_DEMUX(185);
INT_DEMUX(186);
INT_DEMUX(187);
INT_DEMUX(188);
INT_DEMUX(189);
INT_DEMUX(190);
INT_DEMUX(191);
INT_DEMUX(192);
INT_DEMUX(193);
INT_DEMUX(194);
INT_DEMUX(195);
INT_DEMUX(196);
INT_DEMUX(197);
INT_DEMUX(198);
INT_DEMUX(199);
INT_DEMUX(200);
INT_DEMUX(201);
INT_DEMUX(202);
INT_DEMUX(203);
INT_DEMUX(204);
INT_DEMUX(205);
INT_DEMUX(206);
INT_DEMUX(207);
INT_DEMUX(208);
INT_DEMUX(209);
INT_DEMUX(210);
INT_DEMUX(211);
INT_DEMUX(212);
INT_DEMUX(213);
INT_DEMUX(214);
INT_DEMUX(215);
INT_DEMUX(216);
INT_DEMUX(217);
INT_DEMUX(218);
INT_DEMUX(219);
INT_DEMUX(220);
INT_DEMUX(221);
INT_DEMUX(222);
INT_DEMUX(223);
INT_DEMUX(224);
INT_DEMUX(225);
INT_DEMUX(226);
INT_DEMUX(227);
INT_DEMUX(228);
INT_DEMUX(229);
INT_DEMUX(230);
INT_DEMUX(231);
INT_DEMUX(232);
INT_DEMUX(233);
INT_DEMUX(234);
INT_DEMUX(235);
INT_DEMUX(236);
INT_DEMUX(237);
INT_DEMUX(238);
INT_DEMUX(239);
INT_DEMUX(240);
INT_DEMUX(241);
INT_DEMUX(242);
INT_DEMUX(243);
INT_DEMUX(244);
INT_DEMUX(245);
INT_DEMUX(246);
INT_DEMUX(247);
INT_DEMUX(248);
INT_DEMUX(249);
INT_DEMUX(250);
INT_DEMUX(251);
INT_DEMUX(252);
INT_DEMUX(253);
INT_DEMUX(254);
INT_DEMUX(255);

#if !CONFIG_HAS_EXCEPT_VECTOR_TABLE

const void *FixedVectors[] FVECT_SECT = {
	/* 0x00-0x4c: Reserved, must be 0xff (according to e2 studio example) */
	/* Reserved for OFSM */
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)(SET_OFS1_HOCO_BITS(
		0xFFFFFFFF,
		(RX_CGC_PROP_HAS_STATUS_OKAY_OR(DT_NODELABEL(hoco), clock_frequency, 32000000)))),
	(fp)0xFFFFFFFF,
	/* Reserved area */
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	/* Reserved for ID Code */
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	/* Reserved area */
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	/* Reserved area */
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	/* 0x50: Privileged instruction exception */
	INT_Excep_SuperVisorInst,
	/* 0x54: Access exception */
	INT_Excep_AccessInst,
	/* 0x58: Reserved */
	Dummy,
	/* 0x5c: Undefined Instruction Exception */
	INT_Excep_UndefinedInst,
	/* 0x60: Reserved */
	Dummy,
	/* 0x64: Floating Point Exception */
	INT_Excep_FloatingPoint,
	/* 0x68-0x74: Reserved */
	Dummy,
	Dummy,
	Dummy,
	Dummy,
	/* 0x78: Non-maskable interrupt */
	INT_NonMaskableInterrupt,
	_start,
};

#else

/* The reset vector ALWAYS is at address 0xFFFFFFFC. Set it to point at
 * the start routine (in reset.S)
 */
const FVECT_SECT void *resetVector = _start;

/* Exception vector table
 * (see rx-family-rxv2-instruction-set-architecture-users-manual-software)
 */
const void *ExceptVectors[] EXVECT_SECT = {
	/* 0x00-0x4c: Reserved, must be 0xff (according to e2 studio example) */
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	(fp)0xFFFFFFFF,
	/* 0x50: Privileged instruction exception */
	INT_Excep_SuperVisorInst,
	/* 0x54: Access exception */
	INT_Excep_AccessInst,
	/* 0x58: Reserved */
	Dummy,
	/* 0x5c: Undefined Instruction Exception */
	INT_Excep_UndefinedInst,
	/* 0x60: Reserved */
	Dummy,
	/* 0x64: Floating Point Exception */
	INT_Excep_FloatingPoint,
	/* 0x68-0x74: Reserved */
	Dummy,
	Dummy,
	Dummy,
	Dummy,
	/* 0x78: Non-maskable interrupt */
	INT_NonMaskableInterrupt,
};
#endif

const fp RelocatableVectors[] RVECT_SECT = {
	reserved_isr,  switch_isr_wrapper, INT_RuntimeFatalInterrupt,
	reserved_isr,  reserved_isr,       reserved_isr,
	reserved_isr,  reserved_isr,       reserved_isr,
	reserved_isr,  reserved_isr,       reserved_isr,
	reserved_isr,  reserved_isr,       reserved_isr,
	reserved_isr,  int_demux_16,       int_demux_17,
	int_demux_18,  int_demux_19,       int_demux_20,
	int_demux_21,  int_demux_22,       int_demux_23,
	int_demux_24,  int_demux_25,       int_demux_26,
	int_demux_27,  int_demux_28,       int_demux_29,
	int_demux_30,  int_demux_31,       int_demux_32,
	int_demux_33,  int_demux_34,       int_demux_35,
	int_demux_36,  int_demux_37,       int_demux_38,
	int_demux_39,  int_demux_40,       int_demux_41,
	int_demux_42,  int_demux_43,       int_demux_44,
	int_demux_45,  int_demux_46,       int_demux_47,
	int_demux_48,  int_demux_49,       int_demux_50,
	int_demux_51,  int_demux_52,       int_demux_53,
	int_demux_54,  int_demux_55,       int_demux_56,
	int_demux_57,  int_demux_58,       int_demux_59,
	int_demux_60,  int_demux_61,       int_demux_62,
	int_demux_63,  int_demux_64,       int_demux_65,
	int_demux_66,  int_demux_67,       int_demux_68,
	int_demux_69,  int_demux_70,       int_demux_71,
	int_demux_72,  int_demux_73,       int_demux_74,
	int_demux_75,  int_demux_76,       int_demux_77,
	int_demux_78,  int_demux_79,       int_demux_80,
	int_demux_81,  int_demux_82,       int_demux_83,
	int_demux_84,  int_demux_85,       int_demux_86,
	int_demux_87,  int_demux_88,       int_demux_89,
	int_demux_90,  int_demux_91,       int_demux_92,
	int_demux_93,  int_demux_94,       int_demux_95,
	int_demux_96,  int_demux_97,       int_demux_98,
	int_demux_99,  int_demux_100,      int_demux_101,
	int_demux_102, int_demux_103,      int_demux_104,
	int_demux_105, int_demux_106,      int_demux_107,
	int_demux_108, int_demux_109,      int_demux_110,
	int_demux_111, int_demux_112,      int_demux_113,
	int_demux_114, int_demux_115,      int_demux_116,
	int_demux_117, int_demux_118,      int_demux_119,
	int_demux_120, int_demux_121,      int_demux_122,
	int_demux_123, int_demux_124,      int_demux_125,
	int_demux_126, int_demux_127,      int_demux_128,
	int_demux_129, int_demux_130,      int_demux_131,
	int_demux_132, int_demux_133,      int_demux_134,
	int_demux_135, int_demux_136,      int_demux_137,
	int_demux_138, int_demux_139,      int_demux_140,
	int_demux_141, int_demux_142,      int_demux_143,
	int_demux_144, int_demux_145,      int_demux_146,
	int_demux_147, int_demux_148,      int_demux_149,
	int_demux_150, int_demux_151,      int_demux_152,
	int_demux_153, int_demux_154,      int_demux_155,
	int_demux_156, int_demux_157,      int_demux_158,
	int_demux_159, int_demux_160,      int_demux_161,
	int_demux_162, int_demux_163,      int_demux_164,
	int_demux_165, int_demux_166,      int_demux_167,
	int_demux_168, int_demux_169,      int_demux_170,
	int_demux_171, int_demux_172,      int_demux_173,
	int_demux_174, int_demux_175,      int_demux_176,
	int_demux_177, int_demux_178,      int_demux_179,
	int_demux_180, int_demux_181,      int_demux_182,
	int_demux_183, int_demux_184,      int_demux_185,
	int_demux_186, int_demux_187,      int_demux_188,
	int_demux_189, int_demux_190,      int_demux_191,
	int_demux_192, int_demux_193,      int_demux_194,
	int_demux_195, int_demux_196,      int_demux_197,
	int_demux_198, int_demux_199,      int_demux_200,
	int_demux_201, int_demux_202,      int_demux_203,
	int_demux_204, int_demux_205,      int_demux_206,
	int_demux_207, int_demux_208,      int_demux_209,
	int_demux_210, int_demux_211,      int_demux_212,
	int_demux_213, int_demux_214,      int_demux_215,
	int_demux_216, int_demux_217,      int_demux_218,
	int_demux_219, int_demux_220,      int_demux_221,
	int_demux_222, int_demux_223,      int_demux_224,
	int_demux_225, int_demux_226,      int_demux_227,
	int_demux_228, int_demux_229,      int_demux_230,
	int_demux_231, int_demux_232,      int_demux_233,
	int_demux_234, int_demux_235,      int_demux_236,
	int_demux_237, int_demux_238,      int_demux_239,
	int_demux_240, int_demux_241,      int_demux_242,
	int_demux_243, int_demux_244,      int_demux_245,
	int_demux_246, int_demux_247,      int_demux_248,
	int_demux_249, int_demux_250,      int_demux_251,
	int_demux_252, int_demux_253,      int_demux_254,
	int_demux_255,
};
