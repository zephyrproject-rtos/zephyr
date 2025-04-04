/*
 * Copyright 2025 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

.section .text

.global adsp_img_reset_start
.type adsp_img_reset_start, %object
.align 2
adsp_img_reset_start:
.incbin HIFI4_BIN_RESET

.global adsp_img_reset_end
.type adsp_img_reset_end, %object
adsp_img_reset_end:

.global adsp_img_text_start
.type adsp_img_text_start, %object
.align 2
adsp_img_text_start:
.incbin HIFI4_BIN_TEXT

.global adsp_img_text_end
.type adsp_img_text_end, %object
adsp_img_text_end:

.global adsp_img_data_start
.type adsp_img_data_end, %object
.align 2
adsp_img_data_start:
.incbin HIFI4_BIN_DATA

.global adsp_img_data_end
.type adsp_img_data_end, %object
adsp_img_data_end:

.global adsp_img_reset_size
.type adsp_img_reset_size, %object
.align 2
adsp_img_reset_size:
.int  adsp_img_reset_end - adsp_img_reset_start

.global adsp_img_text_size
.type adsp_img_text_size, %object
.align 2
adsp_img_text_size:
.int  adsp_img_text_end - adsp_img_text_start

.global adsp_img_data_size
.type adsp_img_data_size, %object
.align 2
adsp_img_data_size:
.int  adsp_img_data_end - adsp_img_data_start

.end
