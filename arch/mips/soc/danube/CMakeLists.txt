zephyr_sources(danube_idle.c danube_irq.c)

zephyr_compile_options(
  -mips32r2
  ${TOOLCHAIN_C_FLAGS}
)

zephyr_ld_options(
  -mips32r2
  ${TOOLCHAIN_LD_FLAGS}
)
