/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <elfloader.h>
#include <fs/fs.h>
#include <zephyr.h>
#include <string.h>

K_HEAP_DEFINE(_module_mem_heap, CONFIG_DYNAMIC_MODULE_MEM_SIZE * 1024);
static const char ElfMagic[] = {0x7f, 'E', 'L', 'F'};

/* find arbitrary symbol's address according to its name in a module */
void *zmodule_find_sym(zmodule_t *module, const char *sym_name)
{
	elf_word i;

	if (module == NULL) {
		return NULL;
	} else if (module == KERNEL_MODULE) {
		/* find symbols in zephyr kernel */
	} else {
		/* find symbols in module */
		for (i = 0; i < module->sym_cnt; i++) {
			if (strcmp(module->sym_list[i].name, sym_name) == 0) {
				return (void *)module->sym_list[i].addr;
			}
		}
	}
	return NULL;
}

/* load a relocatable object file.
 */
static void zmodule_load_rel(struct fs_file_t *zfp, zmodule_t *module)
{
	elf_ehdr_t ehdr;
	elf_word i, j, sym_cnt, rel_cnt;
	bool flag;
	elf_addr ptr, sym_addr;
	elf_shdr_t symtab_shdr, strtab_shdr;
	elf_shdr_t *shdr_array;
	elf_sym_t *sym;
	elf_rel_t rel;
	char str_name[50];

	fs_seek(zfp, 0, FS_SEEK_SET);
	fs_read(zfp, (void *)&ehdr, sizeof(ehdr));

	shdr_array = (elf_shdr_t *)k_malloc(
			(ehdr.e_shnum - 1) * sizeof(elf_shdr_t));

	/* iterate all sections and get total necessary memory size */
	flag = false;
	for (i = 0; i < ehdr.e_shnum - 1; i++) {
		fs_seek(zfp, ehdr.e_shoff + i * ehdr.e_shentsize, FS_SEEK_SET);
		fs_read(zfp, (void *)&shdr_array[i], sizeof(elf_shdr_t));

		/* get symtab and strtab sections */
		if (shdr_array[i].sh_type == SHT_SYMTAB) {
			symtab_shdr = shdr_array[i];
		} else if (shdr_array[i].sh_type == SHT_STRTAB) {
			strtab_shdr = shdr_array[i];
		}
		if ((shdr_array[i].sh_flags & SHF_ALLOC)
				&& (shdr_array[i].sh_size > 0)) {
			if (!flag) {
				module->virt_start_addr = shdr_array[i].sh_addr;
				module->mem_sz += shdr_array[i].sh_size;
				flag = true;
			} else {
				module->mem_sz += shdr_array[i].sh_size;
			}
		}
	}

	/* allocate memory */
	module->load_start_addr = (elf_addr)k_heap_alloc(&_module_mem_heap,
			(size_t)module->mem_sz, K_NO_WAIT);
	if ((void *)module->load_start_addr == NULL) {
		/* no available memory to load module */
		module = NULL;
		return;
	}
	memset((void *)module->load_start_addr, 0, sizeof(module->mem_sz));

	/* copy sections into memory */
	ptr = module->load_start_addr;
	for (i = 0; i < ehdr.e_shnum - 1; i++) {
		if ((shdr_array[i].sh_flags & SHF_ALLOC)
				&& (shdr_array[i].sh_size > 0)) {
			memcpy((void *)ptr, (void *)&shdr_array[i],
					(size_t)shdr_array[i].sh_size);
			shdr_array[i].sh_addr = ptr;
			ptr += shdr_array[i].sh_size;
		}
	}

	/* Iterate all symbols in symtab and update its st_value,
	 * for sections, using its loading address,
	 * for undef functions or variables, find it's address globally.
	 * */
	sym_cnt = symtab_shdr.sh_size / sizeof(elf_sym_t);
	sym = (elf_sym_t *)symtab_shdr.sh_addr;
	for (i = 0; i < sym_cnt; i++) {
		fs_seek(zfp, strtab_shdr.sh_offset + sym[i].st_name,
				FS_SEEK_SET);
		fs_read(zfp, str_name, sizeof(str_name));
		switch (sym[i].st_shndx) {
		case SHN_UNDEF:
			sym_addr = (elf_addr)zmodule_find_sym(KERNEL_MODULE,
					str_name);
			sym[i].st_value = (elf_addr)sym_addr;
			break;
		case SHN_ABS:
			break;
		case SHN_COMMON:
			break;
		default:
			sym[i].st_value += shdr_array[sym[i].st_shndx].sh_addr;
			break;
		}
	}

	/* symbols relocation */
	for (i = 0; i < ehdr.e_shnum - 1; i++) {
		/* find out relocation sections */
		if ((shdr_array[i].sh_type == SHT_REL)
				|| (shdr_array[i].sh_type == SHT_RELA)) {
			rel_cnt = shdr_array[i].sh_size / sizeof(elf_rel_t);

			for (j = 0; j < rel_cnt; j++) {
				/* get each relocation entry */
				fs_seek(zfp, shdr_array[i].sh_offset
						+ j * sizeof(elf_rel_t),
						FS_SEEK_SET);
				fs_read(zfp, (void *)&rel, sizeof(elf_rel_t));

				/* get corresponding symbol */
				fs_seek(zfp, symtab_shdr.sh_offset
						+ ELF_R_SYM(rel.r_info)
						* sizeof(elf_sym_t),
						FS_SEEK_SET);
				fs_read(zfp, (void *)&sym, sizeof(sym));

				/* doing relocation */
				elfloader_arch_relocate_rel(&rel,
					&shdr_array[shdr_array[i].sh_info],
					sym->st_value);
			}
		}
	}
}

/* load a shared object file */
static void zmodule_load_dyn(struct fs_file_t *zfp, zmodule_t *module)
{
	elf_ehdr_t ehdr;
	elf_phdr_t phdr;
	elf_shdr_t shdr, dynsym_shdr, dynstr_shdr;
	elf_word i, j, rel_cnt, dynsym_cnt, count, len;
	elf_addr end_addr, sym_addr;
	elf_rel_t rel;
	elf_sym_t sym;
	bool flag;
	char str_name[50];

	fs_seek(zfp, 0, FS_SEEK_SET);
	fs_read(zfp, (void *)&ehdr, sizeof(ehdr));

	/* iterate all program segments to get total necessary memory size */
	flag = false;
	end_addr = 0;
	for (i = 0; i < ehdr.e_phnum; i++) {
		fs_seek(zfp, ehdr.e_phoff + i * ehdr.e_phentsize,
				FS_SEEK_SET);
		fs_read(zfp, (void *)&phdr, sizeof(phdr));
		if (phdr.p_type == PT_LOAD) {
			if (!flag) {
				module->virt_start_addr = phdr.p_vaddr;
				end_addr = phdr.p_vaddr + phdr.p_memsz;
				flag = true;
			} else {
				end_addr = phdr.p_vaddr + phdr.p_memsz;
			}
		}
	}
	module->mem_sz = end_addr - module->virt_start_addr;

	/* allocate memory */
	module->load_start_addr = (elf_addr)k_heap_alloc(&_module_mem_heap,
			(size_t)module->mem_sz, K_NO_WAIT);
	if ((void *)module->load_start_addr == NULL) {
		/* no available memory to load module */
		module = NULL;
		return;
	}
	memset((void *)module->load_start_addr, 0, sizeof(module->mem_sz));

	/* copy segments into memory */
	for (i = 0; i < ehdr.e_phnum; i++) {
		fs_seek(zfp, ehdr.e_phoff + i * ehdr.e_phentsize,
				FS_SEEK_SET);
		fs_read(zfp, (void *)&phdr, sizeof(phdr));
		if (phdr.p_type == PT_LOAD) {
			fs_seek(zfp, phdr.p_offset, FS_SEEK_SET);
			fs_read(zfp, (void *)(module->load_start_addr
						+ phdr.p_vaddr
						- module->virt_start_addr),
					phdr.p_filesz);
		}
	}

	/* doing symbols relocation */
	for (i = 0; i < ehdr.e_shnum; i++) {
		fs_seek(zfp, ehdr.e_shoff + i * ehdr.e_shentsize,
				FS_SEEK_SET);
		fs_read(zfp, (void *)&shdr, sizeof(shdr));
		/* get .dynsym and .dynstr section, they will be used
		 * for symbols relocation and finding exported symbols
		 * of module.
		 */
		if (shdr.sh_type == SHT_DYNSYM) {
			/* get .dynsym section */
			dynsym_shdr = shdr;
			/* get .dynstr section */
			fs_seek(zfp, ehdr.e_shoff + dynsym_shdr.sh_link
					* ehdr.e_shentsize, FS_SEEK_SET);
			fs_read(zfp, (void *)&dynstr_shdr,
					sizeof(dynstr_shdr));
		}
		/* find out relocation sections */
		else if ((shdr.sh_type == SHT_REL)
				|| (shdr.sh_type == SHT_RELA)) {
			rel_cnt = shdr.sh_size / sizeof(elf_rel_t);
			for (j = 0; j < rel_cnt; j++) {
				/* get each relocation entry */
				fs_seek(zfp, shdr.sh_offset
						+ j * sizeof(elf_rel_t),
						FS_SEEK_SET);
				fs_read(zfp, (void *)&rel, sizeof(elf_rel_t));

				/* get corresponding symbol */
				fs_seek(zfp, dynsym_shdr.sh_offset
						+ ELF_R_SYM(rel.r_info)
						* sizeof(elf_sym_t),
						FS_SEEK_SET);
				fs_read(zfp, (void *)&sym, sizeof(sym));

				/* get corresponding symbol str name */
				fs_seek(zfp, dynstr_shdr.sh_offset
						+ sym.st_name, FS_SEEK_SET);
				fs_read(zfp, str_name, sizeof(str_name));

				/* find out symbol's real address */
				if (sym.st_shndx == SHN_UNDEF) {
					/* this symbol needs to be found
					 * globally.
					 */
					sym_addr = (elf_addr)zmodule_find_sym(
							KERNEL_MODULE,
							str_name);
				} else {
					/* this symbol could be found locally */
					sym_addr = module->load_start_addr
						+ sym.st_value
						- module->virt_start_addr;
				}

				/* doing relocation */
				elfloader_arch_relocate_dyn(module, &rel,
						sym_addr);
			}
		}
	}

	/* get all exported symbols of module.
	 * iterate all symbols in .dynsym section and find out those symbols
	 * with gloabl, function attributes.
	 */
	/* get total count of exported symbols */
	count = 0;
	dynsym_cnt = dynsym_shdr.sh_size / sizeof(elf_sym_t);
	for (j = 0; j < dynsym_cnt; j++) {
		fs_seek(zfp, dynsym_shdr.sh_offset + j * sizeof(elf_sym_t),
				FS_SEEK_SET);
		fs_read(zfp, (void *)&sym, sizeof(sym));
		if ((ELF_ST_BIND(sym.st_info) == STB_GLOBAL)
				&& (ELF_ST_TYPE(sym.st_info) == STT_FUNC)
				&& (sym.st_shndx != SHN_UNDEF)) {
			count++;
		}
	}
	module->sym_list = (zmodule_symbol_t *)k_malloc(count
			* sizeof(zmodule_symbol_t));
	module->sym_cnt = count;

	/* get each exported symbols's address and string name */
	for (j = 0, count = 0; j < dynsym_cnt; j++) {
		fs_seek(zfp, dynsym_shdr.sh_offset
				+ j * sizeof(elf_sym_t), FS_SEEK_SET);
		fs_read(zfp, (void *)&sym, sizeof(sym));
		if ((ELF_ST_BIND(sym.st_info) == STB_GLOBAL)
				&& (ELF_ST_TYPE(sym.st_info) == STT_FUNC)
				&& (sym.st_shndx != SHN_UNDEF)) {

			/* get symbol's address */
			module->sym_list[count].addr = sym.st_value
				- module->virt_start_addr
				+ module->load_start_addr;

			/* get symbol's string name */
			fs_seek(zfp, dynstr_shdr.sh_offset
					+ sym.st_name, FS_SEEK_SET);
			fs_read(zfp, str_name, sizeof(str_name));
			len = strlen(str_name) + 1;
			module->sym_list[count].name = k_malloc(len);
			memcpy((void *)module->sym_list[count].name,
					(void *)str_name, len);
			count++;
		}
	}
}

void *zmodule_load(const char *filename)
{
	int ret;
	struct fs_file_t zfp;
	elf_ehdr_t ehdr;
	zmodule_t *module;

	ret = fs_open(&zfp, filename, FS_O_READ);
	if (ret) {
		return NULL;
	}

	fs_seek(&zfp, 0, FS_SEEK_SET);
	fs_read(&zfp, (void *)&ehdr, sizeof(ehdr));

	/* check whether this is an valid elf file */
	if (memcmp(ehdr.e_ident, ElfMagic, sizeof(ElfMagic)) != 0) {
		/* invalid elf file */
		return NULL;
	}

	if (ehdr.e_type == ET_REL) {
		/* this is a relocatable file */
		module = k_malloc(sizeof(zmodule_t));
		zmodule_load_rel(&zfp, module);
	} else if (ehdr.e_type == ET_DYN) {
		/* this is a shared object file */
		module = k_malloc(sizeof(zmodule_t));
		zmodule_load_dyn(&zfp, module);
	} else {
		/* unsupported ELF file type */
		module = NULL;
	}

	fs_close(&zfp);
	return module;
}
