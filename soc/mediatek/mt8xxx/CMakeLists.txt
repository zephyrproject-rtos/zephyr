# Copyright 2023 The ChromiumOS Authors
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(soc.c irq.c mbox.c)

zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_MT8195 cpuclk.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_SOC}/linker.ld CACHE INTERNAL "")

add_custom_target(dsp_img ALL
	DEPENDS ${ZEPHYR_FINAL_EXECUTABLE}
	COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen_img.py
		${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME}
		${CMAKE_BINARY_DIR}/zephyr/zephyr.img)

# Sign zephyr.ri using west (if the underlying rimage tool is
# available; generally it isn't except in SOF builds).  Note that the
# "target" string to use for rimage is set at the board level as a
# RIMAGE_TARGET cached (cached so that the python script can read it!)
# cmake variable (not kconfig!).  See board_set_rimage_target().
# Thankfully the SOC name and the rimage target names for these
# platforms are identical.  The west sign integration similarly needs
# a RIMAGE_CONFIG_PATH cmake cached (!) variable set to a directory
# containing the .toml files for the platforms, which SOF will set on
# its own at build time.  And likewise the signing key isn't provided
# by Zephyr and needs tob e found by a SOF build by passing it in
# RIMAGE_SIGN_KEY.
#
# The short version is that zephyr.ri can only realistically be
# exercised from a SOF build and it doesn't belong here in Zephyr;
# rimage is a SOF tool.  Signing audio firmware is only done for SOF
# firmware and not general Zephyr apps for the same hardware. This
# should live in SOF where it doesn't have to be duplicated for every
# device and where it won't be forced to communicate via side channels.
board_set_rimage_target(${CONFIG_SOC})
set(RIMAGE_SIGN_KEY "otc_private_key_3k.pem" CACHE STRING "default rimage key")
add_custom_target(zephyr.ri ALL DEPENDS ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri)
add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
	COMMENT "Sign with rimage..."
	COMMAND west $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose> sign
                     --if-tool-available --tool rimage --build-dir ${CMAKE_BINARY_DIR}
	DEPENDS ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
)
