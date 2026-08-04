# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  precision_clock.c
  precision_deadline.c
  precision_mapping.c
  precision_pi.c
)
