/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

   MAPPING_TABLE (NOLOAD) : {_sMAPPING_TABLE = .; *(MAPPING_TABLE); _eMAPPING_TABLE = .; } >SRAM1
   MB_MEM1 (NOLOAD) : { _sMB_MEM1 = .; *(MB_MEM1); _eMB_MEM1 = .; } >SRAM1
   MB_MEM2 (NOLOAD) : { _sMB_MEM2 = .; *(MB_MEM2); _eMB_MEM2 = .; } >SRAM1
