#
# SPDX-FileCopyrightText: Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_include_directories(.)

# Boot data the ROM reads before the image runs: the flash configuration block
# for the NOR and the external memory configuration data that brings up the
# XSPI1 PSRAM.
if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  zephyr_library_sources(xip/mimxrt2660_evk_xspi_nor_config.c)
endif()

# Keep the heap out of the PSRAM: both the libc malloc arena and the kernel
# k_malloc()/k_heap pool go to SRAM2, which is the placement the SDK's
# xspi_nor_psram linker configuration uses.
#
# Both buffers are reached through the NOINIT section kind, not BSS:
# malloc_arena[] is POOL_SECTION == __noinit in malloc.c, and the buffer behind
# _system_heap is __noinit_named from K_HEAP_DEFINE().  malloc.c additionally
# gets SRAM2_BSS so the sys_heap control block and its mutex sit next to the
# arena rather than staying behind in the PSRAM.
#
# The libc part only fires when COMMON_LIBC_MALLOC_ARENA_SIZE > 0.  With the
# -1 default there is no arena object to place at all: the heap is computed at
# runtime as "everything after _end inside zephyr,sram".
#
# It also does nothing under CONFIG_USERSPACE, and cannot: there malloc.c puts
# the arena in the z_malloc_partition app-memory section instead of .noinit, and
# the MPU programs [z_data_smem_z_malloc_partition_part_start, part_end) as the
# region user threads may touch.  Moving the arena out of that section would
# break the isolation, so a userspace build keeps its heap in the PSRAM -- at
# the size pinned in the defconfig rather than the -1 default.
#
# The condition tests DEFINED plus a string comparison because Kconfig booleans
# reach CMake as the string "y" rather than a CMake boolean, so a plain
# if(CONFIG_FOO) would silently never fire.
if(DEFINED CONFIG_COMMON_LIBC_MALLOC AND
   NOT DEFINED CONFIG_USERSPACE AND
   DEFINED CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE AND
   "${CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE}" GREATER 0)
  zephyr_code_relocate(
    FILES ${ZEPHYR_BASE}/lib/libc/common/source/stdlib/malloc.c
    LOCATION SRAM2_NOINIT
  )
  zephyr_code_relocate(
    FILES ${ZEPHYR_BASE}/lib/libc/common/source/stdlib/malloc.c
    LOCATION SRAM2_BSS
  )
endif()

# The k_malloc()/k_heap pool.  Unconditional: with the default
# HEAP_MEM_POOL_SIZE=0 the K_HEAP_DEFINE() is compiled out and mempool.c has no
# NOINIT section, which gen_relocate_app.py reports as an empty set rather than
# an error.
zephyr_code_relocate(
  FILES ${ZEPHYR_BASE}/kernel/mempool.c
  LOCATION SRAM2_NOINIT
)
