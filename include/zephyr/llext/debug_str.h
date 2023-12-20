/*
 * Copyright (c) 2023 Arduino srl
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef ZEPHYR_LLEXT_DEBUG_STR_H
#define ZEPHYR_LLEXT_DEBUG_STR_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_LLEXT_DEBUG_STRINGS

/**
 * @brief Verbose debug string helpers
 * @defgroup llext_debug_str Debug string helpers
 * @ingroup llext
 * @{
 */

struct llext_loader;
struct llext;

/**
 * @brief Return a string representation of the ELF binding.
 * @param stb ELF st_bind value
 * @return String representation of the ELF st_bind value
 *
 */
const char *elf_st_bind_str(unsigned int stb);

/**
 * @brief Return a string representation of the ELF symbol type.
 * @param stt ELF st_type value
 * @return String representation of the ELF st_type value
 */
const char *elf_st_type_str(unsigned int stt);

/**
 * @brief Return a string representation of the ELF section index.
 * @param shndx ELF section index value
 * @return String representation of the ELF section index value
 */
const char *elf_sect_str(struct llext_loader *ldr, unsigned int shndx);

/**
 * @brief Return a string representation of the llext memory type.
 * @param sh_type llext memory type
 * @return String representation of the llext memory type
 */
const char *llext_mem_str(enum llext_mem mem);

/**
 * @brief Return a string representation of the physical address.
 *
 * This works by looking up the physical address in the following order:
 *
 * 1. If the address matches one in the built-in symbol table (symbols
 *    exported by Zephyr via EXPORT_SYMBOL), `builtin <symbol name>` is
 *    returned;
 * 2. If the address matches one in the symbol table of the loaded extension,
 *    `global <symbol name>` is returned;
 * 3. If the address is inside one of the extension's memory regions,
 *    `mem <memory region name>+<offset>` is returned.
 *
 * If none of the above apply, `addr <address> (?)` is returned.
 *
 * @param ldr llext loader
 * @param ext extension
 * @param addr physical address to be interpreted
 * @return String representation of the physical address
 */
const char *llext_addr_str(struct llext_loader *ldr, struct llext *ext, uintptr_t addr);

/**
 * @brief Return a string representation of the relocation type.
 *
 * This is architecture-specific and must be implemented by architectures
 * supporting the llext subsystem.
 *
 * @param r_type ELF relocation type
 * @return String representation of the relocation type
 */
const char *arch_r_type_str(unsigned int r_type);

#endif

#endif /* ZEPHYR_LLEXT_DEBUG_STR_H */
