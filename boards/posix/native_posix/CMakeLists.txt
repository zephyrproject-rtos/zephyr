zephyr_library()
zephyr_library_compile_definitions(_POSIX_CHEATS_H)
zephyr_library_sources(
	hw_models_top.c
	timer_model.c
	irq_handler.c
	irq_ctrl.c
	main.c
	tracing.c
	cmdline_common.c
	cmdline.c
	)

zephyr_ld_options(
  -lm
)
