/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief generate offset definition header file
 *
 * genOffsetHeader -i <objectModule> -o <outputHeaderName>
 *
 * This Zephyr development host utility will process an ELF object module that
 * consists of a series of absolute symbols representing the byte offset of a
 * structure member and the size of the structure.  Each absolute symbol will
 * be translated into a C preprocessor '#define' directive.  For example,
 * assuming that the module offsets.o contains the following absolute symbols:
 *
 *   $ nm offsets.o
 *   00000000 A ___kernel_t_nested_OFFSET
 *   00000004 A ___kernel_t_irq_stack_OFFSET
 *   00000008 A ___kernel_t_current_OFFSET
 *   0000000c A ___kernel_t_idle_OFFSET
 *
 * ... the following C preprocessor code will be generated:
 *
 *   #define   ___kernel_t_nested_OFFSET      0x0
 *   #define   ___kernel_t_irq_stack_OFFSET   0x4
 *   #define   ___kernel_t_current_OFFSET     0x8
 *   #define   ___kernel_t_idle_OFFSET        0xc
 */

/* includes */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "elf.h"
#include <stdlib.h>		/* for malloc()/free()/exit() */
#include <string.h>		/* for strstr() */
#include <getopt.h>
#include <errno.h>

/* defines */

#undef DEBUG

/* the symbol name suffix used to denote structure member offsets */

#define STRUCT_OFF_SUFFIX "_OFFSET"
#define STRUCT_SIZ_SUFFIX "_SIZEOF"

#ifdef DEBUG
#define DBG_PRINT(args...) printf(args)
#else
#define DBG_PRINT(args...)
#endif


/* byte swapping macros */

#define SWAB_Elf32_Half(x)	((x >> 8) | (x << 8))
#define SWAB_Elf32_Word(x)	(((x >> 24) & 0xff) |		\
				((x << 8)  & 0xff0000) |	\
		                ((x >> 8)  & 0xff00) |		\
	                        ((x << 24) & 0xff000000))

#define SWAB_Elf32_Addr	  SWAB_Elf32_Word
#define SWAB_Elf32_Off    SWAB_Elf32_Word
#define SWAB_Elf32_Sword  SWAB_Elf32_Word

#if defined(_WIN32) || defined(__CYGWIN32__) || defined(__WIN32__)
  #define OPEN_FLAGS (O_RDONLY|O_BINARY)
#else
  #define OPEN_FLAGS (O_RDONLY)
#endif

/* locals */

/* global indicating whether ELF structures need to be byte swapped */

static char swabRequired;


/* usage information */

static char usage[] = "usage: %s -i <objectModule> -o <outputHeaderName>\n";


/* output header file preamble */

static char preamble1[] = "\
/* %s - structure member offsets definition header */\n\
\n\
/*\n\
 * Copyright (c) 2010-2014 Wind River Systems, Inc.\n\
 *\n\
 * Licensed under the Apache License, Version 2.0 (the \"License\");\n\
 * you may not use this file except in compliance with the License.\n\
 * You may obtain a copy of the License at\n\
 *\n\
 *     http://www.apache.org/licenses/LICENSE-2.0\n\
 *\n\
 * Unless required by applicable law or agreed to in writing, software\n\
 * distributed under the License is distributed on an \"AS IS\" BASIS,\n\
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n\
 * See the License for the specific language governing permissions and\n\
 * limitations under the License.\n\
 * SPDX-License-Identifier: Apache-2.0\n\
 */\n\
\n\
/* THIS FILE IS AUTO GENERATED.  PLEASE DO NOT EDIT */\n\
\n\
/*\n\
 * This header file provides macros for the offsets of various structure\n\
 * members.  These offset macros are primarily intended to be used in\n\
 * assembly code.\n\
 */\n\n";

static char preamble2[] = "\
/*\n\
 * Auto-generated header guard.\n\
 */\n\
#ifndef %s\n\
#define %s\n\
 \n\
#ifdef __cplusplus\n\
extern \"C\" {\n\
#endif\n\
\n\
/* defines */\n\n";


/* output header file postscript */

static char postscript[] = "\
\n\
#ifdef __cplusplus\n\
}\n\
#endif\n\
\n\
#endif /* _HGUARD_ */\n";

static Elf32_Ehdr	ehdr;    /* ELF header */
static Elf32_Shdr *	shdr;    /* pointer to array ELF section headers */

/**
 * @brief byte swap the Elf32_Ehdr structure
 *
 * @returns N/A
 */
static void swabElfHdr(Elf32_Ehdr *pHdrToSwab)
{
	if (swabRequired == 0)
	{
		return;  /* do nothing */
	}

	pHdrToSwab->e_type		= SWAB_Elf32_Half(pHdrToSwab->e_type);
	pHdrToSwab->e_machine	= SWAB_Elf32_Half(pHdrToSwab->e_machine);
	pHdrToSwab->e_version	= SWAB_Elf32_Word(pHdrToSwab->e_version);
	pHdrToSwab->e_entry		= SWAB_Elf32_Addr(pHdrToSwab->e_entry);
	pHdrToSwab->e_phoff		= SWAB_Elf32_Off(pHdrToSwab->e_phoff);
	pHdrToSwab->e_shoff		= SWAB_Elf32_Off(pHdrToSwab->e_shoff);
	pHdrToSwab->e_flags		= SWAB_Elf32_Word(pHdrToSwab->e_flags);
	pHdrToSwab->e_ehsize	= SWAB_Elf32_Half(pHdrToSwab->e_ehsize);
	pHdrToSwab->e_phentsize	= SWAB_Elf32_Half(pHdrToSwab->e_phentsize);
	pHdrToSwab->e_phnum		= SWAB_Elf32_Half(pHdrToSwab->e_phnum);
	pHdrToSwab->e_shentsize	= SWAB_Elf32_Half(pHdrToSwab->e_shentsize);
	pHdrToSwab->e_shnum		= SWAB_Elf32_Half(pHdrToSwab->e_shnum);
	pHdrToSwab->e_shstrndx	= SWAB_Elf32_Half(pHdrToSwab->e_shstrndx);
}

/**
 * @brief byte swap the Elf32_Shdr structure
 *
 * @returns N/A
 */
static void swabElfSectionHdr(Elf32_Shdr *pHdrToSwab)
{
	if (swabRequired == 0)
	{
		return;  /* do nothing */
	}

	pHdrToSwab->sh_name		= SWAB_Elf32_Word(pHdrToSwab->sh_name);
	pHdrToSwab->sh_type		= SWAB_Elf32_Word(pHdrToSwab->sh_type);
	pHdrToSwab->sh_flags	= SWAB_Elf32_Word(pHdrToSwab->sh_flags);
	pHdrToSwab->sh_addr		= SWAB_Elf32_Addr(pHdrToSwab->sh_addr);
	pHdrToSwab->sh_offset	= SWAB_Elf32_Off(pHdrToSwab->sh_offset);
	pHdrToSwab->sh_size		= SWAB_Elf32_Word(pHdrToSwab->sh_size);
	pHdrToSwab->sh_link		= SWAB_Elf32_Word(pHdrToSwab->sh_link);
	pHdrToSwab->sh_info		= SWAB_Elf32_Word(pHdrToSwab->sh_info);
	pHdrToSwab->sh_addralign	= SWAB_Elf32_Word(pHdrToSwab->sh_addralign);
	pHdrToSwab->sh_addralign	= SWAB_Elf32_Word(pHdrToSwab->sh_addralign);
}


/**
 *
 * @brief byte swap the Elf32_Sym structure
 *
 * @returns N/A
 */
static void swabElfSym(Elf32_Sym *pHdrToSwab)
{
	if (swabRequired == 0)
	{
		return;  /* do nothing */
	}

	pHdrToSwab->st_name		= SWAB_Elf32_Word(pHdrToSwab->st_name);
	pHdrToSwab->st_value	= SWAB_Elf32_Addr(pHdrToSwab->st_value);
	pHdrToSwab->st_size		= SWAB_Elf32_Word(pHdrToSwab->st_size);
	pHdrToSwab->st_shndx	= SWAB_Elf32_Half(pHdrToSwab->st_shndx);
}

/**
 * @brief load the ELF header
 *
 * @param fd file descriptor of file from which to read
 * @returns 0 on success, -1 on failure
 */
static int ehdrLoad(int fd)
{
	unsigned  ix = 0x12345678;  /* used to auto-detect endian-ness */
	size_t    nBytes;           /* number of bytes read from file */

	if (lseek(fd, 0, SEEK_SET) == -1) {
		fprintf(stderr, "Unable to seek\n");
		return -1;
	}

	nBytes = read(fd, &ehdr, sizeof(ehdr));
	if (nBytes != sizeof(ehdr))
	{
		fprintf(stderr, "Failed to read ELF header\n");
		return -1;
	}

	/* perform some rudimentary ELF file validation */

	if (strncmp((char *)ehdr.e_ident, ELFMAG, 4) != 0)
	{
		fprintf(stderr, "Input object module not ELF format\n");
		return -1;
	}

	/* 64-bit ELF module not supported (for now) */

	if (ehdr.e_ident[EI_CLASS] != ELFCLASS32)
	{
		fprintf(stderr, "ELF64 class not supported\n");
		return -1;
	}

	/*
	 * Dynamically determine the endianess of the host (in the absence of
	 * a compile time macro ala _BYTE_ORDER).  The ELF structures will require
	 * byte swapping if the host and target have different byte ordering.
	 */

	if (((*(char*)&ix == 0x78) && (ehdr.e_ident[EI_DATA] == ELFDATA2MSB)) ||
			((*(char*)&ix == 0x12) && (ehdr.e_ident[EI_DATA] == ELFDATA2LSB)))
	{
		swabRequired = 1;
		DBG_PRINT("Swab required\n");
	}

	swabElfHdr(&ehdr);   /* swap bytes (if required) */

	/* debugging: dump some important ELF header fields */

	DBG_PRINT("Elf header Magic = %s\n", ehdr.e_ident);
	DBG_PRINT("Elf header e_type = %d\n", ehdr.e_type);
	DBG_PRINT("Elf header e_shstrndx = %d\n", ehdr.e_shstrndx);
	DBG_PRINT("Elf header e_shnum = %d\n", ehdr.e_shnum);

	return 0;
}

/**
 * @brief load the section headers
 * @param fd file descriptor of file from which to read
 *
 * @returns 0 on success, -1 on failure
 */
static int shdrsLoad(int fd)
{
	size_t    nBytes;    /* number of bytes read from file */
	unsigned  ix;        /* loop index */

	shdr = malloc(ehdr.e_shnum * sizeof(Elf32_Shdr));
	if (shdr == NULL)
	{
		fprintf(stderr, "No memory for section headers!\n");
		return -1;
	}

	/* Seek to the start of the table of section headers */
	if (lseek(fd, ehdr.e_shoff, SEEK_SET) == -1) {
		fprintf(stderr, "Unable to seek\n");
		return -1;
	}

	for (ix = 0; ix < ehdr.e_shnum; ix++)
	{
		nBytes = read(fd, &shdr[ix], sizeof(Elf32_Shdr));
		if (nBytes != sizeof(Elf32_Shdr))
		{
			fprintf(stderr, "Unable to read entire section header (#%d)\n",
					ix);
			return -1;
		}

		swabElfSectionHdr(&shdr[ix]);   /* swap bytes (if required) */
	}

	return 0;
}

/**
 *
 * symTblFind - search the section headers for the symbol table
 *
 * This routine searches the section headers for the symbol table.  There is
 * expected to be only one symbol table in the section headers.
 *
 * @param pSymTblOffset ptr to symbol table offset
 * @param pSymTblSize ptr to symbol table size
 * @returns 0 if found, -1 if not
 */
static int symTblFind(unsigned *pSymTblOffset, unsigned *pSymTblSize)
{
	unsigned  ix;    /* loop index */

	for (ix = 0; ix < ehdr.e_shnum; ++ix)
	{
		if (shdr[ix].sh_type == SHT_SYMTAB)
		{
			*pSymTblOffset = shdr[ix].sh_offset;
			*pSymTblSize   = shdr[ix].sh_size;

			return 0;
		}
	}

	fprintf(stderr, "Object module missing symbol table!\n");

	return -1;
}

/**
 *
 * @brief search the section headers for the string table
 *
 * This routine searches the section headers for the string table associated
 * with the symbol names.
 *
 * Typically, there are two string tables defined in the section headers.  These
 * are ".shstrtbl" and ".strtbl" for section header names and symbol names
 * respectively.  It has been observed with the DIAB compiler (but not with
 * either the GCC nor ICC compilers) that the two tables may be mashed together
 * into one.  Consequently, the following algorithm is used to select the
 * appropriate string table.
 *
 * 1. Assume that the first found string table is valid.
 * 2. If another string table is found, use that only if its section header
 *    index does not match the index for ".shstrtbl" stored in the ELF header.
 *
 * @param pStrTblIx ptr to string table's index
 * @returns 0 if found, -1 if not
 */
static int strTblFind(unsigned *pStrTblIx)
{
	unsigned  strTblIx = 0xffffffff;
	unsigned  ix;

	for (ix = 0; ix < ehdr.e_shnum; ++ix)
	{
		if (shdr[ix].sh_type == SHT_STRTAB)
		{
			if ((strTblIx == 0xffffffff) ||
					(ix != ehdr.e_shstrndx))
			{
				strTblIx = ix;
			}
		}
	}

	if (strTblIx == 0xffffffff)
	{
		fprintf(stderr, "Object module missing string table!\n");
		return -1;
	}

	*pStrTblIx = strTblIx;

	return 0;
}

/**
 * @brief load the string table
 *
 * @param fd file descriptor of file from which to read
 * @param strTblIx string table's index
 * @param ppStringTable ptr to ptr to string table
 * @returns 0 on success, -1 on failure
 */
static int strTblLoad(int fd, unsigned strTblIx, char **ppStringTable)
{
	char *  pTable;
	int     nBytes;

	DBG_PRINT("Allocating %d bytes for string table\n",
			shdr[strTblIx].sh_size);

	pTable = malloc(shdr[strTblIx].sh_size);
	if (pTable == NULL)
	{
		fprintf(stderr, "No memory for string table!");
		return -1;
	}

	if (lseek(fd, shdr[strTblIx].sh_offset, SEEK_SET) == -1) {
		free(pTable);
		fprintf(stderr, "Unable to seek\n");
		return -1;
	}

	nBytes = read(fd, pTable, shdr[strTblIx].sh_size);
	if (nBytes != shdr[strTblIx].sh_size)
	{
		free(pTable);
		fprintf(stderr, "Unable to read entire string table!\n");
		return -1;
	}

	*ppStringTable = pTable;

	return 0;
}

/**
 *
 * @brief dump the header preamble to the header file
 *
 * @param fd file pointer to which to write
 * @parama filename name of the output file
 * @returns N/A
 */
static void headerPreambleDump(FILE *fp, char *filename)
{
	unsigned hash = 5381;      /* hash value */
	size_t   ix;               /* loop counter */
	char     fileNameHash[20]; /* '_HGUARD_' + 8 character hash + '\0' */

	/* dump header file preamble1[] */

	fprintf(fp, preamble1, filename, filename, filename);

	/*
	 * Dump header file preamble2[]. Hash file name into something that
	 * is small enough to be a C macro name and does not have invalid
	 * characters for a macro name to use as a header guard. The result
	 * of the hash should be unique enough for our purposes.
	 */

	for (ix = 0; ix < sizeof(filename); ++ix)
	{
		hash = (hash * 33) + (unsigned int) filename[ix];
	}

	sprintf(fileNameHash, "_HGUARD_%08x", hash);
	fprintf(fp, preamble2, fileNameHash, fileNameHash);
}

/**
 * @brief dump the absolute symbols to the header file
 *
 * @param fd file descriptor of file from which to read
 * @param fp file pointer to which to write
 * @param symTblOffset symbol table offset
 * @param symTblSize size of the symbol table
 * @param pStringTable ptr to the string table
 * @returns N/A
 */
static void headerAbsoluteSymbolsDump(int fd, FILE *fp, Elf32_Off symTblOffset,
		Elf32_Word symTblSize, char *pStringTable)
{
	Elf32_Sym  aSym;     /* absolute symbol */
	unsigned   ix;       /* loop counter */
	unsigned   numSyms;  /* number of symbols in the symbol table */
	size_t	   nBytes;

	/* context the symbol table: pick out absolute syms */

	numSyms = symTblSize / sizeof(Elf32_Sym);
	if (lseek(fd, symTblOffset, SEEK_SET) == -1) {
		fprintf(stderr, "Unable to seek\n");
		return;
	}

	for (ix = 0; ix < numSyms; ++ix)
	{
		/* read in a single symbol structure */
		nBytes = read(fd, &aSym, sizeof(Elf32_Sym));

		if (nBytes) {
			swabElfSym(&aSym);    /* swap bytes (if required) */
		}

		/*
		 * Only generate definitions for global absolute symbols
		 * of the form *_OFFSET
		 */

		if ((aSym.st_shndx == SHN_ABS) &&
				(ELF_ST_BIND(aSym.st_info) == STB_GLOBAL))
		{
			if ((strstr(&pStringTable[aSym.st_name],
							STRUCT_OFF_SUFFIX) != NULL) ||
					(strstr(&pStringTable[aSym.st_name],
						 STRUCT_SIZ_SUFFIX) != NULL))
			{
				fprintf(fp, "#define\t%s\t0x%X\n",
						&pStringTable[aSym.st_name], aSym.st_value);
			}
		}
	}
}

/**
 *
 * @brief dump the header postscript to the header file
 * @param fp file pointer to which to write
 * @returns N/A
 */
static void headerPostscriptDump(FILE *fp)
{
	fputs(postscript, fp);
}

/**
 * @brief entry point for the genOffsetHeader utility
 *
 * usage: $ genOffsetHeader -i <objectModule> -o <outputHeaderName>
 *
 * @returns 0 on success, 1 on failure
 */
int main(int argc, char *argv[])
{
	Elf32_Off	symTblOffset = 0;
	Elf32_Word	symTblSize;		/* in bytes */
	char *	pStringTable = NULL;
	char *	inFileName = NULL;
	char *	outFileName = NULL;
	int		option;
	int		inFd = -1;
	FILE *	outFile = NULL;
	unsigned    strTblIx;

	/* argument parsing */

	if (argc != 5)
	{
		fprintf(stderr, usage, argv[0]);
		goto errorReturn;
	}

	while ((option = getopt(argc, argv, "i:o:")) != -1)
	{
		switch (option)
		{
			case 'i':
				inFileName = optarg;
				break;
			case 'o':
				outFileName = optarg;
				break;
			default:
				fprintf(stderr, usage, argv[0]);
				goto errorReturn;
		}
	}

	/* open input object ELF module and output header file */

	inFd = open(inFileName, OPEN_FLAGS);

	if (inFd == -1)
	{
		fprintf(stderr, "Cannot open input object module");
		goto errorReturn;
	}

	outFile = fopen(outFileName, "w");

	if (outFile == NULL)
	{
		fprintf(stderr, "Cannot open output header file");
		goto errorReturn;
	}

	/*
	 * In the following order, attempt to ...
	 *  1. Load the ELF header
	 *  2. Load the section headers
	 *  3. Find the symbol table
	 *  4. Find the string table
	 *  5. Load the string table
	 * Bail if any of those steps fail.
	 */

	if ((ehdrLoad(inFd) != 0) ||
			(shdrsLoad(inFd) != 0) ||
			(symTblFind(&symTblOffset, &symTblSize) != 0) ||
			(strTblFind(&strTblIx) != 0) ||
			(strTblLoad(inFd, strTblIx, &pStringTable) != 0))
	{
		goto errorReturn;
	}


	/*
	 * Dump the following to the header file ...
	 *  1. Header file preamble
	 *  2. Absolute symbols
	 *  3. Header file postscript
	 */

	headerPreambleDump(outFile, outFileName);
	headerAbsoluteSymbolsDump(inFd, outFile,
			symTblOffset, symTblSize, pStringTable);
	headerPostscriptDump(outFile);

	/* done: cleanup */

	close(inFd);
	fclose(outFile);
	free(shdr);
	free(pStringTable);

	return 0;

errorReturn:
	if (inFd != -1)
	{
		close(inFd);
	}

	if (outFile != NULL)
	{
		fclose(outFile);
	}

	if (shdr != NULL)
	{
		free(shdr);
	}

	if (pStringTable != NULL)
	{
		free(pStringTable);
	}

	return 1;
}
