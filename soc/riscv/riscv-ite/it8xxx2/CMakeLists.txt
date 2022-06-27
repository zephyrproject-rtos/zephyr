zephyr_sources(
	soc.c
)

# IMPORTANT:
# The h2ram section must be first added to RAM_SECTIONS to avoid gap.
zephyr_linker_sources_ifdef(CONFIG_ESPI_IT8XXX2
  RAM_SECTIONS
  h2ram.ld
)
