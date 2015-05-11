/* memmap_b.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "master.h"
#include "vxmicro.h"

#ifdef MEMMAP_BENCH


/*******************************************************************************
 *
 * memorymap_test - memory map get/free test
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void memorymap_test (void)
    {
    uint32_t et; /* elapsed time */
    int i;
    void* p;

    PRINT_STRING (dashline, output_file);
    et = BENCH_START ();
    for (i = 0; i < NR_OF_MAP_RUNS; i++) {
	task_mem_map_alloc_wait (MAP1, &p);
	task_mem_map_free (MAP1, &p);
	}
    et = TIME_STAMP_DELTA_GET (et);
    check_result ();

    PRINT_F (output_file, FORMAT, "average alloc and dealloc memory page",
	     SYS_CLOCK_HW_CYCLES_TO_NS_AVG (et, (2 * NR_OF_MAP_RUNS)));
    }

#endif /* MEMMAP_BENCH */
