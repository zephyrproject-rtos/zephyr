# The fundamental linker flags always applied to every Zephyr build for XC-DSC
check_set_linker_property(
	TARGET linker PROPERTY base ${LINKERFLAGPREFIX},--check-sections
	# Check to not check section addresses for overlaps.
	${LINKERFLAGPREFIX},--data-init
	# initialize all data sections.Check to support initialized data.
	${LINKERFLAGPREFIX},--pack-data
	# pack initialized data into the smallest possible blocks
	${LINKERFLAGPREFIX},--handles
	# enable special support for dsPIC interrupt/exception handlers
	${LINKERFLAGPREFIX},--no-isr
	# disable default interrupt routine
	${LINKERFLAGPREFIX},--no-ivt
	# do not generate ivt or aivt
	# Check to create an interrupt function for unused vectors.
	${LINKERFLAGPREFIX},--no-gc-sections
	# preserve all sections (do not garbage-collect unused code/data)
	${LINKERFLAGPREFIX},--fill-upper=0
	# zero-fill any gaps above defined sections
	${LINKERFLAGPREFIX},--no-force-link
	# do not force-link all library objects
	${LINKERFLAGPREFIX},--smart-io
	# enable Smart-IO format‐string optimizations
	${LINKERFLAGPREFIX}, -L${XCDSC_TOOLCHAIN_PATH}/lib
	# Set picolib lib path from xc-dsc toolchain to link
	${LINKERFLAGPREFIX}, -lc-pico-elf
	# link picolib from xc-dsc toolchain

	)
