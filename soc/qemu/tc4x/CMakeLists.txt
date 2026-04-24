# SPDX-License-Identifier: Apache-2.0

set(_real_soc ${ZEPHYR_BASE}/soc/infineon/aurix/tc4x)

zephyr_include_directories(${_real_soc})
zephyr_sources(${_real_soc}/soc.c ${_real_soc}/cpu_idle.c)

set(SOC_LINKER_SCRIPT
    ${ZEPHYR_BASE}/include/zephyr/arch/tricore/linker.ld
    CACHE INTERNAL "")
