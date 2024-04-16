zephyr_sources(
	soc.c
)
zephyr_library_sources_ifndef(CONFIG_RISCV_ISA_EXT_M __arithmetic.S)
zephyr_sources_ifdef(CONFIG_SOC_IT8XXX2_USE_ILM ilm.c)
