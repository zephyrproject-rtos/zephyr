/*
 * Copyright 2025 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

.section .text

.global dsp_img_reset_start
.type dsp_img_reset_start, %object
.align 2
dsp_img_reset_start:
.incbin HIFI4_BIN_RESET

.global dsp_img_reset_end
.type dsp_img_reset_end, %object
dsp_img_reset_end:

.global dsp_img_text_start
.type dsp_img_text_start, %object
.align 2
dsp_img_text_start:
.incbin HIFI4_BIN_TEXT

.global dsp_img_text_end
.type dsp_img_text_end, %object
dsp_img_text_end:

.global dsp_img_data_start
.type dsp_img_data_end, %object
.align 2
dsp_img_data_start:
.incbin HIFI4_BIN_DATA

.global dsp_img_data_end
.type dsp_img_data_end, %object
dsp_img_data_end:

.global dsp_img_reset_size
.type dsp_img_reset_size, %object
.align 2
dsp_img_reset_size:
.int  dsp_img_reset_end - dsp_img_reset_start

.global dsp_img_text_size
.type dsp_img_text_size, %object
.align 2
dsp_img_text_size:
.int  dsp_img_text_end - dsp_img_text_start

.global dsp_img_data_size
.type dsp_img_data_size, %object
.align 2
dsp_img_data_size:
.int  dsp_img_data_end - dsp_img_data_start

.end
