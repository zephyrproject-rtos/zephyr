/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/x86/acpi.h>

/*
 * Finding and walking the ACPI tables can be time consuming, so we do
 * it once, early, and then cache the "interesting" results for later.
 */

static struct acpi_madt *madt;

/*
 * ACPI structures use a simple checksum, such that
 * summing all the bytes in the structure yields 0.
 */

static bool validate_checksum(void *buf, int len)
{
	u8_t *cp = buf;
	u8_t checksum = 0;

	while (len--) {
		checksum += *(cp++);
	}

	return (checksum == 0);
}

/*
 * Called very early during initialization to find ACPI tables of interest.
 * First, we find the RDSP, and if found, use that to find the RSDT, which
 * we then use to find the MADT. (This function is long, but easy to follow.)
 */

void z_acpi_init(void)
{
	/*
	 * First, find the RSDP by probing "well-known" areas of memory.
	 */

	struct acpi_rsdp *rsdp = NULL;

	static const struct {
		u32_t base;
		u32_t top;
	} area[] = {
		{ 0x000E0000, 0x00100000 },	/* BIOS ROM */
		{ 0, 0 }
	};

	for (int i = 0; area[i].base && area[i].top && !rsdp; ++i) {
		u32_t addr = area[i].base;

		while (addr < area[i].top) {
			struct acpi_rsdp *probe = UINT_TO_POINTER(addr);

			if ((probe->signature == ACPI_RSDP_SIGNATURE) &&
			    (validate_checksum(probe, sizeof(*probe)))) {
				rsdp = probe;
				break;
			}

			addr += 0x10;
		}
	}

	if (rsdp == NULL) {
		return;
	}

	/*
	 * Then, validate the RSDT fingered by the RSDP.
	 */

	struct acpi_rsdt *rsdt = UINT_TO_POINTER(rsdp->rsdt);
	if ((rsdt->sdt.signature != ACPI_RSDT_SIGNATURE) ||
	    !validate_checksum(rsdt, rsdt->sdt.length)) {
		rsdt = NULL;
		return;
	}

	/*
	 * Finally, probe each SDT listed in the RSDT to find the MADT.
	 * If it's valid, then remember it for later.
	 */

	int nr_sdts = (rsdt->sdt.length - sizeof(rsdt)) / sizeof(u32_t);
	for (int i = 0; i < nr_sdts; ++i) {
		struct acpi_sdt *sdt = UINT_TO_POINTER(rsdt->sdts[i]);

		if ((sdt->signature == ACPI_MADT_SIGNATURE)
		    && validate_checksum(sdt, sdt->length)) {
			madt = (struct acpi_madt *) sdt;
			break;
		}
	}
}

/*
 * Return the 'n'th CPU entry from the ACPI MADT, or NULL if not available.
 */

struct acpi_cpu *z_acpi_get_cpu(int n)
{
	u32_t base = POINTER_TO_UINT(madt);
	u32_t offset;

	if (madt) {
		offset = POINTER_TO_UINT(madt->entries) - base;

		while (offset < madt->sdt.length) {
			struct acpi_madt_entry *entry;

			entry = (struct acpi_madt_entry *) (offset + base);

			if (entry->type == ACPI_MADT_ENTRY_CPU) {
				if (n) {
					--n;
				} else {
					return (struct acpi_cpu *) entry;
				}
			}

			offset += entry->length;
		}
	}

	return NULL;
}
