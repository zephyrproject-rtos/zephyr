/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Place RF library uninitialized data in RAM noinit section.
 * Injected via zephyr_linker_sources(NOINIT).
 */

*(.retention_noinit .retention_noinit.*)
