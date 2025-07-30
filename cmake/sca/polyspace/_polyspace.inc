/**
 * @brief Header file to incorporate modifications for Polyspace analysis
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2025, The MathWorks, Inc.
 */
#ifndef _POLYSPACE_INC
#define _POLYSPACE_INC

#ifdef _POLYSPACE_APP_ONLY
// replace ignored macros with stubs
void Z_LOG(int,const char*,...);
void Z_LOG_INSTANCE(int, const void*,...);
void Z_LOG_HEXDUMP(int,const char*,...);
void Z_LOG_HEXDUMP_INSTANCE(int, const void*,...);
#endif


#endif /* _POLYSPACE_INC */
