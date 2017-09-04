zephyr_library()

zephyr_library_cc_option(-c -xassembler-with-cpp -mtext-section-literals -mlongcalls)
zephyr_library_sources_ifdef(CONFIG_XTENSA_RESET_VECTOR
	reset-vector.S
)
