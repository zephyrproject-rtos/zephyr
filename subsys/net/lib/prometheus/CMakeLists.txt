# Copyright(c) 2024 Sparse Technology, Mustafa Abdullah Kus
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources(
  collector.c
  counter.c
  formatter.c
  gauge.c
  histogram.c
  summary.c
)

zephyr_linker_sources(DATA_SECTIONS prometheus.ld)
