zephyr_cc_option(-mlongcalls)
zephyr_sources(
	cpu_idle.c
	fatal.c
	window_vectors.S
	)

zephyr_sources_ifdef(CONFIG_XTENSA_ASM2
	xtensa-asm2-util.S
	xtensa-asm2.c
	)

zephyr_sources_ifndef(CONFIG_XTENSA_ASM2
		      xtensa_intr.c
		      irq_manage.c
		      swap.S
		      thread.c
		      xtensa_context.S
		      xtensa_intr_asm.S
		      xtensa_vectors.S
		      xt_zephyr.S
		     )

zephyr_sources_ifndef(CONFIG_ATOMIC_OPERATIONS_C atomic.S)
zephyr_sources_ifdef(CONFIG_XTENSA_USE_CORE_CRT1
	crt1.S
)
zephyr_sources_ifdef(CONFIG_IRQ_OFFLOAD
	irq_offload.c
)
add_subdirectory(startup)
