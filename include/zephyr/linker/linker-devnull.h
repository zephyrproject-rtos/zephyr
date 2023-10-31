/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DESCRIPTION
 * Platform independent set of macros for creating a memory segment for
 * aggregating data that shall be kept in the elf file but not in the binary.
 */

#ifndef ZEPHYR_INCLUDE_LINKER_LINKER_DEVNULL_H_

#if defined(CONFIG_LINKER_DEVNULL_MEMORY)

#if defined(CONFIG_XIP)
#if (!defined(ROM_ADDR) && !defined(ROM_BASE)) || !defined(ROM_SIZE)
#error "ROM_SIZE, ROM_ADDR or ROM_BASE not defined"
#endif
#endif /* CONFIG_XIP */

#if (!defined(RAM_ADDR) && !defined(RAM_BASE)) || !defined(RAM_SIZE)
#error "RAM_SIZE, RAM_ADDR or RAM_BASE not defined"
#endif

#if defined(CONFIG_XIP) && !defined(ROM_ADDR)
#define ROM_ADDR ROM_BASE
#endif

#if !defined(RAM_ADDR)
#define RAM_ADDR RAM_BASE
#endif

#define ROM_END_ADDR (ROM_ADDR + ROM_SIZE)
#define DEVNULL_SIZE CONFIG_LINKER_DEVNULL_MEMORY_SIZE
#define ROM_DEVNULL_END_ADDR (ROM_END_ADDR + DEVNULL_SIZE)
#define MAX_ADDR UINT32_MAX

/* Determine where to put the devnull region. It should be adjacent to the ROM
 * region. If ROM starts after RAM or the distance between ROM and RAM is big
 * enough to fit the devnull region then devnull region is placed just after
 * the ROM region. If it cannot be done then the devnull region is placed before
 * the ROM region. It is possible that the devnull region cannot be placed
 * adjacent to the ROM (e.g. ROM starts at 0 and RAM follows ROM). In that
 * case compilation fails and the devnull region is not supported in that
 * configuration.
 */
#if !defined(CONFIG_XIP)

#if RAM_ADDR >= DEVNULL_SIZE
#define DEVNULL_ADDR (RAM_ADDR - DEVNULL_SIZE)
#else
#define DEVNULL_ADDR (RAM_ADDR + RAM_SIZE)
#endif

#else /* CONFIG_XIP */

#if ((ROM_ADDR > RAM_ADDR) && ((MAX_ADDR - ROM_END_ADDR) >= DEVNULL_SIZE)) || \
	((ROM_END_ADDR + DEVNULL_SIZE) <= RAM_ADDR)
#define DEVNULL_ADDR ROM_END_ADDR
#elif ROM_ADDR > DEVNULL_SIZE
#define DEVNULL_ADDR (ROM_ADDR - DEVNULL_SIZE)
#else
#error "Cannot place devnull segment adjacent to ROM region."
#endif

#endif /* CONFIG_XIP */

#define DEVNULL_REGION DEVNULL_ROM

#endif /* CONFIG_LINKER_DEVNULL_MEMORY */

#endif /* ZEPHYR_INCLUDE_LINKER_LINKER_DEVNULL_H_ */
