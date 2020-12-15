/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <arch/x86/acpi.h>

static struct acpi_rsdp *rsdp;
bool is_rdsp_searched = false;
static struct acpi_dmar *dmar;
bool is_dmar_searched;

static bool check_sum(struct acpi_sdt *t)
{
	uint8_t sum = 0, *p = (uint8_t *)t;

	for (int i = 0; i < t->length; i++) {
		sum += p[i];
	}

	return sum == 0;
}


/* We never identity map the NULL page, but may need to read some BIOS data */
static uint8_t *zero_page_base;

static void find_rsdp(void)
{
	uint8_t *bda_seg;

	if (is_rdsp_searched) {
		/* Looking up for RSDP has already been done */
		return;
	}

	if (zero_page_base == NULL) {
		z_phys_map(&zero_page_base, 0, 4096, K_MEM_PERM_RW);
	}

	/* Physical (real mode!) address 0000:040e stores a (real
	 * mode!!) segment descriptor pointing to the 1kb Extended
	 * BIOS Data Area.  Look there first.
	 *
	 * We had to memory map this segment descriptor since it is in
	 * the NULL page. The remaining structures (EBDA etc) are identity
	 * mapped somewhere within the minefield of reserved regions in the
	 * first megabyte and are directly accessible.
	 */
	bda_seg = 0x040e + zero_page_base;
	uint64_t *search = (void *)(long)(((int)*(uint16_t *)bda_seg) << 4);

	/* Might be nothing there, check before we inspect */
	if (search != NULL) {
		for (int i = 0; i < 1024/8; i++) {
			if (search[i] == ACPI_RSDP_SIGNATURE) {
				rsdp = (void *)&search[i];
				goto out;
			}
		}
	}

	/* If it's not there, then look for it in the last 128kb of
	 * real mode memory.
	 */
	search = (uint64_t *)0xe0000;

	for (int i = 0; i < 128*1024/8; i++) {
		if (search[i] == ACPI_RSDP_SIGNATURE) {
			rsdp = (void *)&search[i];
			goto out;
		}
	}

	/* Now we're supposed to look in the UEFI system table, which
	 * is passed as a function argument to the bootloader and long
	 * forgotten by now...
	 */
	rsdp = NULL;
out:
	is_rdsp_searched = true;
}

void *z_acpi_find_table(uint32_t signature)
{
	find_rsdp();

	if (!rsdp) {
		return NULL;
	}

	struct acpi_rsdt *rsdt = (void *)(long)rsdp->rsdt_ptr;

	if (rsdt && check_sum(&rsdt->sdt)) {
		uint32_t *end = (uint32_t *)((char *)rsdt + rsdt->sdt.length);

		for (uint32_t *tp = &rsdt->table_ptrs[0]; tp < end; tp++) {
			struct acpi_sdt *t = (void *)(long)*tp;

			if (t->signature == signature && check_sum(t)) {
				return t;
			}
		}
	}

	if (rsdp->revision < 2) {
		return NULL;
	}

	struct acpi_xsdt *xsdt = (void *)(long)rsdp->xsdt_ptr;

	if (xsdt && check_sum(&xsdt->sdt)) {
		uint64_t *end = (uint64_t *)((char *)xsdt + xsdt->sdt.length);

		for (uint64_t *tp = &xsdt->table_ptrs[0]; tp < end; tp++) {
			struct acpi_sdt *t = (void *)(long)*tp;

			if (t->signature == signature && check_sum(t)) {
				return t;
			}
		}
	}

	return NULL;
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

			if (cpu->flags & ACPI_CPU_FLAGS_ENABLED) {
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
