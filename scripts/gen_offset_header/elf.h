/*
 * Copyright (c) 2010, 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file
 * @brief Executable and Linking Format (ELF) definitions
 *
 * The definitions in this header conform to the "Tool Interface Standard (TIS)
 * Executable and Linking Format (ELF) Specification Version 1.2"
 */

#ifndef _ELF_H
#define _ELF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int    Elf32_Addr;
typedef unsigned short  Elf32_Half;
typedef unsigned int    Elf32_Off;
typedef int             Elf32_Sword;
typedef unsigned int    Elf32_Word;


/*
 * Elf header
 */

#define EI_NIDENT	16

typedef struct
	{
	unsigned char e_ident[EI_NIDENT];
	Elf32_Half	e_type;
	Elf32_Half	e_machine;
	Elf32_Word	e_version;
	Elf32_Addr	e_entry;
	Elf32_Off	e_phoff;
	Elf32_Off	e_shoff;
	Elf32_Word	e_flags;
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;
	Elf32_Half	e_phnum;
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;
	} Elf32_Ehdr;

#define EHDRSZ sizeof(Elf32_Ehdr)

/*
 * e_ident[] values
 */
#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6
#define EI_PAD		7

#define ELFMAG0		0x7f
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'
#define ELFMAG		"\177ELF"
#define SELFMAG		4

/*
 * EI_CLASS
 */
#define ELFCLASSNONE	0
#define ELFCLASS32	1
#define ELFCLASS64	2

/*
 * EI_DATA
 */
#define ELFDATANONE	0
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

/*
 * e_type
 */
#define ET_NONE		0
#define ET_REL		1
#define ET_EXEC		2
#define ET_DYN		3
#define ET_CORE		4
#define ET_LOPROC	0xff00
#define ET_HIPROC	0xffff

/*
 * e_machine
 */
#define EM_NONE		0		/* No machine */
#define EM_M32		1		/* AT&T WE 32100 */
#define EM_SPARC	2		/* SPARC */
#define EM_386		3		/* Intel 80386 */
#define EM_68K		4		/* Motorola 68000 */
#define EM_88K		5		/* Motorola 88000 */
#define EM_486		6		/* Intel 80486 */
#define EM_860		7		/* Intel 80860 */
#define EM_MIPS		8		/* MIPS RS3000 Big-Endian */
#define EM_MIPS_RS4_BE	10		/* MIPS RS4000 Big-Endian */
#define EM_PPC_OLD	17		/* PowerPC - old */
#define EM_PPC		20		/* PowerPC */
#define EM_RCE_OLD	25		/* RCE - old */
#define EM_NEC_830	36		/* NEC 830 series */
#define EM_RCE		39		/* RCE */
#define EM_MCORE	39		/* MCORE */
#define EM_ARM          40              /* ARM  */
#define EM_SH		42		/* SH */
#define EM_COLDFIRE	52		/* Motorola ColdFire */
#define EM_SC		58		/* SC */
#define EM_M32R		36929		/* M32R */
#define EM_NEC		36992		/* NEC 850 series */

/*
 * e_flags
 */
#define EF_PPC_EMB		0x80000000

#define EF_MIPS_NOREORDER	0x00000001
#define EF_MIPS_PIC		0x00000002
#define EF_MIPS_CPIC		0x00000004
#define EF_MIPS_ARCH		0xf0000000
#define EF_MIPS_ARCH_MIPS_2	0x10000000
#define	EF_MIPS_ARCH_MIPS_3	0x20000000

/*
 * e_version and EI_VERSION
 */
#define EV_NONE		0
#define EV_CURRENT	1

/*
 * Special section indexes
 */
#define SHN_UNDEF	0
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC	0xff00
#define SHN_HIPROC	0xff1f
#define SHN_ABS		0xfff1
#define SHN_COMMON	0xfff2
#define SHN_HIRESERVE	0xffff

#define SHN_GHCOMMON	0xff00
/*
 * Section header
 */
typedef struct
	{
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;	/* SHT_... */
	Elf32_Word	sh_flags;	/* SHF_... */
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
} Elf32_Shdr;

#define SHDRSZ sizeof(Elf32_Shdr)

/*
 * sh_type
 */
#define SHT_NULL	    0
#define SHT_PROGBITS	    1
#define SHT_SYMTAB	    2
#define SHT_STRTAB	    3
#define SHT_RELA	    4
#define SHT_HASH	    5
#define SHT_DYNAMIC	    6
#define SHT_NOTE	    7
#define SHT_NOBITS	    8
#define SHT_REL		    9
#define SHT_SHLIB	    10
#define SHT_DYNSYM	    11
#define SHT_COMDAT	    12
#define SHT_LOPROC	    0x70000000
#define SHT_HIPROC	    0x7fffffff
#define SHT_LOUSER	    0x80000000
#define SHT_HIUSER	    0xffffffff
#define SHT_INIT_ARRAY	    14
#define SHT_FINI_ARRAY	    15
#define SHT_PREINIT_ARRAY   16
#define SHT_GROUP	    17
#define SHT_SYMTAB_SHNDX    18
#define SHT_NUM		    19

/*
 * sh_flags
 */
#define SHF_WRITE		0x1
#define SHF_ALLOC		0x2
#define SHF_EXECINSTR		0x4
#define SHF_MASKPROC		0xf0000000
#define SHF_MERGE		0x10  /* not part of all ELF ABI docs */
#define SHF_STRINGS		0x20  /* not part of all ELF ABI docs */
#define SHF_INFO_LINK		0x40
#define SHF_LINK_ORDER		0x80
#define SHF_OS_NONCONFORMING	0x100
#define SHF_GROUP		0x200
#define SHF_TLS			0x400

/*
 * Symbol table
 */
typedef struct
	{
	Elf32_Word	st_name;
	Elf32_Addr	st_value;
	Elf32_Word	st_size;
	unsigned char	st_info;
	unsigned char	st_other;
	Elf32_Half	st_shndx;
	} Elf32_Sym;

#define STN_UNDEF	0

#define STB_LOCAL	0
#define STB_GLOBAL	1
#define STB_WEAK	2
#define STB_LOPROC	13
#define STB_HIPROC	15

#define STT_NOTYPE	0
#define STT_OBJECT	1
#define STT_FUNC	2
#define STT_SECTION	3
#define STT_FILE	4
#define STT_COMMON	5
#define STT_TLS		6
#define STT_LOOS	10
#define STT_HIOS	12
#define STT_LOPROC	13
#define STT_HIPROC	15

#define ELF_ST_BIND(x)          ((x) >> 4)
#define ELF_ST_TYPE(x)          (((unsigned int) x) & 0xf)
#define ELF32_ST_BIND(x)        ELF_ST_BIND(x)
#define ELF32_ST_TYPE(x)        ELF_ST_TYPE(x)


/*
 * The STT_ARM_TFUNC type is used by the gnu compiler to mark Thumb
 * functions.  The STT_ARM_16BIT type is the thumb equivalent of an
 * object.  They are not part of the ARM ABI or EABI - they come from gnu.
 */

#define STT_ARM_TFUNC   STT_LOPROC      /* GNU Thumb function */
#define STT_ARM_16BIT   STT_HIPROC      /* GNU Thumb label    */


/*
 * Relocation
 */
typedef struct
	{
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
	} Elf32_Rel;

typedef struct
	{
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
	Elf32_Sword	r_addend;
	} Elf32_Rela;

#define ELF32_R_SYM(info)	((info)>>8)
#define ELF32_R_TYPE(info)	((unsigned char)(info))
#define ELF32_R_INFO(sym,type)	(((sym)<<8)+(unsigned char)(type))

/*
 * Program header
 */
typedef struct
	{
	Elf32_Word	p_type;
	Elf32_Off	p_offset;
	Elf32_Addr	p_vaddr;
	Elf32_Addr	p_paddr;
	Elf32_Word	p_filesz;
	Elf32_Word	p_memsz;
	Elf32_Word	p_flags;
	Elf32_Word	p_align;
	} Elf32_Phdr;

#define PHDRSZ sizeof(Elf32_Phdr)

/*
 * p_type
 */
#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6
#define PT_LOPROC	0x70000000
#define PT_HIPROC	0x7fffffff

/*
 * p_flags
 */
#define PF_X		0x1
#define PF_W		0x2
#define PF_R		0x4
#define PF_MASKPROC	0xf0000000

typedef struct
	{
	Elf32_Sword	d_tag;
	union
		{
	Elf32_Word  d_val;
	Elf32_Addr  d_ptr;
		} d_un;
	} Elf32_Dyn;

#define DT_NULL		0
#define DT_NEEDED	1
#define DT_PLTRELSZ	2
#define DT_PLTGOT	3
#define DT_HASH		4
#define DT_STRTAB	5
#define DT_SYMTAB	6
#define DT_RELA		7
#define DT_RELASZ	8
#define DT_RELAENT	9
#define DT_STRSZ	10
#define DT_SYMENT	11
#define DT_INIT		12
#define DT_FINI		13
#define DT_SONAME	14
#define DT_RPATH	15
#define DT_SYMBOLIC	16
#define DT_REL		17
#define DT_RELSZ	18
#define DT_RELENT	19
#define DT_PLTREL	20
#define DT_DEBUG	21
#define DT_TEXTREL	22
#define DT_JMPREL	23
#define DT_LOPROC   0x70000000
#define DT_HIPROC   0x7fffffff


#ifdef __cplusplus
}
#endif

#endif /* _ELF_H */
