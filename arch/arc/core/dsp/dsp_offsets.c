/*
 * Copyright (c) 2022 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 DSP and AGU structure member offset definition file
 *
 */
#ifdef CONFIG_DSP_SHARING
GEN_OFFSET_SYM(_callee_saved_stack_t, dsp_ctrl);
GEN_OFFSET_SYM(_callee_saved_stack_t, acc0_glo);
GEN_OFFSET_SYM(_callee_saved_stack_t, acc0_ghi);
#ifdef CONFIG_ARC_DSP_BFLY_SHARING
GEN_OFFSET_SYM(_callee_saved_stack_t, dsp_bfly0);
GEN_OFFSET_SYM(_callee_saved_stack_t, dsp_fft_ctrl);
#endif
#endif
#ifdef CONFIG_ARC_AGU_SHARING
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_ap0);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_ap1);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_ap2);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_ap3);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_os0);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_os1);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod0);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod1);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod2);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod3);
#ifdef CONFIG_ARC_AGU_MEDIUM
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_ap4);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_ap5);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_ap6);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_ap7);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_os2);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_os3);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod4);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod5);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod6);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod7);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod8);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod9);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod10);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod11);
#endif
#ifdef CONFIG_ARC_AGU_LARGE
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_ap8);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_ap9);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_ap10);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_ap11);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_os4);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_os5);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_os6);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_os7);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod12);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod13);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod14);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod15);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod16);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod17);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod18);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod19);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod20);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod21);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod22);
GEN_OFFSET_SYM(_callee_saved_stack_t, agu_mod23);
#endif
#endif
