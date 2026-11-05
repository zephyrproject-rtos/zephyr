/* Copyright The Zephyr Project Contributors */
/* SPDX-License-Identifier: Apache-2.0 */

#if !defined(LINKER_ZEPHYR_PREBUILT) && !defined(LINKER_ZEPHYR_FINAL)
#if defined(CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE)
/* Typed data references retain handlers during discovery. The instruction
 * slots are linked only at their final positions, where branch ranges apply.
 */
/DISCARD/ :
{
	*(.isr.*)
}
#else
.isr_pre0.entries 0 (INFO) :
{
	KEEP(*(.isr.*))
}
#endif
#endif
