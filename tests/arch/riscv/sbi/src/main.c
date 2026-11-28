/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/riscv/sbi.h>
#include <zephyr/version.h>
#include <zephyr/ztest.h>

static unsigned long sbi_value(struct sbiret ret)
{
	zassert_equal(ret.error, SBI_SUCCESS, "SBI error %ld", ret.error);
	return ret.uvalue;
}

ZTEST(sbi_base, test_identification)
{
	unsigned long version = sbi_value(sbi_get_spec_version());
	unsigned long implementation = sbi_value(sbi_get_impl_id());
	unsigned long revision = sbi_value(sbi_get_impl_version());

	/* BASE was introduced with SBI v0.2. Bits 31 and above are reserved. */
	zassert_between_inclusive(version, 2UL, 0x7fffffffUL);
	if (IS_ENABLED(CONFIG_RISCV_S_MODE_INTERNAL_SBI)) {
		zassert_equal(version, 3U); /* SBI v0.3 */
		zassert_equal(revision, KERNEL_VERSION_NUMBER);
		zassert_not_equal(implementation, 1U); /* OpenSBI */
	} else {
		zassert_equal(implementation, 1U); /* OpenSBI */
	}

	/* The standard permits a zero value for each machine identification CSR. */
	(void)sbi_value(sbi_get_mvendorid());
	(void)sbi_value(sbi_get_marchid());
	(void)sbi_value(sbi_get_mimpid());
}

ZTEST(sbi_base, test_probe_extension)
{
	zassert_not_equal(sbi_value(sbi_probe_extension(SBI_EXT_BASE)), 0UL);
	zassert_not_equal(sbi_value(sbi_probe_extension(SBI_EXT_TIME)), 0UL);
	zassert_not_equal(sbi_value(sbi_probe_extension(SBI_EXT_SRST)), 0UL);
	zassert_equal(sbi_value(sbi_probe_extension(0x7fffffffUL)), 0UL);

	/* QEMU's OpenSBI provides HSM. Zephyr's internal runtime does not. */
	zassert_equal(sbi_value(sbi_probe_extension(SBI_EXT_HSM)) != 0UL,
		      IS_ENABLED(CONFIG_RISCV_S_MODE_EXTERNAL_SBI));
}

ZTEST(sbi_base, test_unknown_function)
{
	struct sbiret ret = sbi_ecall(0, 0, 0, 0, 0, 0, 0x7fffffffUL, SBI_EXT_BASE);

	zassert_equal(ret.error, SBI_ERR_NOT_SUPPORTED);
}

ZTEST_SUITE(sbi_base, NULL, NULL, NULL, NULL, NULL);
