/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Boot stub is placed at the start of rom_start (before the vector table).
 * On reset the CPU executes this code from TCM; z_arm_relocate_vector_table()
 * later copies the vector table to address 0.
 */
KEEP(*(.text.boot))
