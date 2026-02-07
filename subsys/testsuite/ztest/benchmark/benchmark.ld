/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_RAM(ztest_benchmark, 4)
ITERABLE_SECTION_RAM(ztest_benchmark_timed, 4)
ITERABLE_SECTION_RAM(ztest_benchmark_suite, 4)
