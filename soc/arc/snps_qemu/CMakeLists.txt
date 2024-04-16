# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL gcc)
	zephyr_compile_options(-mcpu=${GCC_M_CPU})

	zephyr_compile_options_ifdef(CONFIG_ISA_ARCV2 -mno-sdata)

else()

	zephyr_compile_options_ifdef(CONFIG_SOC_QEMU_ARC_HS -arcv2hs -core2 -Xatomic
			       -Xunaligned -Xcode_density -Xswap -Xbitscan
			       -Xmpy_option=qmpyh -Xshift_assist -Xbarrel_shifter
			       -Xtimer0 -Xtimer1)

	zephyr_ld_option_ifdef(CONFIG_SOC_QEMU_ARC_HS -Hlib=hs38_full)

	if(NOT CONFIG_SOC_QEMU_ARC_HS)
		message(WARNING "QEMU ARC platforms other than HS are not supported yet with MW toolchain")
	endif()

endif()
