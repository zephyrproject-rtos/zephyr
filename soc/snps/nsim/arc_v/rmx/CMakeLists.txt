# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL arcmwdt)
# CCAC:
  zephyr_compile_options_ifdef(CONFIG_SOC_SERIES_RMX
					-av5rmx -Zicsr -Zifencei -Zihintpause -Zba -Zbb
					-Zbs -Zca -Zcb -Zcmp -Zcmt -Za -Zm -Zicbom)

  zephyr_ld_options(-Hlib=rmx100)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
