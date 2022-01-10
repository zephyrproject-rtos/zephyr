/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef ELF_H
#define ELF_H

#include <stdint.h>

typedef uint32_t elf32_addr;
typedef uint16_t elf32_half;
typedef uint32_t elf32_off;
typedef int32_t elf32_sword;
typedef uint32_t elf32_word;

typedef uint64_t elf64_addr;
typedef uint16_t elf64_half;
typedef uint64_t elf64_off;
typedef int32_t elf64_sword;
typedef uint32_t elf64_word;
typedef int64_t elf64_sxword;
typedef uint64_t elf64_xword;

/*
 * ELF Header(32-bit)
 */
#define EI_NIDENT 16

struct elf32_ehdr {
	unsigned char e_ident[EI_NIDENT];
	elf32_half e_type;
	elf32_half e_machine;
	elf32_word e_version;
	elf32_addr e_entry;
	elf32_off e_phoff;
	elf32_off e_shoff;
	elf32_word e_flags;
	elf32_half e_ehsize;
	elf32_half e_phentsize;
	elf32_half e_phnum;
	elf32_half e_shentsize;
	elf32_half e_shnum;
	elf32_half e_shstrndx;
};

/*
 * ELF Header(64-bit)
 */
struct elf64_ehdr {
	unsigned char e_ident[EI_NIDENT];
	elf64_half e_type;
	elf64_half e_machine;
	elf64_word e_version;
	elf64_addr e_entry;
	elf64_off e_phoff;
	elf64_off e_shoff;
	elf64_word e_flags;
	elf64_half e_ehsize;
	elf64_half e_phentsize;
	elf64_half e_phnum;
	elf64_half e_shentsize;
	elf64_half e_shnum;
	elf64_half e_shstrndx;
};

/*
 * ELF file type
 */
#define ET_REL 0x1
#define ET_DYN 0x3

/*
 * Section Header(32-bit)
 */
struct elf32_shdr {
	elf32_word sh_name;
	elf32_word sh_type;
	elf32_word sh_flags;
	elf32_addr sh_addr;
	elf32_off sh_offset;
	elf32_word sh_size;
	elf32_word sh_link;
	elf32_word sh_info;
	elf32_word sh_addralign;
	elf32_word sh_entsize;
};

/*
 * Section Header(64-bit)
 */
struct elf64_shdr {
	elf64_word sh_name;
	elf64_word sh_type;
	elf64_xword sh_flags;
	elf64_addr sh_addr;
	elf64_off sh_offset;
	elf64_xword sh_size;
	elf64_word sh_link;
	elf64_word sh_info;
	elf64_xword sh_addralign;
	elf64_xword sh_entsize;
};

/*
 * Section type
 */
#define SHT_PROGBITS 0x1
#define SHT_SYMTAB 0x2
#define SHT_STRTAB 0x3
#define SHT_RELA 0x4
#define SHT_NOBITS 0x8
#define SHT_REL 0x9
#define SHT_DYNSYM 0xB

/*
 * Section flag
 */
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4

/*
 * Symbol table entry(32-bit)
 */
struct elf32_sym {
	elf32_word st_name;
	elf32_addr st_value;
	elf32_word st_size;
	unsigned char st_info;
	unsigned char st_other;
	elf32_half st_shndx;
};

/*
 * Symbol table entry(64-bit)
 */
struct elf64_sym {
	elf64_word st_name;
	elf64_addr st_value;
	elf64_xword st_size;
	unsigned char st_info;
	unsigned char st_other;
	elf64_half st_shndx;
};

#define SHN_UNDEF 0
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define STT_SECTION 0x3

#define STB_GLOBAL 0x1

#define STT_FUNC 0x2

#define ELF32_ST_BIND(i) ((i) >> 4)
#define ELF32_ST_TYPE(i) ((i) & 0xf)

#define ELF64_ST_BIND(i) ((i) >> 4)
#define ELF64_ST_TYPE(i) ((i) & 0xf)

/*
 * Relocation entry(32-bit)
 */
struct elf32_rel {
	elf32_addr r_offset;
	elf32_word r_info;
};

#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((i) & 0xff)

/*
 * Relocation type(i386)
 */
#define R_386_32 0x1
#define R_386_PC32 0x2
#define R_386_GLOB_DAT 0x6
#define R_386_JUMP_SLOT 0x7
#define R_386_RELATIVE 0x8

/*
 * Relocation entry(64-bit)
 */
struct elf64_rela {
	elf64_addr r_offset;
	elf64_xword r_info;
	elf64_sxword r_addend;
};

#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffff)

/*
 * Program header(32-bit)
 */
struct elf32_phdr {
	elf32_word p_type;
	elf32_off p_offset;
	elf32_addr p_vaddr;
	elf32_addr p_paddr;
	elf32_word p_filesz;
	elf32_word p_memsz;
	elf32_word p_flags;
	elf32_word p_align;
};

/*
 * Program header(64-bit)
 */
struct elf64_phdr {
	elf64_word p_type;
	elf64_off p_offset;
	elf64_addr p_vaddr;
	elf64_addr p_paddr;
	elf64_xword p_filesz;
	elf64_xword p_memsz;
	elf64_word p_flags;
	elf64_xword p_align;
};

/*
 * Program segment type
 */
#define PT_LOAD 1

/*
 * Dynamic section entry(32-bit)
 */
struct elf32_dyn {
	elf32_sword d_tag;
	union {
		elf32_word d_val;
		elf32_addr d_ptr;
	} d_un;
};

/*
 * Dynamic section entry(64-bit)
 */
struct elf64_dyn {
	elf64_sxword d_tag;
	union {
		elf64_xword d_val;
		elf64_addr d_ptr;
	} d_un;
};

#ifdef CONFIG_64BIT
	typedef struct elf64_ehdr elf_ehdr_t;
	typedef struct elf64_shdr elf_shdr_t;
	typedef struct elf64_phdr elf_phdr_t;
	typedef elf64_addr elf_addr;
	typedef elf64_half elf_half;
	typedef elf64_xword elf_word;
	typedef struct elf64_rela elf_rel_t;
	typedef struct elf64_sym elf_sym_t;
	#define ELF_R_SYM ELF64_R_SYM
	#define ELF_R_TYPE ELF64_R_TYPE
	#define ELF_ST_BIND ELF64_ST_BIND
	#define ELF_ST_TYPE ELF64_ST_TYPE
#else
	typedef struct elf32_ehdr elf_ehdr_t;
	typedef struct elf32_shdr elf_shdr_t;
	typedef struct elf32_phdr elf_phdr_t;
	typedef elf32_addr elf_addr;
	typedef elf32_half elf_half;
	typedef elf32_word elf_word;
	typedef struct elf32_rel elf_rel_t;
	typedef struct elf32_sym elf_sym_t;
	#define ELF_R_SYM ELF32_R_SYM
	#define ELF_R_TYPE ELF32_R_TYPE
	#define ELF_ST_BIND ELF32_ST_BIND
	#define ELF_ST_TYPE ELF32_ST_TYPE
#endif

#endif /* ELF_H */
