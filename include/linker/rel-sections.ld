	/*
	 * .rel.* are for relocation.
	 * These are being produced by compiler/linker.
	 * Specify these here so they are not considered orphan sections.
	 *
	 * Obtained via
	 * '$(ARCH)-zephyr-elf-ld --verbose'
	 */

#if !defined(CONFIG_RISCV32) && !defined(CONFIG_XTENSA)
	SECTION_PROLOGUE(.rel.dyn,,)
	{
	*(.rel.init)
	*(.rel.text .rel.text.* .rel.text_start.* .rel.gnu.linkonce.t.*)
	*(.rel.fini)
	*(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*)
	*(.rel.data.rel.ro .rel.data.rel.ro.* .rel.gnu.linkonce.d.rel.ro.*)
	*(.rel.data .rel.data.* .rel.gnu.linkonce.d.*)
	*(.rel.tdata .rel.tdata.* .rel.gnu.linkonce.td.*)
	*(.rel.tbss .rel.tbss.* .rel.gnu.linkonce.tb.*)

#if defined(CONFIG_ARCH_POSIX)
	*(.rel.preinit_array)
	*(.rel.init_array)
	*(.rel.fini_array)
#endif

	*(.rel.ctors)
	*(.rel.dtors)
	*(.rel.got)
	*(.rel.bss .rel.bss.* .rel.gnu.linkonce.b.*)

#if defined(CONFIG_ARM) || defined(CONFIG_NIOS2)
	PROVIDE_HIDDEN (__rel_iplt_start = .);
	*(.rel.iplt)
	PROVIDE_HIDDEN (__rel_iplt_end = .);
#endif

	/* These are related to sections defined by Zephyr */
	*(.rel.kernel .rel.kernel.*)
	*(.rel.init_PRE_KERNEL*)
	*(.rel.init_POST_KERNEL*)
	*(.rel.init_APPLICATION*)

#if defined(CONFIG_ARCH_POSIX)
	*(.rel.native_PRE_BOOT*)
	*(.rel.native_FIRST_SLEEP*)
	*(.rel.native_ON_EXIT*)
#endif

	*(.rel.devconfig.*)
	*(.rel._k_alert.*)
	*(.rel._k_mem_pool.*)
	*(.rel._k_timer.*)
	*(.rel._static_thread_data.*)

#if defined(CONFIG_X86)
	*(.rel.ifunc)
	*(.rel.mmulist)
	*(.rel.tss)
#endif
	}
#endif /* !defined(CONFIG_RISCV32) */

#if defined(CONFIG_ARM) || defined(CONFIG_NIOS2) || defined(CONFIG_RISCV32)
	SECTION_PROLOGUE(.rela.dyn,,)
	{
	*(.rela.init)
	*(.rela.text .rela.text.* .rela.gnu.linkonce.t.*)
	*(.rela.fini)
	*(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*)
	*(.rela.data .rela.data.* .rela.gnu.linkonce.d.*)
	*(.rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*)
	*(.rela.tbss .rela.tbss.* .rela.gnu.linkonce.tb.*)
	*(.rela.ctors)
	*(.rela.dtors)
	*(.rela.got)
	*(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)

#if defined(CONFIG_ARM) || defined(CONFIG_NIOS2)
	PROVIDE_HIDDEN (__rela_iplt_start = .);
	*(.rela.iplt)
	PROVIDE_HIDDEN (__rela_iplt_end = .);
#endif

	/* These are related to sections defined by Zephyr */
	*(.rela.kernel .rel.kernel.*)
	*(.rela.init_PRE_KERNEL*)
	*(.rela.init_POST_KERNEL*)
	*(.rela.init_APPLICATION*)
	*(.rela._k_alert.*)
	*(.rela._k_mem_pool.*)
	*(.rela._k_timer.*)
	*(.rela._static_thread_data.*)

#if defined(CONFIG_RISCV32)
	*(.rela.devconfig.*)
	*(.rela.exception.*)
	*(.rela.gnu.linkonce.sw_isr_table)
	*(.rela.sdata.*)
#endif

	}
#endif

#if !defined(CONFIG_RISCV32) && !defined(CONFIG_XTENSA)
	SECTION_PROLOGUE(.rel.plt,,)
	{
	*(.rel.plt)

#if defined(CONFIG_X86)
	PROVIDE_HIDDEN (__rel_iplt_start = .);
	*(.rel.iplt)
	PROVIDE_HIDDEN (__rel_iplt_end = .);
#endif
	}
#endif /* !defined(CONFIG_RISCV32) */

#if defined(CONFIG_ARM) || defined(CONFIG_NIOS2) || defined(CONFIG_RISCV32)
	SECTION_PROLOGUE(.rela.plt,,)
	{
	*(.rela.plt)
	}
#endif
