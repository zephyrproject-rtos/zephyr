# SPDX-License-Identifier: Apache-2.0

#zephyr_sources(clock.c)
#zephyr_include_directories(.)

if(COMPILER STREQUAL gcc)

elseif(COMPILER STREQUAL clang)
  zephyr_sources(div.S)

  zephyr_compile_options_ifdef(CONFIG_SOC_SERIES_NSIM_RV_RMX500 -march=rv32gc -mno-relax)

else ()
# for CCAC:
  zephyr_compile_options_ifdef(CONFIG_SOC_SERIES_NSIM_RV_RMX500
					-av5rmx -Zicsr -Zifencei -Zihintpause -Zba -Zbb
					-Zbs -Zca -Zcb -Zcmp -Zcmt -Za -Zm -Zicbom)

#  zephyr_compile_options_ifdef(CONFIG_SOC_SERIES_NSIM_RV_RMX500 -Hoff=Stackcheck_alloca)

  zephyr_compile_options(-D__MW_ASM_RV_MACRO__)				

  zephyr_ld_options(-Hlib=rmx500)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
