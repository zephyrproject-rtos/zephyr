# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL arcmwdt)
# CCAC:
  zephyr_compile_options(-av5rhx -Zicsr -Zifencei -Zihintpause -Zba -Zbb
                         -Zbc -Zbs -Zca -Zcb -Zcmp -Zcmt -Za -Zm  -Zc
                         -Zicond -Zicbom -Zicbop)

  zephyr_ld_options(-av5rhx)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
