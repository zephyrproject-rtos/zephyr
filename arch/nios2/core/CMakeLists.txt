zephyr_sources(
	thread.c
	cpu_idle.c
	fatal.c
	irq_manage.c
	swap.S
	prep_c.c
	reset.S
	cache.c
	exception.S
	crt0.S
	)

zephyr_sources_if_kconfig(irq_offload.c)
