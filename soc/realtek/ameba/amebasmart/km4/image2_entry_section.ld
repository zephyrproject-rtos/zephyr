/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * IMG1 ignores the reset vector and enters via the Img2EntryFun0 struct, which
 * must be the first bytes of image2 (ORIGIN(RAM), 0x60000020).
 */

	__image2_entry_func__ = .;
	KEEP(*(SORT(.image2.entry.data*)))
	__image2_entry_func_end__ = .;
