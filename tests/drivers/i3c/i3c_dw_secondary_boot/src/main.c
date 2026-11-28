/*
 * Copyright (c) 2026 Mark Tamas
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Build-only coverage for booting a dual-role snps,designware-i3c as a
 * target via the `secondary` devicetree property.  There is no emulator
 * for this IP, so the value is in the driver compiling and linking with
 * the property set, for both controller-enabled and target-only builds.
 */

int main(void)
{
	return 0;
}
