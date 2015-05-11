/* genOffsetHeader.c - generate offset definition header file */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
SYNOPSIS
.tS
genOffsetHeader -i <objectModule> -o <outputHeaderName>
.tE

DESCRIPTION
This VxMicro development host utility will process an ELF object module that
consists of a series of absolute symbols representing the byte offset of a
structure member and the size of the structure.  Each absolute symbol will
be translated into a C preprocessor '#define' directive.  For example,
assuming that the module offsets.o contains the following absolute symbols:

  $ nm offsets.o
  00000010 A __tNANO_common_isp_OFFSET
  00000008 A __tNANO_current_OFFSET
  0000000c A __tNANO_nested_OFFSET
  00000000 A __tNANO_fiber_OFFSET
  00000004 A __tNANO_task_OFFSET

... the following C preprocessor code will be generated:

  #define   __tNANO_common_isp_OFFSET  0x10
  #define   __tNANO_current_OFFSET     0x8
  #define   __tNANO_nested_OFFSET      0xC
  #define   __tNANO_fiber_OFFSET       0x0
  #define   __tNANO_task_OFFSET        0x4
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

#if defined(WINDOWS)
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
 * Redistribution and use in source and binary forms, with or without\n\
 * modification, are permitted provided that the following conditions are met:\n\
 *\n\
 * 1) Redistributions of source code must retain the above copyright notice,\n\
 * this list of conditions and the following disclaimer.\n\
 *\n\
 * 2) Redistributions in binary form must reproduce the above copyright notice,\n\
 * this list of conditions and the following disclaimer in the documentation\n\
 * and/or other materials provided with the distribution.\n\
 *\n\
 * 3) Neither the name of Wind River Systems nor the names of its contributors\n\
 * may be used to endorse or promote products derived from this software without\n\
 * specific prior written permission.\n\
 *\n\
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n\
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n\
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n\
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE\n\
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n\
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n\
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n\
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n\
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n\
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n\
 * POSSIBILITY OF SUCH DAMAGE.\n\
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
static Elf32_Shdr      *shdr;    /* pointer to array ELF section headers */

/*******************************************************************************
*
* swabElfHdr - byte swap the Elf32_Ehdr structure
*
* RETURNS: N/A
*/

static void swabElfHdr(Elf32_Ehdr *pHdrToSwab)
{
	if (swabRequired == 0) {
	   return;  /* do nothing */
	   }

	pHdrToSwab->e_type		= SWAB_Elf32_Half (pHdrToSwab->e_type);
	pHdrToSwab->e_machine	= SWAB_Elf32_Half (pHdrToSwab->e_machine);
	pHdrToSwab->e_version	= SWAB_Elf32_Word (pHdrToSwab->e_version);
	pHdrToSwab->e_entry		= SWAB_Elf32_Addr (pHdrToSwab->e_entry);
	pHdrToSwab->e_phoff		= SWAB_Elf32_Off  (pHdrToSwab->e_phoff);
	pHdrToSwab->e_shoff		= SWAB_Elf32_Off  (pHdrToSwab->e_shoff);
	pHdrToSwab->e_flags		= SWAB_Elf32_Word (pHdrToSwab->e_flags);
	pHdrToSwab->e_ehsize	= SWAB_Elf32_Half (pHdrToSwab->e_ehsize);
	pHdrToSwab->e_phentsize	= SWAB_Elf32_Half (pHdrToSwab->e_phentsize);
	pHdrToSwab->e_phnum		= SWAB_Elf32_Half (pHdrToSwab->e_phnum);
	pHdrToSwab->e_shentsize	= SWAB_Elf32_Half (pHdrToSwab->e_shentsize);
	pHdrToSwab->e_shnum		= SWAB_Elf32_Half (pHdrToSwab->e_shnum);
	pHdrToSwab->e_shstrndx	= SWAB_Elf32_Half (pHdrToSwab->e_shstrndx);
}

/*******************************************************************************
*
* swabElfSectionHdr - byte swap the Elf32_Shdr structure
*
* RETURNS: N/A
*/

static void swabElfSectionHdr(Elf32_Shdr *pHdrToSwab)
{
	if (swabRequired == 0) {
	   return;  /* do nothing */
	   }

	pHdrToSwab->sh_name		= SWAB_Elf32_Word (pHdrToSwab->sh_name);
	pHdrToSwab->sh_type		= SWAB_Elf32_Word (pHdrToSwab->sh_type);
	pHdrToSwab->sh_flags	= SWAB_Elf32_Word (pHdrToSwab->sh_flags);
	pHdrToSwab->sh_addr		= SWAB_Elf32_Addr (pHdrToSwab->sh_addr);
	pHdrToSwab->sh_offset	= SWAB_Elf32_Off  (pHdrToSwab->sh_offset);
	pHdrToSwab->sh_size		= SWAB_Elf32_Word (pHdrToSwab->sh_size);
	pHdrToSwab->sh_link		= SWAB_Elf32_Word (pHdrToSwab->sh_link);
	pHdrToSwab->sh_info		= SWAB_Elf32_Word (pHdrToSwab->sh_info);
	pHdrToSwab->sh_addralign	= SWAB_Elf32_Word (pHdrToSwab->sh_addralign);
	pHdrToSwab->sh_addralign	= SWAB_Elf32_Word (pHdrToSwab->sh_addralign);
}


/*******************************************************************************
*
* swabElfSym - byte swap the Elf32_Sym structure
*
* RETURNS: N/A
*/

static void swabElfSym(Elf32_Sym *pHdrToSwab)
{
	if (swabRequired == 0) {
	   return;  /* do nothing */
	   }

	pHdrToSwab->st_name		= SWAB_Elf32_Word (pHdrToSwab->st_name);
	pHdrToSwab->st_value	= SWAB_Elf32_Addr (pHdrToSwab->st_value);
	pHdrToSwab->st_size		= SWAB_Elf32_Word (pHdrToSwab->st_size);
	pHdrToSwab->st_shndx	= SWAB_Elf32_Half (pHdrToSwab->st_shndx);
}

/*******************************************************************************
*
* ehdrLoad - load the ELF header
*
* RETURNS: 0 on success, -1 on failure
*/

static int ehdrLoad(int fd  /* file descriptor of file from which to read */
					)
{
	unsigned  ix = 0x12345678;  /* used to auto-detect endian-ness */
	size_t    nBytes;           /* number of bytes read from file */

	lseek (fd, 0, SEEK_SET);

	nBytes = read (fd, &ehdr, sizeof (ehdr));
	if (nBytes != sizeof (ehdr)) {
		fprintf (stderr, "Failed to read ELF header\n");
		return -1;
		}

    /* perform some rudimentary ELF file validation */

	if (strncmp ((char *)ehdr.e_ident, ELFMAG, 4) != 0) {
	fprintf (stderr, "Input object module not ELF format\n");
	return -1;
	}

    /* 64-bit ELF module not supported (for now) */

	if (ehdr.e_ident[EI_CLASS] != ELFCLASS32) {
	fprintf (stderr, "ELF64 class not supported\n");
	return -1;
	}

    /*
     * Dynamically determine the endianess of the host (in the absence of
     * a compile time macro ala _BYTE_ORDER).  The ELF structures will require
     * byte swapping if the host and target have different byte ordering.
     */

	if (((*(char *)&ix == 0x78) && (ehdr.e_ident[EI_DATA] == ELFDATA2MSB)) ||
		((*(char *)&ix == 0x12) && (ehdr.e_ident[EI_DATA] == ELFDATA2LSB))) {
	swabRequired = 1;
	DBG_PRINT ("Swab required\n");
	}

	swabElfHdr (&ehdr);   /* swap bytes (if required) */

    /* debugging: dump some important ELF header fields */

	DBG_PRINT ("Elf header Magic = %s\n", ehdr.e_ident);
	DBG_PRINT ("Elf header e_type = %d\n", ehdr.e_type);
	DBG_PRINT ("Elf header e_shstrndx = %d\n", ehdr.e_shstrndx);
	DBG_PRINT ("Elf header e_shnum = %d\n", ehdr.e_shnum);

	return 0;
}

/*******************************************************************************
*
* shdrsLoad - load the section headers
*
* RETURNS: 0 on success, -1 on failure
*/

int shdrsLoad(int fd  /* file descriptor of file from which to read */
			  )
{
	size_t    nBytes;    /* number of bytes read from file */
	unsigned  ix;        /* loop index */

	shdr = malloc (ehdr.e_shnum * sizeof (Elf32_Shdr));
	if (shdr == NULL) {
		fprintf (stderr, "No memory for section headers!\n");
		return -1;
		}

    /* Seek to the start of the table of section headers */
	lseek (fd, ehdr.e_shoff, SEEK_SET);

	for (ix = 0; ix < ehdr.e_shnum; ix++) {
	nBytes = read (fd, &shdr[ix], sizeof (Elf32_Shdr));
	if (nBytes != sizeof (Elf32_Shdr)) {
	    fprintf (stderr, "Unable to read entire section header (#%d)\n",
		             ix);
	    return -1;
		    }

	swabElfSectionHdr (&shdr[ix]);   /* swap bytes (if required) */
		}

	return 0;
}

/*******************************************************************************
*
* symTblFind - search the section headers for the symbol table
*
* This routine searches the section headers for the symbol table.  There is
* expected to be only one symbol table in the section headers.
*
* RETURNS: 0 if found, -1 if not
*/

int symTblFind(unsigned *pSymTblOffset,  /* ptr to symbol table offset */
			   unsigned *pSymTblSize     /* ptr to symbol table size */
			   )
{
	unsigned  ix;    /* loop index */

	for (ix = 0; ix < ehdr.e_shnum; ++ix) {
	if (shdr[ix].sh_type == SHT_SYMTAB) {
	    *pSymTblOffset = shdr[ix].sh_offset;
	    *pSymTblSize   = shdr[ix].sh_size;

		    return 0;
	    }
		}

	fprintf (stderr, "Object module missing symbol table!\n");

	return -1;
}

/*******************************************************************************
*
* strTblFind - search the section headers for the string table
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
* RETURNS 0 if found, -1 if not
*/

int strTblFind(unsigned *pStrTblIx  /* ptr to string table's index */
			   )
{
	unsigned  strTblIx = 0xffffffff;
	unsigned  ix;

	for (ix = 0; ix < ehdr.e_shnum; ++ix) {
		if (shdr[ix].sh_type == SHT_STRTAB) {
		    if ((strTblIx == 0xffffffff) ||
		        (ix != ehdr.e_shstrndx)) {
		        strTblIx = ix;
		        }
		    }
		}

	if (strTblIx == 0xffffffff) {
		fprintf (stderr, "Object module missing string table!\n");
		return -1;
		}

	*pStrTblIx = strTblIx;

	return 0;
}

/*******************************************************************************
*
* strTblLoad - load the string table
*
* RETURNS: 0 on success, -1 on failure
*/

int strTblLoad(int fd, /* file descriptor of file from which to read */
			   unsigned strTblIx, /* string table's index */
			   char **ppStringTable /* ptr to ptr to string table */
			   )
{
	char   *pTable;
	int     nBytes;

	DBG_PRINT ("Allocating %d bytes for string table\n",
		       shdr[strTblIx].sh_size);

	pTable = malloc (shdr[strTblIx].sh_size);
	if (pTable == NULL) {
		fprintf (stderr, "No memory for string table!");
		return -1;
		}

	lseek (fd, shdr[strTblIx].sh_offset, SEEK_SET);

	nBytes = read (fd, pTable, shdr[strTblIx].sh_size);
	if (nBytes != shdr[strTblIx].sh_size) {
		free (pTable);
		fprintf (stderr, "Unable to read entire string table!\n");
		return -1;
		}

	*ppStringTable = pTable;

	return 0;
}

/*******************************************************************************
*
* headerPreambleDump - dump the header preamble to the header file
*
* RETURNS: N/A
*/

void headerPreambleDump(FILE *fp,       /* file pointer to which to write */
						char *filename  /* name of the output file */
						)
{
	unsigned hash = 5381;      /* hash value */
	size_t   ix;               /* loop counter */
	char     fileNameHash[20]; /* '_HGUARD_' + 8 character hash + '\0' */

    /* dump header file preamble1[] */

	fprintf (fp, preamble1, filename, filename, filename);

    /*
     * Dump header file preamble2[]. Hash file name into something that
     * is small enough to be a C macro name and does not have invalid
     * characters for a macro name to use as a header guard. The result
     * of the hash should be unique enough for our purposes.
     */

	for (ix = 0; ix < sizeof(filename); ++ix) {
		hash = (hash * 33) + (unsigned int) filename[ix];
		}

	sprintf(fileNameHash, "_HGUARD_%08x", hash);
	fprintf (fp, preamble2, fileNameHash, fileNameHash);
}

/*******************************************************************************
*
* headerAbsoluteSymbolsDump - dump the absolute symbols to the header file
*
* RETURNS: N/A
*/

void headerAbsoluteSymbolsDump(int fd,   /* file descriptor of file from which to read */
							   FILE *fp, /* file pointer to which to write */
							   Elf32_Off symTblOffset, /* symbol table offset */
							   Elf32_Word symTblSize,  /* size of the symbol table */
							   char *pStringTable      /* ptr to the string table */
							   )
{
	Elf32_Sym  aSym;     /* absolute symbol */
	unsigned   ix;       /* loop counter */
	unsigned   numSyms;  /* number of symbols in the symbol table */

    /* context the symbol table: pick out absolute syms */

	numSyms = symTblSize / sizeof(Elf32_Sym);
	lseek (fd, symTblOffset, SEEK_SET);

	for (ix = 0; ix < numSyms; ++ix) {
	/* read in a single symbol structure */

	read (fd, &aSym, sizeof(Elf32_Sym));

	swabElfSym (&aSym);    /* swap bytes (if required) */

	/*
	 * Only generate definitions for global absolute symbols
	 * of the form *_OFFSET
	 */

	if ((aSym.st_shndx == SHN_ABS) &&
	    (ELF_ST_BIND(aSym.st_info) == STB_GLOBAL)) {
	    if ((strstr (&pStringTable[aSym.st_name],
		                 STRUCT_OFF_SUFFIX) != NULL) ||
	        (strstr (&pStringTable[aSym.st_name],
		                 STRUCT_SIZ_SUFFIX) != NULL)) {
		fprintf (fp, "#define\t%s\t0x%X\n",
		                 &pStringTable[aSym.st_name], aSym.st_value);
	        }
	   }
	}
}

/*******************************************************************************
*
* headerPostscriptDump - dump the header postscript to the header file
*
* RETURNS: N/A
*/

void headerPostscriptDump(FILE *fp  /* file pointer to which to write */
						  )
{
	fputs (postscript, fp);
}

/*******************************************************************************
*
* main - entry point for the genOffsetHeader utility
*
* usage: $ genOffsetHeader -i <objectModule> -o <outputHeaderName>
*
* RETURNS: 0 on success, 1 on failure
*/

int main(int argc, char *argv[])
{
	Elf32_Off	symTblOffset = 0;
	Elf32_Word	symTblSize;		/* in bytes */
	char   *pStringTable = NULL;
	char   *inFileName;
	char   *outFileName;
	int		option;
	int		inFd = -1;
	FILE   *outFile = NULL;
	unsigned    strTblIx;

    /* argument parsing */

	if (argc != 5) {
		fprintf (stderr, usage, argv[0]);
	goto errorReturn;
	}

	while ((option = getopt (argc, argv, "i:o:")) != -1) {
	switch (option) {
	    case 'i':
	        inFileName = optarg;
		break;
	    case 'o':
		outFileName = optarg;
		break;
	    default:
		fprintf (stderr, usage, argv[0]);
		goto errorReturn;
	    }
	}

    /* open input object ELF module and output header file */

	inFd = open (inFileName, OPEN_FLAGS);

	if (inFd == -1) {
	fprintf (stderr, "Cannot open input object module");
	goto errorReturn;
	}

	outFile = fopen (outFileName, "w");

	if (outFile == NULL) {
	fprintf (stderr, "Cannot open output header file");
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

	if ((ehdrLoad (inFd) != 0) ||
		(shdrsLoad (inFd) != 0) ||
		(symTblFind (&symTblOffset, &symTblSize) != 0) ||
		(strTblFind (&strTblIx) != 0) ||
		(strTblLoad (inFd, strTblIx, &pStringTable) != 0)) {
		goto errorReturn;
		}


    /*
     * Dump the following to the header file ...
     *  1. Header file preamble
     *  2. Absolute symbols
     *  3. Header file postscript
     */

	headerPreambleDump (outFile, outFileName);
	headerAbsoluteSymbolsDump (inFd, outFile,
		                       symTblOffset, symTblSize, pStringTable);
	headerPostscriptDump (outFile);

    /* done: cleanup */

	close (inFd);
	fclose (outFile);
	free (shdr);
	free (pStringTable);

	return 0;

errorReturn:
	if (inFd != -1) {
		close (inFd);
		}

	if (outFile != NULL) {
		fclose (outFile);
		}

	if (shdr != NULL) {
		free (shdr);
		}

	if (pStringTable != NULL) {
		free (pStringTable);
		}

	return 1;
}
