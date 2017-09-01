zephyr_include_directories(.)

zephyr_compile_options_ifndef(
  CONFIG_RISCV_GENERIC_TOOLCHAIN
  -march=IMXpulpv2
  )

zephyr_sources(
  soc_irq.S
  vector.S
  pulpino_irq.c
  pulpino_idle.c
  )
