/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ELF_LOADER_H_
#define ELF_LOADER_H_

#include <openamp/remoteproc.h>
#include <openamp/remoteproc_loader.h>

#if defined __cplusplus
extern "C" {
#endif

/* ELF32 base types - 32-bit. */
typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

/* ELF64 base types - 64-bit. */
typedef uint64_t Elf64_Addr;
typedef uint16_t Elf64_Half;
typedef uint64_t Elf64_Off;
typedef int32_t Elf64_Sword;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

/* Size of ELF identifier field in the ELF file header. */
#define     EI_NIDENT       16

/* ELF32 file header */
typedef struct {
	unsigned char e_ident[EI_NIDENT];
	Elf32_Half e_type;
	Elf32_Half e_machine;
	Elf32_Word e_version;
	Elf32_Addr e_entry;
	Elf32_Off e_phoff;
	Elf32_Off e_shoff;
	Elf32_Word e_flags;
	Elf32_Half e_ehsize;
	Elf32_Half e_phentsize;
	Elf32_Half e_phnum;
	Elf32_Half e_shentsize;
	Elf32_Half e_shnum;
	Elf32_Half e_shstrndx;
} Elf32_Ehdr;

/* ELF64 file header */
typedef struct {
	unsigned char e_ident[EI_NIDENT];
	Elf64_Half e_type;
	Elf64_Half e_machine;
	Elf64_Word e_version;
	Elf64_Addr e_entry;
	Elf64_Off e_phoff;
	Elf64_Off e_shoff;
	Elf64_Word e_flags;
	Elf64_Half e_ehsize;
	Elf64_Half e_phentsize;
	Elf64_Half e_phnum;
	Elf64_Half e_shentsize;
	Elf64_Half e_shnum;
	Elf64_Half e_shstrndx;
} Elf64_Ehdr;

/* e_ident */
#define     ET_NONE         0
#define     ET_REL          1	/* Re-locatable file         */
#define     ET_EXEC         2	/* Executable file           */
#define     ET_DYN          3	/* Shared object file        */
#define     ET_CORE         4	/* Core file                 */
#define     ET_LOOS         0xfe00	/* Operating system-specific */
#define     ET_HIOS         0xfeff	/* Operating system-specific */
#define     ET_LOPROC       0xff00	/* remote_proc-specific        */
#define     ET_HIPROC       0xffff	/* remote_proc-specific        */

/* e_machine */
#define     EM_ARM          40	/* ARM/Thumb Architecture    */

/* e_version */
#define     EV_CURRENT      1	/* Current version           */

/* e_ident[] Identification Indexes */
#define     EI_MAG0         0	/* File identification       */
#define     EI_MAG1         1	/* File identification       */
#define     EI_MAG2         2	/* File identification       */
#define     EI_MAG3         3	/* File identification       */
#define     EI_CLASS        4	/* File class                */
#define     EI_DATA         5	/* Data encoding             */
#define     EI_VERSION      6	/* File version              */
#define     EI_OSABI        7	/* Operating system/ABI identification */
#define     EI_ABIVERSION   8	/* ABI version               */
#define     EI_PAD          9	/* Start of padding bytes    */
#define     EI_NIDENT       16	/* Size of e_ident[]         */

/*
 * EI_MAG0 to EI_MAG3 - A file's first 4 bytes hold amagic number, identifying
 * the file as an ELF object file
 */
#define     ELFMAG0         0x7f /* e_ident[EI_MAG0]          */
#define     ELFMAG1         'E'	/* e_ident[EI_MAG1]          */
#define     ELFMAG2         'L'	/* e_ident[EI_MAG2]          */
#define     ELFMAG3         'F'	/* e_ident[EI_MAG3]          */
#define     ELFMAG          "\177ELF"
#define     SELFMAG         4

/*
 * EI_CLASS - The next byte, e_ident[EI_CLASS], identifies the file's class, or
 * capacity.
 */
#define     ELFCLASSNONE    0	/* Invalid class             */
#define     ELFCLASS32      1	/* 32-bit objects            */
#define     ELFCLASS64      2	/* 64-bit objects            */

/*
 * EI_DATA - Byte e_ident[EI_DATA] specifies the data encoding of the
 * remote_proc-specific data in the object file. The following encodings are
 * currently defined.
 */
#define     ELFDATANONE     0	/* Invalid data encoding     */
#define     ELFDATA2LSB     1	/* See Data encodings, below */
#define     ELFDATA2MSB     2	/* See Data encodings, below */

/* EI_OSABI - We do not define an OS specific ABI */
#define     ELFOSABI_NONE   0

/* ELF32 program header */
typedef struct elf32_phdr{
	Elf32_Word p_type;
	Elf32_Off p_offset;
	Elf32_Addr p_vaddr;
	Elf32_Addr p_paddr;
	Elf32_Word p_filesz;
	Elf32_Word p_memsz;
	Elf32_Word p_flags;
	Elf32_Word p_align;
} Elf32_Phdr;

/* ELF64 program header */
typedef struct elf64_phdr {
	Elf64_Word p_type;
	Elf64_Word p_flags;
	Elf64_Off p_offset;
	Elf64_Addr p_vaddr;
	Elf64_Addr p_paddr;
	Elf64_Xword p_filesz;
	Elf64_Xword p_memsz;
	Elf64_Xword p_align;
} Elf64_Phdr;

/* segment types */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_TLS     7               /* Thread local storage segment */
#define PT_LOOS    0x60000000      /* OS-specific */
#define PT_HIOS    0x6fffffff      /* OS-specific */
#define PT_LOPROC  0x70000000
#define PT_HIPROC  0x7fffffff

/* ELF32 section header. */
typedef struct {
	Elf32_Word sh_name;
	Elf32_Word sh_type;
	Elf32_Word sh_flags;
	Elf32_Addr sh_addr;
	Elf32_Off sh_offset;
	Elf32_Word sh_size;
	Elf32_Word sh_link;
	Elf32_Word sh_info;
	Elf32_Word sh_addralign;
	Elf32_Word sh_entsize;
} Elf32_Shdr;

/* ELF64 section header. */
typedef struct {
	Elf64_Word sh_name;
	Elf64_Word sh_type;
	Elf64_Xword sh_flags;
	Elf64_Addr sh_addr;
	Elf64_Off sh_offset;
	Elf64_Xword sh_size;
	Elf64_Word sh_link;
	Elf64_Word sh_info;
	Elf64_Xword sh_addralign;
	Elf64_Xword sh_entsize;
} Elf64_Shdr;

/* sh_type */
#define     SHT_NULL                0
#define     SHT_PROGBITS            1
#define     SHT_SYMTAB              2
#define     SHT_STRTAB              3
#define     SHT_RELA                4
#define     SHT_HASH                5
#define     SHT_DYNAMIC             6
#define     SHT_NOTE                7
#define     SHT_NOBITS              8
#define     SHT_REL                 9
#define     SHT_SHLIB               10
#define     SHT_DYNSYM              11
#define     SHT_INIT_ARRAY          14
#define     SHT_FINI_ARRAY          15
#define     SHT_PREINIT_ARRAY       16
#define     SHT_GROUP               17
#define     SHT_SYMTAB_SHNDX        18
#define     SHT_LOOS                0x60000000
#define     SHT_HIOS                0x6fffffff
#define     SHT_LOPROC              0x70000000
#define     SHT_HIPROC              0x7fffffff
#define     SHT_LOUSER              0x80000000
#define     SHT_HIUSER              0xffffffff

/* sh_flags */
#define     SHF_WRITE       0x1
#define     SHF_ALLOC       0x2
#define     SHF_EXECINSTR   0x4
#define     SHF_MASKPROC    0xf0000000

/* Relocation entry (without addend) */
typedef struct {
	Elf32_Addr r_offset;
	Elf32_Word r_info;

} Elf32_Rel;

typedef struct {
	Elf64_Addr r_offset;
	Elf64_Xword r_info;

} Elf64_Rel;

/* Relocation entry with addend */
typedef struct {
	Elf32_Addr r_offset;
	Elf32_Word r_info;
	Elf32_Sword r_addend;

} Elf32_Rela;

typedef struct elf64_rela {
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
	Elf64_Sxword r_addend;
} Elf64_Rela;

/* Macros to extract information from 'r_info' field of relocation entries */
#define ELF32_R_SYM(i)  ((i) >> 8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF64_R_SYM(i)  ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffff)

/* Symbol table entry */
typedef struct {
	Elf32_Word st_name;
	Elf32_Addr st_value;
	Elf32_Word st_size;
	unsigned char st_info;
	unsigned char st_other;
	Elf32_Half st_shndx;

} Elf32_Sym;

typedef struct elf64_sym {
	Elf64_Word st_name;
	unsigned char	st_info;
	unsigned char	st_other;
	Elf64_Half st_shndx;
	Elf64_Addr st_value;
	Elf64_Xword st_size;
} Elf64_Sym;

/* ARM specific dynamic relocation codes */
#define     R_ARM_GLOB_DAT	21	/* 0x15 */
#define     R_ARM_JUMP_SLOT	22	/* 0x16 */
#define     R_ARM_RELATIVE	23	/* 0x17 */
#define     R_ARM_ABS32		2	/* 0x02 */

/* ELF decoding information */
struct elf32_info {
	Elf32_Ehdr ehdr;
	unsigned int load_state;
	Elf32_Phdr *phdrs;
	Elf32_Shdr *shdrs;
	void *shstrtab;
};

struct elf64_info {
	Elf64_Ehdr ehdr;
	unsigned int load_state;
	Elf64_Phdr *phdrs;
	Elf64_Shdr *shdrs;
	void *shstrtab;
};

#define ELF_STATE_INIT              0x0UL
#define ELF_STATE_WAIT_FOR_PHDRS    0x100UL
#define ELF_STATE_WAIT_FOR_SHDRS    0x200UL
#define ELF_STATE_WAIT_FOR_SHSTRTAB 0x400UL
#define ELF_STATE_HDRS_COMPLETE     0x800UL
#define ELF_STATE_MASK              0xFF00UL
#define ELF_NEXT_SEGMENT_MASK       0x00FFUL

extern struct loader_ops elf_ops;

/**
 * elf_identify - check if it is an ELF file
 *
 * It will check if the input image header is an ELF header.
 *
 * @img_data: firmware private data which will be passed to user defined loader
 *            operations
 * @len: firmware header length
 *
 * return 0 for success or negative value for failure.
 */
int elf_identify(const void *img_data, size_t len);

/**
 * elf_load_header - Load ELF headers
 *
 * It will get the ELF header, the program header, and the section header.
 *
 * @img_data: image data
 * @offset: input image data offset to the start of image file
 * @len: input image data length
 * @img_info: pointer to store image information data
 * @last_load_state: last state return by this function
 * @noffset: pointer to next offset required by loading ELF header
 * @nlen: pointer to next data length required by loading ELF header
 *
 * return ELF loading header state, or negative value for failure
 */
int elf_load_header(const void *img_data, size_t offset, size_t len,
		    void **img_info, int last_load_state,
		    size_t *noffset, size_t *nlen);

/**
 * elf_load - load ELF data
 *
 * It will parse the ELF image and return the target device address,
 * offset to the start of the ELF image of the data to load and the
 * length of the data to load.
 *
 * @rproc: pointer to remoteproc instance
 * @img_data: image data which will passed to the function.
 *            it can be NULL, if image data doesn't need to be handled
 *            by the load function. E.g. binary data which was
 *            loaded to the target memory.
 * @offset: last loaded image data offset to the start of image file
 * @len: last loaded image data length
 * @img_info: pointer to store image information data
 * @last_load_state: the returned state of the last function call.
 * @da: target device address, if the data to load is not for target memory
 *      the da will be set to ANY.
 * @noffset: pointer to next offset required by loading ELF header
 * @nlen: pointer to next data length required by loading ELF header
 * @padding: value to pad it is possible that a size of a segment in memory
 *           is larger than what it is in the ELF image. e.g. a segment
 *           can have stack section .bss. It doesn't need to copy image file
 *           space, in this case, it will be packed with 0.
 * @nmemsize: pointer to next data target memory size. The size of a segment
 *            in the target memory can be larger than the its size in the
 *            image file.
 *
 * return 0 for success, otherwise negative value for failure
 */
int elf_load(struct remoteproc *rproc, const void *img_data,
	     size_t offset, size_t len,
	     void **img_info, int last_load_state,
	     metal_phys_addr_t *da,
	     size_t *noffset, size_t *nlen,
	     unsigned char *padding, size_t *nmemsize);

/**
 * elf_release - Release ELF image information
 *
 * It will release ELF image information data.
 *
 * @img_info: pointer to ELF image information
 */
void elf_release(void *img_info);

/**
 * elf_get_entry - Get entry point
 *
 * It will return entry point specified in the ELF file.
 *
 * @img_info: pointer to ELF image information
 *
 * return entry address
 */
metal_phys_addr_t elf_get_entry(void *img_info);

/**
 * elf_locate_rsc_table - locate the resource table information
 *
 * It will return the length of the resource table, and the device address of
 * the resource table.
 *
 * @img_info: pointer to ELF image information
 * @da: pointer to the device address
 * @offset: pointer to the offset to in the ELF image of the resource
 *          table section.
 * @size: pointer to the size of the resource table section.
 *
 * return 0 if successfully locate the resource table, negative value for
 * failure.
 */
int elf_locate_rsc_table(void *img_info, metal_phys_addr_t *da,
			 size_t *offset, size_t *size);

#if defined __cplusplus
}
#endif

#endif /* ELF_LOADER_H_ */
