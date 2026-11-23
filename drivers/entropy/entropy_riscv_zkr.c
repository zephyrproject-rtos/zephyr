/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* seed CSR fields */
#define SEED_OPST    GENMASK(31, 30)
#define SEED_ENTROPY GENMASK(15, 0)

/* seed CSR status (OPST) values */
#define SEED_OPST_BIST 0U
#define SEED_OPST_WAIT 1U
#define SEED_OPST_ES16 2U
#define SEED_OPST_DEAD 3U

static int entropy_riscv_zkr_get_entropy_isr(const struct device *dev __unused, uint8_t *buffer,
					     uint16_t length, uint32_t flags)
{
	uint32_t max_retries =
		((flags & ENTROPY_BUSYWAIT) != 0U) ? CONFIG_ENTROPY_RISCV_ZKR_MAX_RETRIES : 0U;
	uint32_t failure_counter = 0U;
	uint16_t generated_bytes = 0U;

	while (length > 0U) {
		/*
		 * The seed CSR traps on read-only accesses, it has to be written as well.
		 * It is 32 bits wide, the upper bits read on RV64 are not part of it.
		 */
		uint32_t seed = (uint32_t)csr_swap(CSR_SEED, 0);
		uint32_t opst = FIELD_GET(SEED_OPST, seed);

		if (opst == SEED_OPST_DEAD) {
			return -EIO;
		}

		if (opst == SEED_OPST_ES16) {
			uint16_t value = (uint16_t)FIELD_GET(SEED_ENTROPY, seed);
			size_t to_copy = MIN(length, sizeof(value));

			memcpy(buffer, &value, to_copy);
			buffer += to_copy;
			length -= to_copy;
			generated_bytes += to_copy;
			failure_counter = 0U;
		} else if (failure_counter < max_retries) {
			/* BIST or WAIT, no entropy available yet */
			failure_counter++;
			k_busy_wait(CONFIG_ENTROPY_RISCV_ZKR_RETRY_WAIT_USEC);
		} else {
			break;
		}
	}

	return (int)generated_bytes;
}

static int entropy_riscv_zkr_get_entropy(const struct device *dev, uint8_t *buffer,
					 uint16_t length)
{
	int ret = entropy_riscv_zkr_get_entropy_isr(dev, buffer, length, ENTROPY_BUSYWAIT);

	if (ret < 0) {
		return ret;
	}

	return (ret == (int)length) ? 0 : -ENODATA;
}

static DEVICE_API(entropy, entropy_riscv_zkr_api) = {
	.get_entropy = entropy_riscv_zkr_get_entropy,
	.get_entropy_isr = entropy_riscv_zkr_get_entropy_isr,
};

DEVICE_DEFINE(riscv_zkr_entropy_device, "RISCV_ZKR_ENTROPY", NULL, NULL, NULL, NULL, PRE_KERNEL_1,
	      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_riscv_zkr_api);

const struct device *const z_arch_entropy_dev = DEVICE_GET(riscv_zkr_entropy_device);
