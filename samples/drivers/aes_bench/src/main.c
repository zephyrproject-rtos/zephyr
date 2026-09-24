/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/cpu_load.h>
#include <zephyr/crypto/crypto.h>

#ifdef CONFIG_CRYPTO_MBEDTLS_SHIM
#define CRYPTO_DRV_NAME CONFIG_CRYPTO_MBEDTLS_SHIM_DRV_NAME
#elif CONFIG_CRYPTO_ESP32_AES
#define CRYPTO_DEV_COMPAT espressif_esp32_aes
#elif CONFIG_CRYPTO_MCHP_AES_G1
#define CRYPTO_DEV_COMPAT microchip_aes_g1
#elif DT_HAS_COMPAT_STATUS_OKAY(sifli_sf32lb_crypto)
#define CRYPTO_DEV_COMPAT sifli_sf32lb_crypto
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_aes)
#define CRYPTO_DEV_COMPAT st_stm32_aes
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_cryp)
#define CRYPTO_DEV_COMPAT st_stm32_cryp
#elif DT_HAS_COMPAT_STATUS_OKAY(bflb_sec_eng_aes)
#define CRYPTO_DEV_COMPAT bflb_sec_eng_aes
#elif DT_HAS_COMPAT_STATUS_OKAY(realtek_bee_aes)
#define CRYPTO_DEV_COMPAT realtek_bee_aes
#elif CONFIG_CRYPTO_INFINEON_MXCRYPTOLITE
#define CRYPTO_DEV_COMPAT infineon_mxcryptolite_crypto
#elif CONFIG_CRYPTO_INFINEON_MXCRYPTO
#define CRYPTO_DEV_COMPAT infineon_mxcrypto_crypto
#else
#error "You need to enable one crypto device"
#endif

/*
 * Some crypto drivers require IO buffers to be cache-line aligned and
 * to occupy whole cache lines (the underlying allocation must be padded
 * up to a multiple of the cache-line size).
 */
#define IO_ALIGNMENT_BYTES CONFIG_AES_BENCH_IO_ALIGNMENT_BYTES

#define AES_BLOCK_LEN 16

static uint8_t bench_buf[64 * 1024] __aligned(IO_ALIGNMENT_BYTES);
static uint8_t bench_out[sizeof(bench_buf)] __aligned(IO_ALIGNMENT_BYTES);

static const uint8_t aes256_key[32] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};

static inline const struct device *get_crypto_dev(void)
{
#ifdef CRYPTO_DRV_NAME
	return device_get_binding(CRYPTO_DRV_NAME);
#else
	return DEVICE_DT_GET_ONE(CRYPTO_DEV_COMPAT);
#endif
}

static int run_bench_case(const struct device *dev, const char *name, size_t msg_len, int loops)
{
	struct cipher_ctx ctx = {
		.keylen = sizeof(aes256_key),
		.key.bit_stream = aes256_key,
		.flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
	};
	struct cipher_pkt pkt = {
		.in_buf = bench_buf,
		.in_len = msg_len,
		.out_buf = bench_out,
		.out_buf_max = sizeof(bench_out),
	};
	uint8_t iv[AES_BLOCK_LEN] = {0};
	uint64_t total_bytes = (uint64_t)msg_len * (uint64_t)loops;
	uint64_t cps = sys_clock_hw_cycles_per_sec();
	uint64_t dt_cycles;
	uint64_t bps;
	uint32_t t0;
	uint32_t t1;
	int cpu_permille;
	int rc = cipher_begin_session(dev, &ctx, CRYPTO_CIPHER_ALGO_AES, CRYPTO_CIPHER_MODE_CBC,
				      CRYPTO_CIPHER_OP_ENCRYPT);

	if (rc == -ENOTSUP) {
		printk("BENCH %s: not supported by this driver, skipping\n", name);
		return 0;
	}
	if (rc != 0) {
		printk("BENCH %s: begin_session failed (%d)\n", name, rc);
		return rc;
	}

	/* Warm-up */
	rc = cipher_cbc_op(&ctx, &pkt, iv);
	if (rc != 0) {
		printk("BENCH %s: warmup failed (%d)\n", name, rc);
		cipher_free_session(dev, &ctx);
		return rc;
	}

	(void)cpu_load_get(true);
	t0 = k_cycle_get_32();
	for (int i = 0; i < loops; i++) {
		rc = cipher_cbc_op(&ctx, &pkt, iv);
		if (rc != 0) {
			printk("BENCH %s: loop %d failed (%d)\n", name, i, rc);
			cipher_free_session(dev, &ctx);
			return rc;
		}
	}
	t1 = k_cycle_get_32();
	cpu_permille = cpu_load_get(true);

	cipher_free_session(dev, &ctx);

	dt_cycles = (uint32_t)(t1 - t0);
	/*
	 * On very fast/simulated targets (e.g. native_sim) a run can complete
	 * within a single cycle-counter tick, making dt_cycles read back as 0.
	 * Clamp it to avoid a division-by-zero (SIGFPE) below; the resulting
	 * throughput figure is meaningless in that case anyway.
	 */
	if (dt_cycles == 0) {
		dt_cycles = 1;
	}
	bps = (total_bytes * cps) / dt_cycles;

	printk("BENCH %s len=%u loops=%d cycles=%llu cpb=%u MBps=%u.%03u CPU=%d.%d%%\n", name,
	       (unsigned int)msg_len, loops, (unsigned long long)dt_cycles,
	       (unsigned int)(dt_cycles / total_bytes), (unsigned int)(bps / 1000000ULL),
	       (unsigned int)((bps % 1000000ULL) / 1000ULL), cpu_permille / 10,
	       cpu_permille % 10);

	return 0;
}

static int run_bench_suite(const char *name)
{
	/*
	 * 65520 (4095 * AES_BLOCK_LEN), not 65536: CBC needs a block-aligned
	 * length, and some HW drivers take a uint16_t byte count and silently
	 * wrap 65536 to 0.
	 */
	static const size_t msg_lens[] = {64, 256, 1024, 4096, 16384, 65520};
	static const int loop_counts[] = {2000, 1000, 500, 200, 100, 40};
	const struct device *dev = get_crypto_dev();
	int rc;

	if (!dev || !device_is_ready(dev)) {
		printk("Crypto device is not ready\n");
		return -ENODEV;
	}

	for (size_t i = 0; i < sizeof(bench_buf); i++) {
		bench_buf[i] = (uint8_t)(i & 0xff);
	}

	for (size_t i = 0; i < ARRAY_SIZE(msg_lens); i++) {
		rc = run_bench_case(dev, name, msg_lens[i], loop_counts[i]);
		if (rc != 0) {
			return rc;
		}
	}

	return 0;
}

int main(void)
{
#if defined(CONFIG_CRYPTO_MBEDTLS_SHIM)
	printk("AES SW benchmark\n");
#else
	printk("AES HW benchmark\n");
#endif

	(void)run_bench_suite("AES256-CBC");

	return 0;
}
