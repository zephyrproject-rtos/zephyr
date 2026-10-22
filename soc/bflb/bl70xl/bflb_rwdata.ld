/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Place RF library writable data sections in RAM.
 * Injected via zephyr_linker_sources(RWDATA).
 */

*(.retention .retention.*)
