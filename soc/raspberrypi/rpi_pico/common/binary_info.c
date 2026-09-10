/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "binary_info.h"

extern uint32_t __rom_region_end;
bi_decl(bi_binary_end((intptr_t)&__rom_region_end));

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_NAME
bi_decl(bi_program_name((uint32_t)BI_PROGRAM_NAME));
#endif

#if defined(CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_VERSION_STRING)
bi_decl(bi_program_version_string((uint32_t)BI_PROGRAM_VERSION_STRING));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_DESCRIPTION
bi_decl(bi_program_description((uint32_t)BI_PROGRAM_DESCRIPTION));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_URL
bi_decl(bi_program_url((uint32_t)BI_PROGRAM_URL));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PROGRAM_BUILD_DATE
bi_decl(bi_program_build_date_string((uint32_t)BI_PROGRAM_BUILD_DATE));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_PICO_BOARD
bi_decl(bi_string(BINARY_INFO_TAG_RASPBERRY_PI, BINARY_INFO_ID_RP_PICO_BOARD,
		  (uint32_t)BI_PICO_BOARD));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_SDK_VERSION_STRING
bi_decl(bi_string(BINARY_INFO_TAG_RASPBERRY_PI, BINARY_INFO_ID_RP_SDK_VERSION,
		  (uint32_t)BI_SDK_VERSION_STRING));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_BOOT_STAGE2_NAME
bi_decl(bi_string(BINARY_INFO_TAG_RASPBERRY_PI, BINARY_INFO_ID_RP_BOOT2_NAME,
		  (uint32_t)BI_BOOT_STAGE2_NAME));
#endif

#ifdef CONFIG_RPI_PICO_BINARY_INFO_ATTRIBUTE_BUILD_TYPE
#ifdef CONFIG_DEBUG
bi_decl(bi_program_build_attribute((uint32_t)"Debug"));
#else
bi_decl(bi_program_build_attribute((uint32_t)"Release"));
#endif
#endif
