/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Layout of struct pie_regs, see main.c:
 *   0: Q0 to Q7, 16 bytes each
 * 128: QACC_H_0 to QACC_H_4
 * 148: QACC_L_0 to QACC_L_4
 * 168: ACCX_0
 * 172: UA_STATE_0 to UA_STATE_3
 */

/*
 * pie_get - Store the PIE registers to a 16-byte aligned struct pie_regs
 */
.global pie_get
.align 4
pie_get:
	entry a1, 32

	st.qr q0, a2, 0
	st.qr q1, a2, 16
	st.qr q2, a2, 32
	st.qr q3, a2, 48
	st.qr q4, a2, 64
	st.qr q5, a2, 80
	st.qr q6, a2, 96
	st.qr q7, a2, 112

	rur.qacc_h_0 a3
	s32i a3, a2, 128
	rur.qacc_h_1 a3
	s32i a3, a2, 132
	rur.qacc_h_2 a3
	s32i a3, a2, 136
	rur.qacc_h_3 a3
	s32i a3, a2, 140
	rur.qacc_h_4 a3
	s32i a3, a2, 144

	rur.qacc_l_0 a3
	s32i a3, a2, 148
	rur.qacc_l_1 a3
	s32i a3, a2, 152
	rur.qacc_l_2 a3
	s32i a3, a2, 156
	rur.qacc_l_3 a3
	s32i a3, a2, 160
	rur.qacc_l_4 a3
	s32i a3, a2, 164

	rur.accx_0 a3
	s32i a3, a2, 168

	rur.ua_state_0 a3
	s32i a3, a2, 172
	rur.ua_state_1 a3
	s32i a3, a2, 176
	rur.ua_state_2 a3
	s32i a3, a2, 180
	rur.ua_state_3 a3
	s32i a3, a2, 184

	retw

/*
 * pie_set - Load the PIE registers from a 16-byte aligned struct pie_regs
 */
.global pie_set
.align 4
pie_set:
	entry a1, 32

	ld.qr q0, a2, 0
	ld.qr q1, a2, 16
	ld.qr q2, a2, 32
	ld.qr q3, a2, 48
	ld.qr q4, a2, 64
	ld.qr q5, a2, 80
	ld.qr q6, a2, 96
	ld.qr q7, a2, 112

	l32i a3, a2, 128
	wur.qacc_h_0 a3
	l32i a3, a2, 132
	wur.qacc_h_1 a3
	l32i a3, a2, 136
	wur.qacc_h_2 a3
	l32i a3, a2, 140
	wur.qacc_h_3 a3
	l32i a3, a2, 144
	wur.qacc_h_4 a3

	l32i a3, a2, 148
	wur.qacc_l_0 a3
	l32i a3, a2, 152
	wur.qacc_l_1 a3
	l32i a3, a2, 156
	wur.qacc_l_2 a3
	l32i a3, a2, 160
	wur.qacc_l_3 a3
	l32i a3, a2, 164
	wur.qacc_l_4 a3

	l32i a3, a2, 168
	wur.accx_0 a3

	l32i a3, a2, 172
	wur.ua_state_0 a3
	l32i a3, a2, 176
	wur.ua_state_1 a3
	l32i a3, a2, 180
	wur.ua_state_2 a3
	l32i a3, a2, 184
	wur.ua_state_3 a3

	retw
