/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <metal/alloc.h>
#include <metal/log.h>
#include <openamp/elf_loader.h>
#include <openamp/remoteproc.h>

static int elf_is_64(const void *elf_info)
{
	const unsigned char *tmp = elf_info;

	if (tmp[EI_CLASS] == ELFCLASS64)
		return 1;
	else
		return 0;
}

static size_t elf_ehdr_size(const void *elf_info)
{
	if (elf_info == NULL)
		return sizeof(Elf64_Ehdr);
	else if (elf_is_64(elf_info) != 0)
		return sizeof(Elf64_Ehdr);
	else
		return sizeof(Elf32_Ehdr);
}

static size_t elf_phoff(const void *elf_info)
{
	if (elf_is_64(elf_info) == 0) {
		const Elf32_Ehdr *ehdr = elf_info;

		return ehdr->e_phoff;
	} else {
		const Elf64_Ehdr *ehdr = elf_info;

		return ehdr->e_phoff;
	}
}

static size_t elf_phentsize(const void *elf_info)
{
	if (elf_is_64(elf_info) == 0) {
		const Elf32_Ehdr *ehdr = elf_info;

		return ehdr->e_phentsize;
	} else {
		const Elf64_Ehdr *ehdr = elf_info;

		return ehdr->e_phentsize;
	}
}

static int elf_phnum(const void *elf_info)
{
	if (elf_is_64(elf_info) == 0) {
		const Elf32_Ehdr *ehdr = elf_info;

		return ehdr->e_phnum;
	} else {
		const Elf64_Ehdr *ehdr = elf_info;

		return ehdr->e_phnum;
	}
}

static size_t elf_shoff(const void *elf_info)
{
	if (elf_is_64(elf_info) == 0) {
		const Elf32_Ehdr *ehdr = elf_info;

		return ehdr->e_shoff;
	} else {
		const Elf64_Ehdr *ehdr = elf_info;

		return ehdr->e_shoff;
	}
}

static size_t elf_shentsize(const void *elf_info)
{
	if (elf_is_64(elf_info) == 0) {
		const Elf32_Ehdr *ehdr = elf_info;

		return ehdr->e_shentsize;
	} else {
		const Elf64_Ehdr *ehdr = elf_info;

		return ehdr->e_shentsize;
	}
}

static int elf_shnum(const void *elf_info)
{
	if (elf_is_64(elf_info) == 0) {
		const Elf32_Ehdr *ehdr = elf_info;

		return ehdr->e_shnum;
	} else {
		const Elf64_Ehdr *ehdr = elf_info;

		return ehdr->e_shnum;
	}
}

static int elf_shstrndx(const void *elf_info)
{
	if (elf_is_64(elf_info) == 0) {
		const Elf32_Ehdr *ehdr = elf_info;

		return ehdr->e_shstrndx;
	} else {
		const Elf64_Ehdr *ehdr = elf_info;

		return ehdr->e_shstrndx;
	}
}

static void *elf_phtable_ptr(void *elf_info)
{
	if (elf_is_64(elf_info) == 0) {
		struct elf32_info *einfo = elf_info;

		return (void *)&einfo->phdrs;
	} else {
		struct elf64_info *einfo = elf_info;

		return (void *)&einfo->phdrs;
	}
}

static void *elf_shtable_ptr(void *elf_info)
{
	if (elf_is_64(elf_info) == 0) {
		struct elf32_info *einfo = elf_info;

		return (void *)(&einfo->shdrs);
	} else {
		struct elf64_info *einfo = elf_info;

		return (void *)(&einfo->shdrs);
	}
}

static void **elf_shstrtab_ptr(void *elf_info)
{
	if (elf_is_64(elf_info) == 0) {
		struct elf32_info *einfo = elf_info;

		return &einfo->shstrtab;
	} else {
		struct elf64_info *einfo = elf_info;

		return &einfo->shstrtab;
	}
}

static unsigned int *elf_load_state(void *elf_info)
{
	if (elf_is_64(elf_info) == 0) {
		struct elf32_info *einfo = elf_info;

		return &einfo->load_state;
	} else {
		struct elf64_info *einfo = elf_info;

		return &einfo->load_state;
	}
}

static void elf_parse_segment(void *elf_info, const void *elf_phdr,
			      unsigned int *p_type, size_t *p_offset,
			      metal_phys_addr_t *p_vaddr,
			      metal_phys_addr_t *p_paddr,
			      size_t *p_filesz, size_t *p_memsz)
{
	if (elf_is_64(elf_info) == 0) {
		const Elf32_Phdr *phdr = elf_phdr;

		if (p_type != NULL)
			*p_type = (unsigned int)phdr->p_type;
		if (p_offset != NULL)
			*p_offset = (size_t)phdr->p_offset;
		if (p_vaddr != NULL)
			*p_vaddr = (metal_phys_addr_t)phdr->p_vaddr;
		if (p_paddr != NULL)
			*p_paddr = (metal_phys_addr_t)phdr->p_paddr;
		if (p_filesz != NULL)
			*p_filesz = (size_t)phdr->p_filesz;
		if (p_memsz != NULL)
			*p_memsz = (size_t)phdr->p_memsz;
	} else {
		const Elf64_Phdr *phdr = elf_phdr;

		if (p_type != NULL)
			*p_type = (unsigned int)phdr->p_type;
		if (p_offset != NULL)
			*p_offset = (size_t)phdr->p_offset;
		if (p_vaddr != NULL)
		if (p_vaddr != NULL)
			*p_vaddr = (metal_phys_addr_t)phdr->p_vaddr;
		if (p_paddr != NULL)
			*p_paddr = (metal_phys_addr_t)phdr->p_paddr;
		if (p_filesz != NULL)
			*p_filesz = (size_t)phdr->p_filesz;
		if (p_memsz != NULL)
			*p_memsz = (size_t)phdr->p_memsz;
	}
}

static const void *elf_get_segment_from_index(void *elf_info, int index)
{
	if (elf_is_64(elf_info) == 0) {
		const struct elf32_info *einfo = elf_info;
		const Elf32_Ehdr *ehdr = &einfo->ehdr;
		const Elf32_Phdr *phdrs = einfo->phdrs;

		if (phdrs == NULL)
			return NULL;
		if (index < 0 || index > ehdr->e_phnum)
			return NULL;
		return &phdrs[index];
	} else {
		const struct elf64_info *einfo = elf_info;
		const Elf64_Ehdr *ehdr = &einfo->ehdr;
		const Elf64_Phdr *phdrs = einfo->phdrs;

		if (phdrs == NULL)
			return NULL;
		if (index < 0 || index > ehdr->e_phnum)
			return NULL;
		return &phdrs[index];
	}
}

static void *elf_get_section_from_name(void *elf_info, const char *name)
{
	unsigned int i;
	const char *name_table;

	if (elf_is_64(elf_info) == 0) {
		struct elf32_info *einfo = elf_info;
		Elf32_Ehdr *ehdr = &einfo->ehdr;
		Elf32_Shdr *shdr = einfo->shdrs;

		name_table = einfo->shstrtab;
		if (shdr == NULL || name_table == NULL)
			return NULL;
		for (i = 0; i < ehdr->e_shnum; i++, shdr++) {
			if (strcmp(name, name_table + shdr->sh_name))
				continue;
			else
				return shdr;
		}
	} else {
		struct elf64_info *einfo = elf_info;
		Elf64_Ehdr *ehdr = &einfo->ehdr;
		Elf64_Shdr *shdr = einfo->shdrs;

		name_table = einfo->shstrtab;
		if (shdr == NULL || name_table == NULL)
			return NULL;
		for (i = 0; i < ehdr->e_shnum; i++, shdr++) {
			if (strcmp(name, name_table + shdr->sh_name))
				continue;
			else
				return shdr;
		}
	}
	return NULL;
}

static void *elf_get_section_from_index(void *elf_info, int index)
{
	if (elf_is_64(elf_info) == 0) {
		struct elf32_info *einfo = elf_info;
		Elf32_Ehdr *ehdr = &einfo->ehdr;
		Elf32_Shdr *shdr = einfo->shdrs;

		if (shdr == NULL)
			return NULL;
		if (index > ehdr->e_shnum)
			return NULL;
		return &einfo->shdrs[index];
	} else {
		struct elf64_info *einfo = elf_info;
		Elf64_Ehdr *ehdr = &einfo->ehdr;
		Elf64_Shdr *shdr = einfo->shdrs;

		if (shdr == NULL)
			return NULL;
		if (index > ehdr->e_shnum)
			return NULL;
		return &einfo->shdrs[index];
	}
}

static void elf_parse_section(void *elf_info, void *elf_shdr,
			      unsigned int *sh_type, unsigned int *sh_flags,
			      metal_phys_addr_t *sh_addr,
			      size_t *sh_offset, size_t *sh_size,
			      unsigned int *sh_link, unsigned int *sh_info,
			      unsigned int *sh_addralign,
			      size_t *sh_entsize)
{
	if (elf_is_64(elf_info) == 0) {
		Elf32_Shdr *shdr = elf_shdr;

		if (sh_type != NULL)
			*sh_type = shdr->sh_type;
		if (sh_flags != NULL)
			*sh_flags = shdr->sh_flags;
		if (sh_addr != NULL)
			*sh_addr = (metal_phys_addr_t)shdr->sh_addr;
		if (sh_offset != NULL)
			*sh_offset = shdr->sh_offset;
		if (sh_size != NULL)
			*sh_size = shdr->sh_size;
		if (sh_link != NULL)
			*sh_link = shdr->sh_link;
		if (sh_info != NULL)
			*sh_info = shdr->sh_info;
		if (sh_addralign != NULL)
			*sh_addralign = shdr->sh_addralign;
		if (sh_entsize != NULL)
			*sh_entsize = shdr->sh_entsize;
	} else {
		Elf64_Shdr *shdr = elf_shdr;

		if (sh_type != NULL)
			*sh_type = shdr->sh_type;
		if (sh_flags != NULL)
			*sh_flags = shdr->sh_flags;
		if (sh_addr != NULL)
			*sh_addr = (metal_phys_addr_t)(shdr->sh_addr &
				   (metal_phys_addr_t)(-1));
		if (sh_offset != NULL)
			*sh_offset = shdr->sh_offset;
		if (sh_size != NULL)
			*sh_size = shdr->sh_size;
		if (sh_link != NULL)
			*sh_link = shdr->sh_link;
		if (sh_info != NULL)
			*sh_info = shdr->sh_info;
		if (sh_addralign != NULL)
			*sh_addralign = shdr->sh_addralign;
		if (sh_entsize != NULL)
			*sh_entsize = shdr->sh_entsize;
	}
}

static const void *elf_next_load_segment(void *elf_info, int *nseg,
				   metal_phys_addr_t *da,
				   size_t *noffset, size_t *nfsize,
				   size_t *nmsize)
{
	const void *phdr;
	unsigned int p_type = PT_NULL;

	if (elf_info == NULL || nseg == NULL)
		return NULL;
	while(p_type != PT_LOAD) {
		phdr = elf_get_segment_from_index(elf_info, *nseg);
		if (phdr == NULL)
			return NULL;
		elf_parse_segment(elf_info, phdr, &p_type, noffset,
				  da, NULL, nfsize, nmsize);
		*nseg = *nseg + 1;
	}
	return phdr;
}

static size_t elf_info_size(const void *img_data)
{
	if (elf_is_64(img_data) == 0)
		return sizeof(struct elf32_info);
	else
		return sizeof(struct elf64_info);
}

int elf_identify(const void *img_data, size_t len)
{
	if (len < SELFMAG || img_data == NULL)
		return -RPROC_EINVAL;
	if (memcmp(img_data, ELFMAG, SELFMAG) != 0)
		return -RPROC_EINVAL;
	else
		return 0;
}

int elf_load_header(const void *img_data, size_t offset, size_t len,
		    void **img_info, int last_load_state,
		    size_t *noffset, size_t *nlen)
{
	unsigned int *load_state;

	metal_assert(noffset != NULL);
	metal_assert(nlen != NULL);
	/* Get ELF header */
	if (last_load_state == ELF_STATE_INIT) {
		size_t tmpsize;

		metal_log(METAL_LOG_DEBUG, "Loading ELF headering\r\n");
		tmpsize = elf_ehdr_size(img_data);
		if (len < tmpsize) {
			*noffset = 0;
			*nlen = tmpsize;
			return ELF_STATE_INIT;
		} else {
			size_t infosize = elf_info_size(img_data);

			if (*img_info == NULL) {
				*img_info = metal_allocate_memory(infosize);
				if (*img_info == NULL)
					return -ENOMEM;
				memset(*img_info, 0, infosize);
			}
			memcpy(*img_info, img_data, tmpsize);
			load_state = elf_load_state(*img_info);
			*load_state = ELF_STATE_WAIT_FOR_PHDRS;
			last_load_state = ELF_STATE_WAIT_FOR_PHDRS;
		}
	}
	metal_assert(*img_info != NULL);
	load_state = elf_load_state(*img_info);
	if (last_load_state != (int)*load_state)
		return -RPROC_EINVAL;
	/* Get ELF program headers */
	if (*load_state == ELF_STATE_WAIT_FOR_PHDRS) {
		size_t phdrs_size;
		size_t phdrs_offset;
		char **phdrs;
		const void *img_phdrs;

		metal_log(METAL_LOG_DEBUG, "Loading ELF program header.\r\n");
		phdrs_offset = elf_phoff(*img_info);
		phdrs_size = elf_phnum(*img_info) * elf_phentsize(*img_info);
		if (offset > phdrs_offset ||
		    offset + len < phdrs_offset + phdrs_size) {
			*noffset = phdrs_offset;
			*nlen = phdrs_size;
			return (int)*load_state;
		}
		/* caculate the programs headers offset to the image_data */
		phdrs_offset -= offset;
		img_phdrs = (const void *)
			    ((const char *)img_data + phdrs_offset);
		phdrs = (char **)elf_phtable_ptr(*img_info);
		(*phdrs) = metal_allocate_memory(phdrs_size);
		if (*phdrs == NULL)
			return -ENOMEM;
		memcpy((void *)(*phdrs), img_phdrs, phdrs_size);
		*load_state = ELF_STATE_WAIT_FOR_SHDRS |
			       RPROC_LOADER_READY_TO_LOAD;
	}
	/* Get ELF Section Headers */
	if ((*load_state & ELF_STATE_WAIT_FOR_SHDRS) != 0) {
		size_t shdrs_size;
		size_t shdrs_offset;
		char **shdrs;
		const void *img_shdrs;

		metal_log(METAL_LOG_DEBUG, "Loading ELF section header.\r\n");
		shdrs_offset = elf_shoff(*img_info);
		if (elf_shnum(*img_info) == 0) {
			*load_state = (*load_state & (~ELF_STATE_MASK)) |
				       ELF_STATE_HDRS_COMPLETE;
		       *nlen = 0;
			return (int)*load_state;
		}
		shdrs_size = elf_shnum(*img_info) * elf_shentsize(*img_info);
		if (offset > shdrs_offset ||
		    offset + len < shdrs_offset + shdrs_size) {
			*noffset = shdrs_offset;
			*nlen = shdrs_size;
			return (int)*load_state;
		}
		/* caculate the sections headers offset to the image_data */
		shdrs_offset -= offset;
		img_shdrs = (const void *)
			    ((const char *)img_data + shdrs_offset);
		shdrs = (char **)elf_shtable_ptr(*img_info);
		(*shdrs) = metal_allocate_memory(shdrs_size);
		if (*shdrs == NULL)
			return -ENOMEM;
		memcpy((void *)*shdrs, img_shdrs, shdrs_size);
		*load_state = (*load_state & (~ELF_STATE_MASK)) |
			       ELF_STATE_WAIT_FOR_SHSTRTAB;
		metal_log(METAL_LOG_DEBUG,
			  "Loading ELF section header complete.\r\n");
	}
	/* Get ELF SHSTRTAB section */
	if ((*load_state & ELF_STATE_WAIT_FOR_SHSTRTAB) != 0) {
		size_t shstrtab_size;
		size_t shstrtab_offset;
		int shstrndx;
		void *shdr;
		void **shstrtab;

		metal_log(METAL_LOG_DEBUG, "Loading ELF shstrtab.\r\n");
		shstrndx = elf_shstrndx(*img_info);
		shdr = elf_get_section_from_index(*img_info, shstrndx);
		if (shdr == NULL)
			return -RPROC_EINVAL;
		elf_parse_section(*img_info, shdr, NULL, NULL,
				  NULL, &shstrtab_offset,
				  &shstrtab_size, NULL, NULL,
				  NULL, NULL);
		if (offset > shstrtab_offset ||
		    offset + len < shstrtab_offset + shstrtab_size) {
			*noffset = shstrtab_offset;
			*nlen = shstrtab_size;
			return (int)*load_state;
		}
		/* Caculate shstrtab section offset to the input image data */
		shstrtab_offset -= offset;
		shstrtab = elf_shstrtab_ptr(*img_info);
		*shstrtab = metal_allocate_memory(shstrtab_size);
		if (*shstrtab == NULL)
			return -ENOMEM;
		memcpy(*shstrtab,
		       (const void *)((const char *)img_data + shstrtab_offset),
		       shstrtab_size);
		*load_state = (*load_state & (~ELF_STATE_MASK)) |
			       ELF_STATE_HDRS_COMPLETE;
		*nlen = 0;
		return *load_state;
	}
	return last_load_state;
}

int elf_load(struct remoteproc *rproc,
	     const void *img_data, size_t offset, size_t len,
	     void **img_info, int last_load_state,
	     metal_phys_addr_t *da,
	     size_t *noffset, size_t *nlen,
	     unsigned char *padding, size_t *nmemsize)
{
	unsigned int *load_state;
	const void *phdr;

	(void)rproc;
	metal_assert(da != NULL);
	metal_assert(noffset != NULL);
	metal_assert(nlen != NULL);
	if ((last_load_state & RPROC_LOADER_MASK) == RPROC_LOADER_NOT_READY) {
		metal_log(METAL_LOG_DEBUG,
			  "%s, needs to load header first\r\n");
		last_load_state = elf_load_header(img_data, offset, len,
						  img_info, last_load_state,
						  noffset, nlen);
		if ((last_load_state & RPROC_LOADER_MASK) ==
		    RPROC_LOADER_NOT_READY) {
			*da = RPROC_LOAD_ANYADDR;
			return last_load_state;
		}
	}
	metal_assert(img_info != NULL && *img_info != NULL);
	load_state = elf_load_state(*img_info);
	/* For ELF, segment padding value is 0 */
	if (padding != NULL)
		*padding = 0;
	if ((*load_state & RPROC_LOADER_READY_TO_LOAD) != 0) {
		int nsegment;
		size_t nsegmsize = 0;
		size_t nsize = 0;
		int phnums = 0;

		nsegment = (int)(*load_state & ELF_NEXT_SEGMENT_MASK);
		phdr = elf_next_load_segment(*img_info, &nsegment, da,
					     noffset, &nsize, &nsegmsize);
		if (phdr == NULL) {
			metal_log(METAL_LOG_DEBUG, "cannot find more segement\r\n");
			*load_state = (*load_state & (~ELF_NEXT_SEGMENT_MASK)) |
				      (unsigned int)(nsegment & ELF_NEXT_SEGMENT_MASK);
			return *load_state;
		}
		*nlen = nsize;
		*nmemsize = nsegmsize;
		phnums = elf_phnum(*img_info);
		metal_log(METAL_LOG_DEBUG, "segment: %d, total segs %d\r\n",
			  nsegment, phnums);
		if (nsegment == elf_phnum(*img_info)) {
			*load_state = (*load_state & (~RPROC_LOADER_MASK)) |
				      RPROC_LOADER_POST_DATA_LOAD;
		}
		*load_state = (*load_state & (~ELF_NEXT_SEGMENT_MASK)) |
			      (unsigned int)(nsegment & ELF_NEXT_SEGMENT_MASK);
	} else if ((*load_state & RPROC_LOADER_POST_DATA_LOAD) != 0) {
		if ((*load_state & ELF_STATE_HDRS_COMPLETE) == 0) {
			last_load_state = elf_load_header(img_data, offset,
							  len, img_info,
							  last_load_state,
							  noffset, nlen);
			if (last_load_state < 0)
				return last_load_state;
			if ((last_load_state & ELF_STATE_HDRS_COMPLETE) != 0) {
				*load_state = (*load_state &
						(~RPROC_LOADER_MASK)) |
						RPROC_LOADER_LOAD_COMPLETE;
				*nlen = 0;
			}
			*da = RPROC_LOAD_ANYADDR;
		} else {
		/* TODO: will handle relocate later */
			*nlen = 0;
			*load_state = (*load_state &
					(~RPROC_LOADER_MASK)) |
					RPROC_LOADER_LOAD_COMPLETE;
		}
	}
	return *load_state;
}

void elf_release(void *img_info)
{
	if (img_info == NULL)
		return;
	if (elf_is_64(img_info) == 0) {
		struct elf32_info *elf_info = img_info;

		if (elf_info->phdrs != NULL)
			metal_free_memory(elf_info->phdrs);
		if (elf_info->shdrs != NULL)
			metal_free_memory(elf_info->shdrs);
		if (elf_info->shstrtab != NULL)
			metal_free_memory(elf_info->shstrtab);
		metal_free_memory(img_info);

	} else {
		struct elf64_info *elf_info = img_info;

		if (elf_info->phdrs != NULL)
			metal_free_memory(elf_info->phdrs);
		if (elf_info->shdrs != NULL)
			metal_free_memory(elf_info->shdrs);
		if (elf_info->shstrtab != NULL)
			metal_free_memory(elf_info->shstrtab);
		metal_free_memory(img_info);
	}
}

metal_phys_addr_t elf_get_entry(void *elf_info)
{
	if (!elf_info)
		return METAL_BAD_PHYS;

	if (elf_is_64(elf_info) == 0) {
		Elf32_Ehdr *elf_ehdr = (Elf32_Ehdr *)elf_info;
		Elf32_Addr e_entry;

		e_entry = elf_ehdr->e_entry;
		return (metal_phys_addr_t)e_entry;
	} else {
		Elf64_Ehdr *elf_ehdr = (Elf64_Ehdr *)elf_info;
		Elf64_Addr e_entry;

		e_entry = elf_ehdr->e_entry;
		return (metal_phys_addr_t)(e_entry & (metal_phys_addr_t)(-1));
	}
}

int elf_locate_rsc_table(void *elf_info, metal_phys_addr_t *da,
			 size_t *offset, size_t *size)
{
	char *sect_name = ".resource_table";
	void *shdr;
	unsigned int *load_state;

	if (elf_info == NULL)
		return -RPROC_EINVAL;

	load_state = elf_load_state(elf_info);
	if ((*load_state & ELF_STATE_HDRS_COMPLETE) == 0)
		return -RPROC_ERR_LOADER_STATE;
	shdr = elf_get_section_from_name(elf_info, sect_name);
	if (shdr == NULL) {
		metal_assert(size != NULL);
		*size = 0;
		return 0;
	}
	elf_parse_section(elf_info, shdr, NULL, NULL,
			  da, offset, size,
			  NULL, NULL, NULL, NULL);
	return 0;
}

int elf_get_load_state(void *img_info)
{
	unsigned int *load_state;

	if (img_info == NULL)
		return -RPROC_EINVAL;
	load_state = elf_load_state(img_info);
	return (int)(*load_state);
}

struct loader_ops elf_ops = {
	.load_header = elf_load_header,
	.load_data = elf_load,
	.locate_rsc_table = elf_locate_rsc_table,
	.release = elf_release,
	.get_entry = elf_get_entry,
	.get_load_state = elf_get_load_state,
};
