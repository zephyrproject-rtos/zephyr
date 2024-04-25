# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL gcc)
# GCC:

else ()
# CCAC:
  zephyr_compile_options_ifdef(CONFIG_SOC_SERIES_RMX5XX
					-av5rmx -Zicsr -Zifencei -Zihintpause -Zba -Zbb
					-Zbs -Zca -Zcb -Zcmp -Zcmt -Za -Zm -Zicbom)

  zephyr_compile_options(-D__MW_ASM_RV_MACRO__)

  zephyr_ld_options(-Hlib=rmx500)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
