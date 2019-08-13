# SPDX-License-Identifier: Apache-2.0

zephyr_cc_option(-mlongcalls)
zephyr_sources(
	cpu_idle.c
	fatal.c
	window_vectors.S
	xtensa-asm2-util.S
	xtensa-asm2.c
	)

zephyr_sources_ifndef(CONFIG_ATOMIC_OPERATIONS_C atomic.S)
zephyr_sources_ifdef(CONFIG_XTENSA_USE_CORE_CRT1
	crt1.S
)
zephyr_sources_ifdef(CONFIG_IRQ_OFFLOAD
	irq_offload.c
)
add_subdirectory(startup)
