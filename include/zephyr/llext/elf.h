/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef ZEPHYR_LLEXT_ELF_H
#define ZEPHYR_LLEXT_ELF_H

#include <stdint.h>

/**
 * @brief ELF types and parsing
 *
 * Reference documents can be found here https://refspecs.linuxfoundation.org/elf/
 *
 * @defgroup elf ELF data types and defines
 * @ingroup llext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Unsigned program address */
typedef uint32_t elf32_addr;
/** Unsigned medium integer */
typedef uint16_t elf32_half;
/** Unsigned file offset */
typedef uint32_t elf32_off;
/** Signed integer */
typedef int32_t elf32_sword;
/** Unsigned integer */
typedef uint32_t elf32_word;

/** Unsigned program address */
typedef uint64_t elf64_addr;
/** Unsigned medium integer */
typedef uint16_t elf64_half;
/** Unsigned file offset */
typedef uint64_t elf64_off;
/** Signed integer */
typedef int32_t elf64_sword;
/** Unsigned integer */
typedef uint32_t elf64_word;
/** Signed long integer */
typedef int64_t elf64_sxword;
/** Unsigned long integer */
typedef uint64_t elf64_xword;


/**
 * @brief ELF identifier block
 *
 * 4 byte magic (.ELF)
 * 1 byte class (Invalid, 32 bit, 64 bit)
 * 1 byte endianness (Invalid, LSB, MSB)
 * 1 byte version (1)
 * 1 byte OS ABI (0 None, 1 HP-UX, 2 NetBSD, 3 Linux)
 * 1 byte ABI (0)
 * 7 bytes padding
 */
#define EI_NIDENT 16

/**
 * @brief ELF Header(32-bit)
 */
struct elf32_ehdr {
	/** Magic string identifying ELF binary */
	unsigned char e_ident[EI_NIDENT];
	/** Type of ELF */
	elf32_half e_type;
	/** Machine type */
	elf32_half e_machine;
	/** Object file version */
	elf32_word e_version;
	/** Virtual address of entry */
	elf32_addr e_entry;
	/** Program header table offset */
	elf32_off e_phoff;
	/** Section header table offset */
	elf32_off e_shoff;
	/** Processor specific flags */
	elf32_word e_flags;
	/** ELF header size */
	elf32_half e_ehsize;
	/** Program header count */
	elf32_half e_phentsize;
	/** Program header count */
	elf32_half e_phnum;
	/** Section header size */
	elf32_half e_shentsize;
	/** Section header count */
	elf32_half e_shnum;
	/** Section header containing section header string table */
	elf32_half e_shstrndx;
};

/**
 * @brief ELF Header(64-bit)
 */
struct elf64_ehdr {
	/** Magic string identifying ELF binary */
	unsigned char e_ident[EI_NIDENT];
	/** Type of ELF */
	elf64_half e_type;
	/** Machine type */
	elf64_half e_machine;
	/** Object file version */
	elf64_word e_version;
	/** Virtual address of entry */
	elf64_addr e_entry;
	/** Program header table offset */
	elf64_off e_phoff;
	/** Section header table offset */
	elf64_off e_shoff;
	/** Processor specific flags */
	elf64_word e_flags;
	/** ELF header size */
	elf64_half e_ehsize;
	/** Program header size */
	elf64_half e_phentsize;
	/** Program header count */
	elf64_half e_phnum;
	/** Section header size */
	elf64_half e_shentsize;
	/** Section header count */
	elf64_half e_shnum;
	/** Section header containing section header string table */
	elf64_half e_shstrndx;
};

/** Relocatable (unlinked) ELF */
#define ET_REL  1

/** Executable (without PIC/PIE) ELF */
#define ET_EXEC 2

/** Dynamic (executable with PIC/PIE or shared lib) ELF */
#define ET_DYN  3

/** Core Dump */
#define ET_CORE 4

/**
 * @brief Section Header(32-bit)
 */
struct elf32_shdr {
	/** Section header name index in section header string table */
	elf32_word sh_name;
	/** Section type */
	elf32_word sh_type;
	/** Section header attributes */
	elf32_word sh_flags;
	/** Address of section in the image */
	elf32_addr sh_addr;
	/** Location of section in the ELF binary in bytes */
	elf32_off sh_offset;
	/** Section size in bytes */
	elf32_word sh_size;
	/** Section header table link index, depends on section type */
	elf32_word sh_link;
	/** Section info, depends on section type */
	elf32_word sh_info;
	/** Section address alignment */
	elf32_word sh_addralign;
	/** Section contains table of fixed size entries sh_entsize bytes large */
	elf32_word sh_entsize;
};

/**
 * @brief Section Header(64-bit)
 */
struct elf64_shdr {
	/** Section header name index in section header string table */
	elf64_word sh_name;
	/** Section type */
	elf64_word sh_type;
	/** Section header attributes */
	elf64_xword sh_flags;
	/** Address of section in the image */
	elf64_addr sh_addr;
	/** Location of section in the ELF binary in bytes */
	elf64_off sh_offset;
	/** Section size in bytes */
	elf64_xword sh_size;
	/** Section header table link index, depends on section type */
	elf64_word sh_link;
	/** Section info, depends on section type */
	elf64_word sh_info;
	/** Section address alignment */
	elf64_xword sh_addralign;
	/** Section contains table of fixed size entries sh_entsize bytes large */
	elf64_xword sh_entsize;
};

#define SHT_PROGBITS 0x1
#define SHT_SYMTAB 0x2
#define SHT_STRTAB 0x3
#define SHT_RELA 0x4
#define SHT_NOBITS 0x8
#define SHT_REL 0x9
#define SHT_DYNSYM 0xB

#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4

/**
 * @brief Symbol table entry(32-bit)
 */
struct elf32_sym {
	/** Name of the symbol as an index into the symbol string table */
	elf32_word st_name;
	/** Value or location of the symbol */
	elf32_addr st_value;
	/** Size of the symbol */
	elf32_word st_size;
	/** Symbol binding and type information */
	unsigned char st_info;
	/** Symbol visibility */
	unsigned char st_other;
	/** Symbols related section given by section header index */
	elf32_half st_shndx;
};

/**
 * @brief Symbol table entry(64-bit)
 */
struct elf64_sym {
	/** Name of the symbol as an index into the symbol string table */
	elf64_word st_name;
	/** Symbol binding and type information */
	unsigned char st_info;
	/** Symbol visibility */
	unsigned char st_other;
	/** Symbols related section given by section header index */
	elf64_half st_shndx;
	/** Value or location of the symbol */
	elf64_addr st_value;
	/** Size of the symbol */
	elf64_xword st_size;
};

#define SHN_UNDEF 0
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2

#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_COMMON 5
#define STT_LOOS 10
#define STT_HIOS 12
#define STT_LOPROC 13
#define STT_HIPROC 15

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOOS 10
#define STB_HIOS 12
#define STB_LOPROC 13
#define STB_HIPROC 15

/**
 * @brief Symbol binding from 32bit st_info
 *
 * @param i Value of st_info
 */
#define ELF32_ST_BIND(i) ((i) >> 4)

/**
 * @brief Symbol type from 32bit st_info
 *
 * @param i Value of st_info
 */
#define ELF32_ST_TYPE(i) ((i) & 0xf)

/**
 * @brief Symbol binding from 32bit st_info
 *
 * @param i Value of st_info
 */
#define ELF64_ST_BIND(i) ((i) >> 4)


/**
 * @brief Symbol type from 32bit st_info
 *
 * @param i Value of st_info
 */
#define ELF64_ST_TYPE(i) ((i) & 0xf)

/**
 * @brief Relocation entry for 32-bit ELFs
 */
struct elf32_rel {
	/** Offset in the section to perform a relocation */
	elf32_addr r_offset;

	/** Information about the relocation, related symbol and type */
	elf32_word r_info;
};

struct elf32_rela {
	elf32_addr r_offset;
	elf32_word r_info;
	elf32_word r_addend;
};

/**
 * @brief Relocation symbol index from r_info
 *
 * @param i Value of r_info
 */
#define ELF32_R_SYM(i) ((i) >> 8)

/**
 * @brief Relocation type from r_info
 *
 * @param i Value of r_info
 */
#define ELF32_R_TYPE(i) ((i) & 0xff)

/**
 * @brief Relocation entry for 64-bit ELFs
 */
struct elf64_rel {
	/** Offset in section to perform a relocation */
	elf64_addr r_offset;
	/** Information about relocation, related symbol and type */
	elf64_xword r_info;
};

struct elf64_rela {
	elf64_addr r_offset;
	elf64_xword r_info;
	elf64_sxword r_addend;
};

/** @brief Relocation symbol from r_info
 *
 * @param i Value of r_info
 */
#define ELF64_R_SYM(i) ((i) >> 32)

/**
 * @brief Relocation type from r_info
 *
 * @param i Value of r_info
 */
#define ELF64_R_TYPE(i) ((i) & 0xffffffff)

#define R_386_NONE 0
#define R_386_32 1
#define R_386_PC32 2
#define R_386_GOT32 3
#define R_386_PLT32 4
#define R_386_COPY 5
#define R_386_GLOB_DAT 6
#define R_386_JMP_SLOT 7
#define R_386_RELATIVE 8
#define R_386_GOTOFF 9

#define R_ARM_NONE 0
#define R_ARM_PC24 1
#define R_ARM_ABS32 2
#define R_ARM_REL32 3
#define R_ARM_COPY 4
#define R_ARM_CALL 28
#define R_ARM_V4BX 40

#define R_XTENSA_NONE 0
#define R_XTENSA_32 1
#define R_XTENSA_SLOT0_OP 20

/**
 * @brief Program header(32-bit)
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

/**
 * @brief Program header(64-bit)
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

/**
 * @brief Program segment type
 */
#define PT_LOAD 1

/**
 * @brief Dynamic section entry(32-bit)
 */
struct elf32_dyn {
	elf32_sword d_tag;
	union {
		elf32_word d_val;
		elf32_addr d_ptr;
	} d_un;
};

/**
 * @brief Dynamic section entry(64-bit)
 */
struct elf64_dyn {
	elf64_sxword d_tag;
	union {
		elf64_xword d_val;
		elf64_addr d_ptr;
	} d_un;
};

#if defined(CONFIG_64BIT) || defined(__DOXYGEN__)
/** Machine sized elf header structure */
typedef struct elf64_ehdr elf_ehdr_t;
/** Machine sized section header structure */
typedef struct elf64_shdr elf_shdr_t;
/** Machine sized program header structure */
typedef struct elf64_phdr elf_phdr_t;
/** Machine sized program address */
typedef elf64_addr elf_addr;
/** Machine sized small integer */
typedef elf64_half elf_half;
/** Machine sized integer */
typedef elf64_xword elf_word;
/** Machine sized relocation struct */
typedef struct elf64_rel elf_rel_t;
typedef struct elf64_rela elf_rela_t;
/** Machine sized symbol struct */
typedef struct elf64_sym elf_sym_t;
/** Machine sized macro alias for obtaining a relocation symbol */
#define ELF_R_SYM ELF64_R_SYM
/** Machine sized macro alias for obtaining a relocation type */
#define ELF_R_TYPE ELF64_R_TYPE
/** Machine sized macro alias for obtaining a symbol bind */
#define ELF_ST_BIND ELF64_ST_BIND
/** Machine sized macro alias for obtaining a symbol type */
#define ELF_ST_TYPE ELF64_ST_TYPE
#else
/** Machine sized elf header structure */
typedef struct elf32_ehdr elf_ehdr_t;
/** Machine sized section header structure */
typedef struct elf32_shdr elf_shdr_t;
/** Machine sized program header structure */
typedef struct elf32_phdr elf_phdr_t;
/** Machine sized program address */
typedef elf32_addr elf_addr;
/** Machine sized small integer */
typedef elf32_half elf_half;
/** Machine sized integer */
typedef elf32_word elf_word;
/** Machine sized relocation struct */
typedef struct elf32_rel elf_rel_t;
typedef struct elf32_rela elf_rela_t;
/** Machine sized symbol struct */
typedef struct elf32_sym elf_sym_t;
/** Machine sized macro alias for obtaining a relocation symbol */
#define ELF_R_SYM ELF32_R_SYM
/** Machine sized macro alias for obtaining a relocation type */
#define ELF_R_TYPE ELF32_R_TYPE
/** Machine sized macro alias for obtaining a symbol bind */
#define ELF_ST_BIND ELF32_ST_BIND
/** Machine sized macro alias for obtaining a symbol type */
#define ELF_ST_TYPE ELF32_ST_TYPE
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_LLEXT_ELF_H */
