/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <metal/alloc.h>
#include <openamp/elf_loader.h>

/* Local functions. */

static int elf_loader_get_needed_sections(struct elf_decode_info *elf_info);
static int elf_loader_relocs_specific(struct elf_decode_info *elf_info,
				      Elf32_Shdr * section);
static void *elf_loader_get_entry_point_address(struct elf_decode_info
						*elf_info);
static int elf_loader_relocate_link(struct elf_decode_info *elf_info);
static int elf_loader_seek_and_read(void *firmware, void *destination,
				    Elf32_Off offset, Elf32_Word size);
static int elf_loader_read_headers(void *firmware,
				   struct elf_decode_info *elf_info);
static int elf_loader_load_sections(void *firmware,
				    struct elf_decode_info *elf_info);
static int elf_loader_get_decode_info(void *firmware,
				      struct elf_decode_info *elf_info);
static int elf_loader_reloc_entry(struct elf_decode_info *elf_info,
				  Elf32_Rel * rel_entry);
static Elf32_Addr elf_loader_get_dynamic_symbol_addr(struct elf_decode_info
						     *elf_info, int index);

/**
 * elf_loader_init
 *
 * Initializes ELF loader.
 *
 * @param loader    - pointer to remoteproc loader
 *
 * @return  - 0 if success, error otherwise
 */
int elf_loader_init(struct remoteproc_loader *loader)
{

	/* Initialize loader function table */
	loader->load_firmware = elf_loader_load_remote_firmware;
	loader->retrieve_entry = elf_loader_retrieve_entry_point;
	loader->retrieve_rsc = elf_loader_retrieve_resource_section;
	loader->attach_firmware = elf_loader_attach_firmware;
	loader->detach_firmware = elf_loader_detach_firmware;
	loader->retrieve_load_addr = elf_get_load_address;

	return RPROC_SUCCESS;
}

/**
 * elf_loader_attach_firmware
 *
 * Attaches an ELF firmware to the loader
 *
 * @param loader    - pointer to remoteproc loader
 * @param firmware -  pointer to the firmware start location
 *
 * @return  - 0 if success, error otherwise
 */
int elf_loader_attach_firmware(struct remoteproc_loader *loader, void *firmware)
{

	struct elf_decode_info *elf_info;
	int status;

	/* Allocate memory for decode info structure. */
	elf_info = metal_allocate_memory(sizeof(struct elf_decode_info));

	if (!elf_info) {
		return RPROC_ERR_NO_MEM;
	}

	/* Clear the ELF decode struct. */
	memset(elf_info, 0, sizeof(struct elf_decode_info));

	/* Get the essential information to decode the ELF. */
	status = elf_loader_get_decode_info(firmware, elf_info);

	if (status) {
		/* Free memory. */
		metal_free_memory(elf_info);
		return status;
	}

	elf_info->firmware = firmware;
	loader->fw_decode_info = elf_info;

	return status;
}

/**
 * elf_loader_detach_firmware
 *
 * Detaches ELF firmware from the loader
 *
 * @param loader - pointer to remoteproc loader
 *
 * @return  - 0 if success, error otherwise
 */
int elf_loader_detach_firmware(struct remoteproc_loader *loader)
{

	struct elf_decode_info *elf_info =
	    (struct elf_decode_info *)loader->fw_decode_info;
	if (elf_info) {
		/* Free memory. */
		metal_free_memory(elf_info->shstrtab);
		metal_free_memory(elf_info->section_headers_start);
		metal_free_memory(elf_info);
	}

	return RPROC_SUCCESS;
}

/**
 * elf_loader_retrieve_entry_point
 *
 * Retrieves the ELF entrypoint.
 *
 * @param loader - pointer to remoteproc loader
 *
 * @return  - entrypoint
 */
void *elf_loader_retrieve_entry_point(struct remoteproc_loader *loader)
{

	return elf_loader_get_entry_point_address((struct elf_decode_info *)
						  loader->fw_decode_info);
}

/**
 * elf_loader_retrieve_resource_section
 *
 * Retrieves the resource section.
 *
 * @param loader - pointer to remoteproc loader
 * @param size   - pointer to contain the size of the section
 *
 * @return  - pointer to resource section
 */
void *elf_loader_retrieve_resource_section(struct remoteproc_loader *loader,
					   unsigned int *size)
{

	Elf32_Shdr *rsc_header;
	void *resource_section = NULL;
	struct elf_decode_info *elf_info =
	    (struct elf_decode_info *)loader->fw_decode_info;

	if (elf_info->rsc) {
		/* Retrieve resource section header. */
		rsc_header = elf_info->rsc;
		/* Retrieve resource section size. */
		*size = rsc_header->sh_size;

		/* Locate the start of resource section. */
		resource_section = (void *)((uintptr_t)elf_info->firmware
					    + rsc_header->sh_offset);
	}

	/* Return the address of resource section. */
	return resource_section;
}

/**
 * elf_loader_load_remote_firmware
 *
 * Loads the ELF firmware.
 *
 * @param loader - pointer to remoteproc loader
 *
 * @return  - 0 if success, error otherwise
 */
int elf_loader_load_remote_firmware(struct remoteproc_loader *loader)
{

	struct elf_decode_info *elf_info =
	    (struct elf_decode_info *)loader->fw_decode_info;
	int status;

	/* Load ELF sections. */
	status = elf_loader_load_sections(elf_info->firmware, elf_info);

	if (!status) {

		/* Perform dynamic relocations if needed. */
		status = elf_loader_relocate_link(elf_info);
	}

	return status;
}

/**
 * elf_get_load_address
 *
 * Provides firmware load address.
 *
 * @param loader - pointer to remoteproc loader
 *
 * @return  - load address pointer
 */
void *elf_get_load_address(struct remoteproc_loader *loader)
{

	struct elf_decode_info *elf_info =
	    (struct elf_decode_info *)loader->fw_decode_info;
	int status = 0;
	Elf32_Shdr *current = (Elf32_Shdr *) (elf_info->section_headers_start);

	/* Traverse all sections except the reserved null section. */
	int section_count = elf_info->elf_header.e_shnum - 1;
	while ((section_count > 0) && (status == 0)) {
		/* Compute the pointer to section header. */
		current = (Elf32_Shdr *) (((unsigned char *)current)
					  + elf_info->elf_header.e_shentsize);
		/* Get the name of current section. */
		char *current_name = elf_info->shstrtab + current->sh_name;
		if (!strcmp(current_name, ".text")) {
			return ((void *)(current->sh_addr));
		}
		/* Move to the next section. */
		section_count--;
	}

	return (RPROC_ERR_PTR);
}

/**
 * elf_loader_get_needed_sections
 *
 * Retrieves the sections we need during the load and link from the
 * section headers list.
 *
 * @param elf_info  - ELF object decode info container.
 *
 * @return- Pointer to the ELF section header.
 */

static int elf_loader_get_needed_sections(struct elf_decode_info *elf_info)
{
	Elf32_Shdr *current = (Elf32_Shdr *) (elf_info->section_headers_start);

	/* We are interested in the following sections:
	   .dynsym
	   .dynstr
	   .rel.plt
	   .rel.dyn
	 */
	int sections_to_find = 5;

	/* Search for sections but skip the reserved null section. */

	int section_count = elf_info->elf_header.e_shnum - 1;
	while ((section_count > 0) && (sections_to_find > 0)) {
		/* Compute the section header pointer. */
		current = (Elf32_Shdr *) (((unsigned char *)current)
					  + elf_info->elf_header.e_shentsize);

		/* Get the name of current section. */
		char *current_name = elf_info->shstrtab + current->sh_name;

		/* Proceed if the section is allocatable and is not executable. */
		if ((current->sh_flags & SHF_ALLOC)
		    && !(current->sh_flags & SHF_EXECINSTR)) {
			/* Check for '.dynsym' or '.dynstr' or '.rel.plt' or '.rel.dyn'. */
			if (*current_name == '.') {
				current_name++;

				/* Check for '.dynsym' or 'dynstr'. */
				if (*current_name == 'd') {
					current_name++;

					/* Check for '.dynsym'. */
					if (strncmp(current_name, "ynsym", 5) == 0) {
						elf_info->dynsym = current;
						sections_to_find--;
					}

					/* Check for '.dynstr'. */
					else if (strncmp(current_name, "ynstr", 5) == 0) {
						elf_info->dynstr = current;
						sections_to_find--;
					}
				}

				/* Check for '.rel.plt' or '.rel.dyn'. */
				else if (*current_name == 'r') {
					current_name++;

					/* Check for '.rel.plt'. */
					if (strncmp(current_name, "el.plt", 6) == 0) {
						elf_info->rel_plt = current;
						sections_to_find--;
					}

					/* Check for '.rel.dyn'. */
					else if (strncmp(current_name, "el.dyn", 6) == 0) {
						elf_info->rel_dyn = current;
						sections_to_find--;
					}

					/* Check for '.resource_table'. */
					else if (strncmp(current_name, "esource_table", 13)
						 == 0) {
						elf_info->rsc = current;
						sections_to_find--;
					}
				}
			}
		}

		/* Move to the next section. */
		section_count--;
	}

	/* Return remaining sections section. */
	return (sections_to_find);
}

/**
 * elf_loader_relocs_specific
 *
 * Processes the relocations contained in the specified section.
 *
 * @param elf_info - elf decoding information.
 * @param section  - header of the specified relocation section.
 *
 * @return  - 0 if success, error otherwise
 */
static int elf_loader_relocs_specific(struct elf_decode_info *elf_info,
				      Elf32_Shdr * section)
{

	unsigned char *section_load_addr = (unsigned char *)section->sh_addr;
	int status = 0;
	unsigned int i;

	/* Check the section type. */
	if (section->sh_type == SHT_REL) {
		/* Traverse the list of relocation entries contained in the section. */
		for (i = 0; (i < section->sh_size) && (status == 0);
		     i += section->sh_entsize) {
			/* Compute the relocation entry address. */
			Elf32_Rel *rel_entry =
			    (Elf32_Rel *) (section_load_addr + i);

			/* Process the relocation entry. */
			status = elf_loader_reloc_entry(elf_info, rel_entry);
		}
	}

	/* Return status to caller. */
	return (status);
}

/**
 * elf_loader_get_entry_point_address
 *
 * Retrieves the entry point address from the specified ELF object.
 *
 * @param elf_info       - elf object decode info container.
 * @param runtime_buffer - buffer containing ELF sections which are
 *                         part of runtime.
 *
 * @return - entry point address of the specified ELF object.
 */
static void *elf_loader_get_entry_point_address(struct elf_decode_info
						*elf_info)
{
	return ((void *)elf_info->elf_header.e_entry);
}

/**
 * elf_loader_relocate_link
 *
 * Relocates and links the given ELF object.
 *
 * @param elf_info       - elf object decode info container.

 *
 * @return  - 0 if success, error otherwise
 */

static int elf_loader_relocate_link(struct elf_decode_info *elf_info)
{
	int status = 0;

	/* Check of .rel.dyn section exists in the ELF. */
	if (elf_info->rel_dyn) {
		/* Relocate and link .rel.dyn section. */
		status =
		    elf_loader_relocs_specific(elf_info, elf_info->rel_dyn);
	}

	/* Proceed to check if .rel.plt section exists, if no error encountered yet. */
	if (status == 0 && elf_info->rel_plt) {
		/* Relocate and link .rel.plt section. */
		status =
		    elf_loader_relocs_specific(elf_info, elf_info->rel_plt);
	}

	/* Return status to caller */
	return (status);
}

/**
 * elf_loader_seek_and_read
 *
 * Seeks to the specified offset in the given file and reads the data
 * into the specified destination location.
 *
 * @param firmware     - firmware to read from.
 * @param destination - Location into which the data should be read.
 * @param offset      - Offset to seek in the file.
 * @param size        - Size of the data to read.

 *
 * @return  - 0 if success, error otherwise
 */

static int elf_loader_seek_and_read(void *firmware, void *destination,
				    Elf32_Off offset, Elf32_Word size)
{
	char *src = (char *)firmware;

	/* Seek to the specified offset. */
	src = src + offset;

	/* Read the data. */
	memcpy((char *)destination, src, size);

	/* Return status to caller. */
	return (0);
}

/**
 * elf_loader_read_headers
 *
 * Reads the ELF headers (ELF header, section headers and the section
 * headers string table) essential to access further information from
 * the file containing the ELF object.
 *
 * @param firmware    - firmware to read from.
 * @param elf_info - ELF object decode info container.
 *
 * @return  - 0 if success, error otherwise
 */
static int elf_loader_read_headers(void *firmware,
				   struct elf_decode_info *elf_info)
{
	int status = 0;
	unsigned int section_count;

	/* Read the ELF header. */
	status = elf_loader_seek_and_read(firmware, &(elf_info->elf_header), 0,
					  sizeof(Elf32_Ehdr));

	/* Ensure the read was successful. */
	if (!status) {
		/* Get section count from the ELF header. */
		section_count = elf_info->elf_header.e_shnum;

		/* Allocate memory to read in the section headers. */
		elf_info->section_headers_start = metal_allocate_memory(section_count * elf_info->elf_header.e_shentsize);

		/* Check if the allocation was successful. */
		if (elf_info->section_headers_start) {
			/* Read the section headers list. */
			status = elf_loader_seek_and_read(firmware,
							  elf_info->
							  section_headers_start,
							  elf_info->elf_header.
							  e_shoff,
							  section_count *
							  elf_info->elf_header.
							  e_shentsize);

			/* Ensure the read was successful. */
			if (!status) {
				/* Compute the pointer to section header string table section. */
				Elf32_Shdr *section_header_string_table =
				    (Elf32_Shdr *) (elf_info->
						    section_headers_start +
						    elf_info->elf_header.
						    e_shstrndx *
						    elf_info->elf_header.
						    e_shentsize);

				/* Allocate the memory for section header string table. */
				elf_info->shstrtab = metal_allocate_memory(section_header_string_table->sh_size);

				/* Ensure the allocation was successful. */
				if (elf_info->shstrtab) {
					/* Read the section headers string table. */
					status =
					    elf_loader_seek_and_read(firmware,
								     elf_info->
								     shstrtab,
								     section_header_string_table->
								     sh_offset,
								     section_header_string_table->
								     sh_size);
				}
			}
		}
	}

	/* Return status to caller. */
	return (status);
}

/**
 * elf_loader_file_read_sections
 *
 * Reads the ELF section contents from the specified file containing
 * the ELF object.
 *
 * @param firmware - firmware to read from.
 * @param elf_info - ELF object decode info container.
 *
 * @return  - 0 if success, error otherwise
 */
static int elf_loader_load_sections(void *firmware,
				    struct elf_decode_info *elf_info)
{
	int status = 0;
	Elf32_Shdr *current = (Elf32_Shdr *) (elf_info->section_headers_start);

	/* Traverse all sections except the reserved null section. */
	int section_count = elf_info->elf_header.e_shnum - 1;
	while ((section_count > 0) && (status == 0)) {
		/* Compute the pointer to section header. */
		current = (Elf32_Shdr *) (((unsigned char *)current)
					  + elf_info->elf_header.e_shentsize);

		/* Make sure the section can be allocated and is not empty. */
		if ((current->sh_flags & SHF_ALLOC) && (current->sh_size)) {
			char *destination = NULL;

			/* Check if the section is part of runtime and is not section with
			 * no-load attributes such as BSS or heap. */
			if ((current->sh_type & SHT_NOBITS) == 0) {
				/* Compute the destination address where the section should
				 * be copied. */
				destination = (char *)(current->sh_addr);
				status =
				    elf_loader_seek_and_read(firmware,
							     destination,
							     current->sh_offset,
							     current->sh_size);
			}
		}

		/* Move to the next section. */
		section_count--;
	}

	/* Return status to caller. */
	return (status);
}

/**
 * elf_loader_get_decode_info
 *
 * Retrieves the information necessary to decode the ELF object for
 * loading, relocating and linking.
 *
 * @param firmware - firmware to read from.
 * @param elf_info - ELF object decode info container.
 *
 * @return  - 0 if success, error otherwise
 */
static int elf_loader_get_decode_info(void *firmware,
				      struct elf_decode_info *elf_info)
{
	int status;

	/* Read the ELF headers (ELF header and section headers including
	 * the section header string table). */
	status = elf_loader_read_headers(firmware, elf_info);

	/* Ensure that ELF headers were read successfully. */
	if (!status) {
		/* Retrieve the sections required for load. */
		elf_loader_get_needed_sections(elf_info);

	}

	/* Return status to caller. */
	return (status);
}

/**
 * elf_loader_get_dynamic_symbol_addr
 *
 * Retrieves the (relocatable) address of the symbol specified as
 * index from the given ELF object.
 *
 * @param elf_info - ELF object decode info container.
 * @param index    - Index of the desired symbol in the dynamic symbol table.
 *
 * @return - Address of the specified symbol.
 */
static Elf32_Addr elf_loader_get_dynamic_symbol_addr(struct elf_decode_info
						     *elf_info, int index)
{
	Elf32_Sym *symbol_entry = (Elf32_Sym *) (elf_info->dynsym_addr
						 +
						 index *
						 elf_info->dynsym->sh_entsize);

	/* Return the symbol address. */
	return (symbol_entry->st_value);
}

/**
 * elf_loader_reloc_entry
 *
 * Processes the specified relocation entry. It handles the relocation
 * and linking both cases.
 *
 *
 * @param elf_info - ELF object decode info container.
 *
 * @return  - 0 if success, error otherwise
 */
static int elf_loader_reloc_entry(struct elf_decode_info *elf_info,
				  Elf32_Rel * rel_entry)
{
	unsigned char rel_type = ELF32_R_TYPE(rel_entry->r_info);
	int status = 0;

	switch (rel_type) {
	case R_ARM_ABS32:	/* 0x02 */
		{
			Elf32_Addr sym_addr =
			    elf_loader_get_dynamic_symbol_addr(elf_info,
							       ELF32_R_SYM
							       (rel_entry->
								r_info));

			if (sym_addr) {
				*((unsigned int *)(rel_entry->r_offset)) =
				    (unsigned int)sym_addr;
				break;
			}
		}

		break;

	default:
		break;
	}

	return status;
}
