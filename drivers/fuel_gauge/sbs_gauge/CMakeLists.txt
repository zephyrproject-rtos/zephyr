zephyr_library()

zephyr_library_sources(sbs_gauge.c)

zephyr_include_directories_ifdef(CONFIG_EMUL_SBS_GAUGE .)
zephyr_library_sources_ifdef(CONFIG_EMUL_SBS_GAUGE ../../sensor/sbs_gauge/emul_sbs_gauge.c)
