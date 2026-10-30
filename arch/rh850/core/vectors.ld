/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* RBASE and EBASE need 512-byte alignment */
. = ALIGN(512);
KEEP(*(.vectors))
