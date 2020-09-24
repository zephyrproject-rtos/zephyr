/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <arch/x86/acpi.h>

static struct acpi_rsdp *rsdp;
bool is_rdsp_searched = false;

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
		z_mem_map(&zero_page_base, 0, 4096, K_MEM_PERM_RW);
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
