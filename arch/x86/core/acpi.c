/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/arch/x86/acpi.h>
#include <zephyr/arch/x86/efi.h>

static struct acpi_rsdp *rsdp;
static bool is_rsdp_searched;

static struct acpi_dmar *dmar;
static bool is_dmar_searched;

static bool check_sum(struct acpi_sdt *t)
{
	uint8_t sum = 0U, *p = (uint8_t *)t;

	for (int i = 0; i < t->length; i++) {
		sum += p[i];
	}

	return sum == 0U;
}

static void find_rsdp(void)
{
	uint8_t *bda_seg, *zero_page_base;
	uint64_t *search;
	uintptr_t search_phys, rsdp_phys = 0U;
	size_t search_length = 0U, rsdp_length;

	if (is_rsdp_searched) {
		/* Looking up for RSDP has already been done */
		return;
	}

	/* Let's first get it from EFI, if enabled */
	if (IS_ENABLED(CONFIG_X86_EFI)) {
		rsdp_phys = (uintptr_t)efi_get_acpi_rsdp();
		if (rsdp_phys != 0UL) {
			/* See at found label why this is required */
			search_length = sizeof(struct acpi_rsdp);
			z_phys_map((uint8_t **)&search, rsdp_phys, search_length, 0);
			rsdp = (struct acpi_rsdp *)search;

			goto found;
		}
	}

	/* We never identity map the NULL page, so need to map it before
	 * it can be accessed.
	 */
	z_phys_map(&zero_page_base, 0, 4096, 0);

	/* Physical (real mode!) address 0000:040e stores a (real
	 * mode!!) segment descriptor pointing to the 1kb Extended
	 * BIOS Data Area.
	 *
	 * We had to memory map this segment descriptor since it is in
	 * the NULL page. The remaining structures (EBDA etc) are identity
	 * mapped somewhere within the minefield of reserved regions in the
	 * first megabyte and are directly accessible.
	 */
	bda_seg = 0x040e + zero_page_base;
	search_phys = (long)(((int)*(uint16_t *)bda_seg) << 4);

	/* Unmap after use */
	z_phys_unmap(zero_page_base, 4096);

	/* Might be nothing there, check before we inspect.
	 * Note that EBDA usually is in 0x80000 to 0x100000.
	 */
	if ((POINTER_TO_UINT(search_phys) >= 0x80000UL) &&
	    (POINTER_TO_UINT(search_phys) < 0x100000UL)) {
		search_length = 1024;
		z_phys_map((uint8_t **)&search, search_phys, search_length, 0);

		for (int i = 0; i < 1024/8; i++) {
			if (search[i] == ACPI_RSDP_SIGNATURE) {
				rsdp_phys = search_phys + i * 8;
				rsdp = (void *)&search[i];
				goto found;
			}
		}

		z_phys_unmap((uint8_t *)search, search_length);
	}

	/* If it's not there, then look for it in the last 128kb of
	 * real mode memory.
	 */
	search_phys = 0xe0000;
	search_length = 128 * 1024;
	z_phys_map((uint8_t **)&search, search_phys, search_length, 0);

	rsdp_phys = 0U;
	for (int i = 0; i < 128*1024/8; i++) {
		if (search[i] == ACPI_RSDP_SIGNATURE) {
			rsdp_phys = search_phys + i * 8;
			rsdp = (void *)&search[i];
			goto found;
		}
	}

	z_phys_unmap((uint8_t *)search, search_length);

	rsdp = NULL;

	is_rsdp_searched = true;

	return;

found:
	/* Determine length of RSDP table.
	 * ACPI v2 and above uses the length field.
	 * Otherwise, just the size of struct itself.
	 */
	if (rsdp->revision < 2) {
		rsdp_length = sizeof(*rsdp);
	} else {
		rsdp_length = rsdp->length;
	}

	/* Need to unmap search since it is still mapped */
	if (search_length != 0U) {
		z_phys_unmap((uint8_t *)search, search_length);
	}

	/* Now map the RSDP */
	z_phys_map((uint8_t **)&rsdp, rsdp_phys, rsdp_length, 0);

	is_rsdp_searched = true;
}

void *z_acpi_find_table(uint32_t signature)
{
	uint8_t *mapped_tbl;
	uint32_t length;
	struct acpi_rsdt *rsdt;
	struct acpi_xsdt *xsdt;
	struct acpi_sdt *t;
	uintptr_t t_phys;
	bool tbl_found;

	find_rsdp();

	if (!rsdp) {
		return NULL;
	}

	if (rsdp->rsdt_ptr != 0U) {
		z_phys_map((uint8_t **)&rsdt, rsdp->rsdt_ptr, sizeof(*rsdt), 0);
		tbl_found = false;

		if (check_sum(&rsdt->sdt)) {
			/* Remap the memory to the indicated length of RSDT */
			length = rsdt->sdt.length;
			z_phys_unmap((uint8_t *)rsdt, sizeof(*rsdt));
			z_phys_map((uint8_t **)&rsdt, rsdp->rsdt_ptr, length, 0);

			uint32_t *end = (uint32_t *)((char *)rsdt + rsdt->sdt.length);

			/* Extra indirection required to avoid
			 * -Waddress-of-packed-member
			 */
			void *table_ptrs = &rsdt->table_ptrs[0];

			for (uint32_t *tp = table_ptrs; tp < end; tp++) {
				t_phys = (long)*tp;
				z_phys_map(&mapped_tbl, t_phys, sizeof(*t), 0);
				t = (void *)mapped_tbl;

				if (t->signature == signature && check_sum(t)) {
					tbl_found = true;
					break;
				}

				z_phys_unmap(mapped_tbl, sizeof(*t));
			}
		}

		z_phys_unmap((uint8_t *)rsdt, sizeof(*rsdt));

		if (tbl_found) {
			goto found;
		}
	}

	if (rsdp->revision < 2) {
		return NULL;
	}

	if (rsdp->xsdt_ptr != 0ULL) {
		z_phys_map((uint8_t **)&xsdt, rsdp->xsdt_ptr, sizeof(*xsdt), 0);

		tbl_found = false;
		if (check_sum(&xsdt->sdt)) {
			/* Remap the memory to the indicated length of RSDT */
			length = xsdt->sdt.length;
			z_phys_unmap((uint8_t *)xsdt, sizeof(*xsdt));
			z_phys_map((uint8_t **)&xsdt, rsdp->xsdt_ptr, length, 0);

			uint64_t *end = (uint64_t *)((char *)xsdt + xsdt->sdt.length);

			/* Extra indirection required to avoid
			 * -Waddress-of-packed-member
			 */
			void *table_ptrs = &xsdt->table_ptrs[0];

			for (uint64_t *tp = table_ptrs; tp < end; tp++) {
				t_phys = (long)*tp;
				z_phys_map(&mapped_tbl, t_phys, sizeof(*t), 0);
				t = (void *)mapped_tbl;

				if (t->signature == signature && check_sum(t)) {
					tbl_found = true;
					break;
				}

				z_phys_unmap(mapped_tbl, sizeof(*t));
			}
		}

		z_phys_unmap((uint8_t *)xsdt, sizeof(*xsdt));

		if (tbl_found) {
			goto found;
		}
	}

	return NULL;

found:
	/* Remap to indicated length of the table */
	length = t->length;
	z_phys_unmap(mapped_tbl, sizeof(*t));
	z_phys_map(&mapped_tbl, t_phys, length, 0);
	t = (void *)mapped_tbl;

	return t;
}

/*
 * Return the 'n'th CPU entry from the ACPI MADT, or NULL if not available.
 */

struct acpi_cpu *z_acpi_get_cpu(int n)
{
	struct acpi_madt *madt = z_acpi_find_table(ACPI_MADT_SIGNATURE);
	uintptr_t base = POINTER_TO_UINT(madt);
	uintptr_t offset;

	if (!madt) {
		return NULL;
	}

	offset = POINTER_TO_UINT(madt->entries) - base;

	while (offset < madt->sdt.length) {
		struct acpi_madt_entry *entry;

		entry = (struct acpi_madt_entry *)(offset + base);
		if (entry->type == ACPI_MADT_ENTRY_CPU) {
			struct acpi_cpu *cpu = (struct acpi_cpu *)entry;

			if ((cpu->flags & ACPI_CPU_FLAGS_ENABLED) != 0) {
				if (n == 0) {
					return cpu;
				}

				--n;
			}
		}

		offset += entry->length;
	}

	return NULL;
}

static void find_dmar(void)
{
	if (is_dmar_searched) {
		return;
	}

	dmar = z_acpi_find_table(ACPI_DMAR_SIGNATURE);
	is_dmar_searched = true;
}

struct acpi_dmar *z_acpi_find_dmar(void)
{
	find_dmar();
	return dmar;
}

struct acpi_drhd *z_acpi_find_drhds(int *n)
{
	struct acpi_drhd *drhds = NULL;
	uintptr_t offset;
	uintptr_t base;

	find_dmar();

	if (dmar == NULL) {
		return NULL;
	}

	*n = 0;
	base = POINTER_TO_UINT(dmar);

	offset = POINTER_TO_UINT(dmar->remap_entries) - base;
	while (offset < dmar->sdt.length) {
		struct acpi_dmar_entry *entry;

		entry = (struct acpi_dmar_entry *)(offset + base);
		if (entry->type == ACPI_DMAR_TYPE_DRHD) {
			if (*n == 0) {
				drhds = (struct acpi_drhd *)entry;
			}

			(*n)++;
		} else {
			/* DMAR entries are found packed by type so
			 * if type is not DRHD, we will not encounter one,
			 * anymore.
			 */
			break;
		}

		offset += entry->length;
	}

	return drhds;
}

struct acpi_dmar_dev_scope *z_acpi_get_drhd_dev_scopes(struct acpi_drhd *drhd,
						       int *n)
{
	uintptr_t offset;
	uintptr_t base;

	if (drhd->entry.length <= ACPI_DRHD_MIN_SIZE) {
		return NULL;
	}

	*n = 0;
	base = POINTER_TO_UINT(drhd);

	offset = POINTER_TO_UINT(drhd->device_scope) - base;
	while (offset < drhd->entry.length) {
		struct acpi_dmar_dev_scope *dev_scope;

		dev_scope = (struct acpi_dmar_dev_scope *)(offset + base);

		(*n)++;

		offset += dev_scope->length;
	}

	return (*n == 0) ? NULL : drhd->device_scope;
}

struct acpi_dmar_dev_path *
z_acpi_get_dev_scope_paths(struct acpi_dmar_dev_scope *dev_scope, int *n)
{
	switch (dev_scope->type) {
	case ACPI_DRHD_DEV_SCOPE_PCI_EPD:
		/* Fall through */
	case ACPI_DRHD_DEV_SCOPE_PCI_SUB_H:
		/* Fall through */
	case ACPI_DRHD_DEV_SCOPE_IOAPIC:
		if (dev_scope->length < (ACPI_DMAR_DEV_SCOPE_MIN_SIZE +
					 ACPI_DMAR_DEV_PATH_SIZE)) {
			return NULL;
		}

		break;
	case ACPI_DRHD_DEV_SCOPE_MSI_CAP_HPET:
		/* Fall through */
	case ACPI_DRHD_DEV_SCOPE_NAMESPACE_DEV:
		if (dev_scope->length != (ACPI_DMAR_DEV_SCOPE_MIN_SIZE +
					  ACPI_DMAR_DEV_PATH_SIZE)) {
			return NULL;
		}

		break;
	default:
		return NULL;
	}

	*n = (dev_scope->length - ACPI_DMAR_DEV_SCOPE_MIN_SIZE) /
		ACPI_DMAR_DEV_PATH_SIZE;

	return dev_scope->path;
}

uint16_t z_acpi_get_dev_id_from_dmar(uint8_t dev_scope_type)
{
	struct acpi_drhd *drhd;
	int n_drhd;

	find_dmar();

	if (dmar == NULL) {
		return USHRT_MAX;
	}

	drhd = z_acpi_find_drhds(&n_drhd);

	for (; n_drhd > 0; n_drhd--) {
		struct acpi_dmar_dev_scope *dev_scope;
		int n_ds;

		dev_scope = z_acpi_get_drhd_dev_scopes(drhd, &n_ds);
		for (; n_ds > 0; n_ds--) {
			if (dev_scope->type == dev_scope_type) {
				struct acpi_dmar_dev_path *path;
				int n_path;

				path = z_acpi_get_dev_scope_paths(dev_scope,
								  &n_path);
				if (n_path > 0) {
					union acpi_dmar_id id;

					/* Let's over simplify for now:
					 * we don't look for secondary bus
					 * and extra paths. We just stop here.
					 */

					id.bits.bus = dev_scope->start_bus_num;
					id.bits.device = path->device;
					id.bits.function = path->function;

					return id.raw;
				}
			}

			dev_scope = (struct acpi_dmar_dev_scope *)(
				POINTER_TO_UINT(dev_scope) + dev_scope->length);
		}

		drhd = (struct acpi_drhd *)(POINTER_TO_UINT(drhd) +
					    drhd->entry.length);
	}

	return USHRT_MAX;
}
