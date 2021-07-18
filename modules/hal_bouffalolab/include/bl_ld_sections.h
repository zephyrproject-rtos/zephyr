/*
 * Copyright (c) 2021-2024 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BL_LD_SECTIONS_H
#define __BL_LD_SECTIONS_H

#define ATTR_STRINGIFY(x)        #x
#define ATTR_TOSTRING(x)         ATTR_STRINGIFY(x)
#define ATTR_UNI_SYMBOL          __FILE__ ATTR_TOSTRING(__LINE__)
#define ATTR_CLOCK_SECTION       __attribute__((section(".itcm.sclock_rlt_code." ATTR_UNI_SYMBOL)))
#define ATTR_CLOCK_CONST_SECTION __attribute__((section(".itcm.sclock_rlt_const." ATTR_UNI_SYMBOL)))
#define ATTR_TCM_SECTION         __attribute__((section(".itcm.code." ATTR_UNI_SYMBOL)))
#define ATTR_TCM_CONST_SECTION   __attribute__((section(".itcm.const." ATTR_UNI_SYMBOL)))
#define ATTR_DTCM_SECTION        __attribute__((section(".dtcm.data")))
#define ATTR_HSRAM_SECTION       __attribute__((section(".hsram_code")))
#define ATTR_DMA_RAM_SECTION     __attribute__((section(".system_ram")))
#define ATTR_HBN_RAM_SECTION     __attribute__((section(".hbn_ram_code")))
#define ATTR_HBN_RAM_CONST_SECTION __attribute__((section(".hbn_ram_data")))
#define ATTR_FALLTHROUGH()       __attribute__((fallthrough))
#define ATTR_USED                __attribute__((__used__))
#define ATTR_EALIGN(x)           __aligned(size)

#endif /* __BL_LD_SECTIONS_H */
