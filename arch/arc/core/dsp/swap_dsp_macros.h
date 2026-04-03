/*
 * Copyright (c) 2022 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief save and load macro for ARCv2 DSP and AGU regs
 *
 */
.macro _save_dsp_regs
#ifdef CONFIG_DSP_SHARING
	ld_s r13, [r2, ___thread_base_t_user_options_OFFSET]
	bbit0 r13, K_DSP_IDX, dsp_skip_save
	lr r13, [_ARC_V2_DSP_CTRL]
	st_s r13, [sp, ___callee_saved_stack_t_dsp_ctrl_OFFSET]
	lr r13, [_ARC_V2_ACC0_GLO]
	st_s r13, [sp, ___callee_saved_stack_t_acc0_glo_OFFSET]
	lr r13, [_ARC_V2_ACC0_GHI]
	st_s r13, [sp, ___callee_saved_stack_t_acc0_ghi_OFFSET]
#ifdef CONFIG_ARC_DSP_BFLY_SHARING
	lr r13, [_ARC_V2_DSP_BFLY0]
	st_s r13, [sp, ___callee_saved_stack_t_dsp_bfly0_OFFSET]
	lr r13, [_ARC_V2_DSP_FFT_CTRL]
	st_s r13, [sp, ___callee_saved_stack_t_dsp_fft_ctrl_OFFSET]
#endif
#endif
dsp_skip_save :
#ifdef CONFIG_ARC_AGU_SHARING
	_save_agu_regs
#endif
.endm

.macro _save_agu_regs
#ifdef CONFIG_ARC_AGU_SHARING
	ld_s r13, [r2, ___thread_base_t_user_options_OFFSET]
	btst r13, K_AGU_IDX

	jeq agu_skip_save

	lr r13, [_ARC_V2_AGU_AP0]
	st r13, [sp, ___callee_saved_stack_t_agu_ap0_OFFSET]
	lr r13, [_ARC_V2_AGU_AP1]
	st r13, [sp, ___callee_saved_stack_t_agu_ap1_OFFSET]
	lr r13, [_ARC_V2_AGU_AP2]
	st r13, [sp, ___callee_saved_stack_t_agu_ap2_OFFSET]
	lr r13, [_ARC_V2_AGU_AP3]
	st r13, [sp, ___callee_saved_stack_t_agu_ap3_OFFSET]
	lr r13, [_ARC_V2_AGU_OS0]
	st r13, [sp, ___callee_saved_stack_t_agu_os0_OFFSET]
	lr r13, [_ARC_V2_AGU_OS1]
	st r13, [sp, ___callee_saved_stack_t_agu_os1_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD0]
	st r13, [sp, ___callee_saved_stack_t_agu_mod0_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD1]
	st r13, [sp, ___callee_saved_stack_t_agu_mod1_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD2]
	st r13, [sp, ___callee_saved_stack_t_agu_mod2_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD3]
	st r13, [sp, ___callee_saved_stack_t_agu_mod3_OFFSET]
#ifdef CONFIG_ARC_AGU_MEDIUM
	lr r13, [_ARC_V2_AGU_AP4]
	st r13, [sp, ___callee_saved_stack_t_agu_ap4_OFFSET]
	lr r13, [_ARC_V2_AGU_AP5]
	st r13, [sp, ___callee_saved_stack_t_agu_ap5_OFFSET]
	lr r13, [_ARC_V2_AGU_AP6]
	st r13, [sp, ___callee_saved_stack_t_agu_ap6_OFFSET]
	lr r13, [_ARC_V2_AGU_AP7]
	st r13, [sp, ___callee_saved_stack_t_agu_ap7_OFFSET]
	lr r13, [_ARC_V2_AGU_OS2]
	st r13, [sp, ___callee_saved_stack_t_agu_os2_OFFSET]
	lr r13, [_ARC_V2_AGU_OS3]
	st r13, [sp, ___callee_saved_stack_t_agu_os3_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD4]
	st r13, [sp, ___callee_saved_stack_t_agu_mod4_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD5]
	st r13, [sp, ___callee_saved_stack_t_agu_mod5_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD6]
	st r13, [sp, ___callee_saved_stack_t_agu_mod6_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD7]
	st r13, [sp, ___callee_saved_stack_t_agu_mod7_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD8]
	st r13, [sp, ___callee_saved_stack_t_agu_mod8_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD9]
	st r13, [sp, ___callee_saved_stack_t_agu_mod9_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD10]
	st r13, [sp, ___callee_saved_stack_t_agu_mod10_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD11]
	st r13, [sp, ___callee_saved_stack_t_agu_mod11_OFFSET]
#endif
#ifdef CONFIG_ARC_AGU_LARGE
	lr r13, [_ARC_V2_AGU_AP8]
	st r13, [sp, ___callee_saved_stack_t_agu_ap8_OFFSET]
	lr r13, [_ARC_V2_AGU_AP9]
	st r13, [sp, ___callee_saved_stack_t_agu_ap9_OFFSET]
	lr r13, [_ARC_V2_AGU_AP10]
	st r13, [sp, ___callee_saved_stack_t_agu_ap10_OFFSET]
	lr r13, [_ARC_V2_AGU_AP11]
	st r13, [sp, ___callee_saved_stack_t_agu_ap11_OFFSET]
	lr r13, [_ARC_V2_AGU_OS4]
	st r13, [sp, ___callee_saved_stack_t_agu_os4_OFFSET]
	lr r13, [_ARC_V2_AGU_OS5]
	st r13, [sp, ___callee_saved_stack_t_agu_os5_OFFSET]
	lr r13, [_ARC_V2_AGU_OS6]
	st r13, [sp, ___callee_saved_stack_t_agu_os6_OFFSET]
	lr r13, [_ARC_V2_AGU_OS7]
	st r13, [sp, ___callee_saved_stack_t_agu_os7_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD12]
	st r13, [sp, ___callee_saved_stack_t_agu_mod12_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD13]
	st r13, [sp, ___callee_saved_stack_t_agu_mod13_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD14]
	st r13, [sp, ___callee_saved_stack_t_agu_mod14_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD15]
	st r13, [sp, ___callee_saved_stack_t_agu_mod15_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD16]
	st r13, [sp, ___callee_saved_stack_t_agu_mod16_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD17]
	st r13, [sp, ___callee_saved_stack_t_agu_mod17_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD18]
	st r13, [sp, ___callee_saved_stack_t_agu_mod18_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD19]
	st r13, [sp, ___callee_saved_stack_t_agu_mod19_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD20]
	st r13, [sp, ___callee_saved_stack_t_agu_mod20_OFFSET]
	lr r13, [_ARC_V2_AGU_MOD21]
	_st32_huge_offset r13, sp, ___callee_saved_stack_t_agu_mod21_OFFSET, r1
	lr r13, [_ARC_V2_AGU_MOD22]
	_st32_huge_offset r13, sp, ___callee_saved_stack_t_agu_mod22_OFFSET, r1
	lr r13, [_ARC_V2_AGU_MOD23]
	_st32_huge_offset r13, sp, ___callee_saved_stack_t_agu_mod23_OFFSET, r1
#endif
#endif
agu_skip_save :
.endm

.macro _load_dsp_regs
#ifdef CONFIG_DSP_SHARING
	ld_s r13, [r2, ___thread_base_t_user_options_OFFSET]
	bbit0 r13, K_DSP_IDX, dsp_skip_load
	ld_s r13, [sp, ___callee_saved_stack_t_dsp_ctrl_OFFSET]
	sr r13, [_ARC_V2_DSP_CTRL]
	ld_s r13, [sp, ___callee_saved_stack_t_acc0_glo_OFFSET]
	sr r13, [_ARC_V2_ACC0_GLO]
	ld_s r13, [sp, ___callee_saved_stack_t_acc0_ghi_OFFSET]
	sr r13, [_ARC_V2_ACC0_GHI]
#ifdef CONFIG_ARC_DSP_BFLY_SHARING
	ld_s r13, [sp, ___callee_saved_stack_t_dsp_bfly0_OFFSET]
	sr r13, [_ARC_V2_DSP_BFLY0]
	ld_s r13, [sp, ___callee_saved_stack_t_dsp_fft_ctrl_OFFSET]
	sr r13, [_ARC_V2_DSP_FFT_CTRL]
#endif
#endif
dsp_skip_load :
#ifdef CONFIG_ARC_AGU_SHARING
	_load_agu_regs
#endif
.endm

.macro _load_agu_regs
#ifdef CONFIG_ARC_AGU_SHARING
	ld_s r13, [r2, ___thread_base_t_user_options_OFFSET]
	btst r13, K_AGU_IDX

	jeq agu_skip_load

	ld r13, [sp, ___callee_saved_stack_t_agu_ap0_OFFSET]
	sr r13, [_ARC_V2_AGU_AP0]
	ld r13, [sp, ___callee_saved_stack_t_agu_ap1_OFFSET]
	sr r13, [_ARC_V2_AGU_AP1]
	ld r13, [sp, ___callee_saved_stack_t_agu_ap2_OFFSET]
	sr r13, [_ARC_V2_AGU_AP2]
	ld r13, [sp, ___callee_saved_stack_t_agu_ap3_OFFSET]
	sr r13, [_ARC_V2_AGU_AP3]
	ld r13, [sp, ___callee_saved_stack_t_agu_os0_OFFSET]
	sr r13, [_ARC_V2_AGU_OS0]
	ld r13, [sp, ___callee_saved_stack_t_agu_os1_OFFSET]
	sr r13, [_ARC_V2_AGU_OS1]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod0_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD0]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod1_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD1]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod2_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD2]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod3_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD3]
#ifdef CONFIG_ARC_AGU_MEDIUM
	ld r13, [sp, ___callee_saved_stack_t_agu_ap4_OFFSET]
	sr r13, [_ARC_V2_AGU_AP4]
	ld r13, [sp, ___callee_saved_stack_t_agu_ap5_OFFSET]
	sr r13, [_ARC_V2_AGU_AP5]
	ld r13, [sp, ___callee_saved_stack_t_agu_ap6_OFFSET]
	sr r13, [_ARC_V2_AGU_AP6]
	ld r13, [sp, ___callee_saved_stack_t_agu_ap7_OFFSET]
	sr r13, [_ARC_V2_AGU_AP7]
	ld r13, [sp, ___callee_saved_stack_t_agu_os2_OFFSET]
	sr r13, [_ARC_V2_AGU_OS2]
	ld r13, [sp, ___callee_saved_stack_t_agu_os3_OFFSET]
	sr r13, [_ARC_V2_AGU_OS3]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod4_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD4]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod5_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD5]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod6_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD6]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod7_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD7]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod8_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD8]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod9_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD9]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod10_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD10]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod11_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD11]
#endif
#ifdef CONFIG_ARC_AGU_LARGE
	ld r13, [sp, ___callee_saved_stack_t_agu_ap8_OFFSET]
	sr r13, [_ARC_V2_AGU_AP8]
	ld r13, [sp, ___callee_saved_stack_t_agu_ap9_OFFSET]
	sr r13, [_ARC_V2_AGU_AP9]
	ld r13, [sp, ___callee_saved_stack_t_agu_ap10_OFFSET]
	sr r13, [_ARC_V2_AGU_AP10]
	ld r13, [sp, ___callee_saved_stack_t_agu_ap11_OFFSET]
	sr r13, [_ARC_V2_AGU_AP11]
	ld r13, [sp, ___callee_saved_stack_t_agu_os4_OFFSET]
	sr r13, [_ARC_V2_AGU_OS4]
	ld r13, [sp, ___callee_saved_stack_t_agu_os5_OFFSET]
	sr r13, [_ARC_V2_AGU_OS5]
	ld r13, [sp, ___callee_saved_stack_t_agu_os6_OFFSET]
	sr r13, [_ARC_V2_AGU_OS6]
	ld r13, [sp, ___callee_saved_stack_t_agu_os7_OFFSET]
	sr r13, [_ARC_V2_AGU_OS7]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod12_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD12]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod13_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD13]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod14_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD14]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod15_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD15]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod16_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD16]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod17_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD17]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod18_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD18]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod19_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD19]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod20_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD20]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod21_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD21]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod22_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD22]
	ld r13, [sp, ___callee_saved_stack_t_agu_mod23_OFFSET]
	sr r13, [_ARC_V2_AGU_MOD23]
#endif
#endif
agu_skip_load :
.endm

.macro _dsp_extension_probe
#ifdef CONFIG_ARC_DSP_TURNED_OFF
	mov	 r0, 0 /* DSP_CTRL_DISABLED_ALL */
	sr	 r0, [_ARC_V2_DSP_CTRL]
#endif
.endm
