/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/cpu_load.h>
#include <string.h>

#ifdef CONFIG_CRYPTO_MBEDTLS_SHIM
#define CRYPTO_DRV_NAME CONFIG_CRYPTO_MBEDTLS_SHIM_DRV_NAME
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_hash)
#define CRYPTO_DEV_COMPAT st_stm32_hash
#elif DT_HAS_COMPAT_STATUS_OKAY(microchip_sha_g1_crypto)
#define CRYPTO_DEV_COMPAT microchip_sha_g1_crypto
#elif DT_HAS_COMPAT_STATUS_OKAY(renesas_smartbond_crypto)
#define CRYPTO_DEV_COMPAT renesas_smartbond_crypto
#elif DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_sha)
#define CRYPTO_DEV_COMPAT espressif_esp32_sha
#elif DT_HAS_COMPAT_STATUS_OKAY(raspberrypi_pico_sha256)
#define CRYPTO_DEV_COMPAT raspberrypi_pico_sha256
#elif DT_HAS_COMPAT_STATUS_OKAY(bflb_sec_eng_sha)
#define CRYPTO_DEV_COMPAT bflb_sec_eng_sha
#elif DT_HAS_COMPAT_STATUS_OKAY(realtek_bee_sha256)
#define CRYPTO_DEV_COMPAT realtek_bee_sha256
#else
#error "No supported hash crypto device found"
#endif

#define IO_ALIGNMENT_BYTES 32
#define SHA256_LEN         32
#define SHA3_512_LEN       64

static const uint8_t msg_abc[] __aligned(IO_ALIGNMENT_BYTES) = {'a', 'b', 'c'};
static const uint8_t sha256_abc[SHA256_LEN] = {
	0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40,
	0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17,
	0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
};

/* 64 x 'a': exactly one full 512-bit block, forcing padding into a second
 * block. Digest verified with `printf 'a%.0s' {1..64} | sha256sum`.
 */
static const uint8_t msg_64a[64] __aligned(IO_ALIGNMENT_BYTES) = {
	'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
	'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
	'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
	'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
};
static const uint8_t sha256_64a[SHA256_LEN] = {
	0xff, 0xe0, 0x54, 0xfe, 0x7a, 0xe0, 0xcb, 0x6d, 0xc6, 0x5c, 0x3a,
	0xf9, 0xb6, 0x1d, 0x52, 0x09, 0xf4, 0x39, 0x85, 0x1d, 0xb4, 0x3d,
	0x0b, 0xa5, 0x99, 0x73, 0x37, 0xdf, 0x15, 0x46, 0x68, 0xeb,
};

static const uint8_t sha3_512_abc[SHA3_512_LEN] = {
	0xb7, 0x51, 0x85, 0x0b, 0x1a, 0x57, 0x16, 0x8a, 0x56, 0x93, 0xcd, 0x92, 0x4b,
	0x6b, 0x09, 0x6e, 0x08, 0xf6, 0x21, 0x82, 0x74, 0x44, 0xf7, 0x0d, 0x88, 0x4f,
	0x5d, 0x02, 0x40, 0xd2, 0x71, 0x2e, 0x10, 0xe1, 0x16, 0xe9, 0x19, 0x2a, 0xf3,
	0xc9, 0x1a, 0x7e, 0xc5, 0x76, 0x47, 0xe3, 0x93, 0x40, 0x57, 0x34, 0x0b, 0x4c,
	0xf4, 0x08, 0xd5, 0xa5, 0x65, 0x92, 0xf8, 0x27, 0x4e, 0xec, 0x53, 0xf0,
};
static const uint8_t sha3_512_64a[SHA3_512_LEN] = {
	0x21, 0x41, 0xe9, 0x4c, 0x71, 0x99, 0x55, 0x87, 0x2c, 0x45, 0x5c, 0x83, 0xeb,
	0x83, 0xe7, 0x61, 0x8a, 0x9b, 0x52, 0x3a, 0x0e, 0xe9, 0xf1, 0x18, 0xe7, 0x94,
	0xfb, 0xff, 0x8b, 0x14, 0x85, 0x45, 0xc8, 0xe8, 0xca, 0xab, 0xef, 0x08, 0xd8,
	0xcf, 0xdb, 0x1d, 0xfb, 0x36, 0xb4, 0xdd, 0x81, 0xcc, 0x48, 0xbf, 0xc7, 0x7e,
	0x7f, 0x85, 0x63, 0x21, 0x97, 0xb8, 0x82, 0xfd, 0x9c, 0x43, 0x84, 0xe0,
};

static uint8_t bench_buf[64 * 1024] __aligned(IO_ALIGNMENT_BYTES);

static inline const struct device *get_crypto_dev(void)
{
#ifdef CRYPTO_DRV_NAME
	return device_get_binding(CRYPTO_DRV_NAME);
#else
	return DEVICE_DT_GET_ONE(CRYPTO_DEV_COMPAT);
#endif
}

static int do_hash_compute(struct hash_ctx *ctx, const uint8_t *in, size_t in_len, uint8_t *out)
{
	struct hash_pkt pkt = {
		.in_buf = in,
		.in_len = in_len,
		.out_buf = out,
	};

	return hash_compute(ctx, &pkt);
}

static int run_kat(const struct device *dev)
{
	uint8_t out[SHA256_LEN] __aligned(IO_ALIGNMENT_BYTES) = {0};
	int rc;
	struct hash_ctx ctx;

	ctx = (struct hash_ctx){
		.flags = CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
	};
	rc = hash_begin_session(dev, &ctx, CRYPTO_HASH_ALGO_SHA256);
	if (rc) {
		printk("FAIL: hash_begin_session(SHA256) rc=%d\n", rc);
		return rc;
	}

	rc = do_hash_compute(&ctx, msg_abc, sizeof(msg_abc), out);
	if (rc || memcmp(out, sha256_abc, SHA256_LEN) != 0) {
		printk("FAIL: SHA256('abc') rc=%d\n", rc);
		hash_free_session(dev, &ctx);
		return rc ? rc : -EINVAL;
	}
	printk("PASS: SHA256('abc')\n");

	rc = do_hash_compute(&ctx, msg_64a, sizeof(msg_64a), out);
	if (rc || memcmp(out, sha256_64a, SHA256_LEN) != 0) {
		printk("FAIL: SHA256(64 x 'a') rc=%d\n", rc);
		hash_free_session(dev, &ctx);
		return rc ? rc : -EINVAL;
	}
	printk("PASS: SHA256(64 x 'a')\n");

	hash_free_session(dev, &ctx);
	printk("PASS: SHA256 KATs (abc, 64 x 'a')\n");
	return 0;
}

static int run_sha3_512_kat(const struct device *dev)
{
	uint8_t out[SHA3_512_LEN] __aligned(IO_ALIGNMENT_BYTES) = {0};
	struct hash_ctx ctx = {
		.flags = CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
	};
	int rc;

	rc = hash_begin_session(dev, &ctx, CRYPTO_HASH_ALGO_SHA3_512);
	if (rc == -ENOTSUP) {
		printk("SKIP: SHA3-512 is not supported\n");
		return 0;
	}
	if (rc) {
		printk("FAIL: hash_begin_session(SHA3-512) rc=%d\n", rc);
		return rc;
	}

	rc = do_hash_compute(&ctx, msg_abc, sizeof(msg_abc), out);
	if (rc || memcmp(out, sha3_512_abc, SHA3_512_LEN) != 0) {
		printk("FAIL: SHA3-512('abc') rc=%d\n", rc);
		hash_free_session(dev, &ctx);
		return rc ? rc : -EINVAL;
	}
	printk("PASS: SHA3-512('abc')\n");

	rc = do_hash_compute(&ctx, msg_64a, sizeof(msg_64a), out);
	if (rc || memcmp(out, sha3_512_64a, SHA3_512_LEN) != 0) {
		printk("FAIL: SHA3-512(64 x 'a') rc=%d\n", rc);
		hash_free_session(dev, &ctx);
		return rc ? rc : -EINVAL;
	}
	printk("PASS: SHA3-512(64 x 'a')\n");

	hash_free_session(dev, &ctx);
	printk("PASS: SHA3-512 KATs (abc, 64 x 'a')\n");
	return 0;
}

static int run_bench_case(const struct device *dev, enum hash_algo algo, const char *name,
			  size_t msg_len, int loops)
{
	struct hash_ctx ctx = {
		.flags = CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
	};
	uint8_t out[SHA3_512_LEN] __aligned(IO_ALIGNMENT_BYTES);
	uint64_t total_bytes = (uint64_t)msg_len * (uint64_t)loops;
	uint64_t dt_cycles;
	uint64_t cps = sys_clock_hw_cycles_per_sec();
	uint64_t bps;
	uint32_t t0;
	uint32_t t1;
	int rc;
	int cpu_permille;

	rc = hash_begin_session(dev, &ctx, algo);
	if (rc) {
		if (rc != -ENOTSUP) {
			printk("bench %s: hash_begin_session rc=%d\n", name, rc);
		}
		return rc;
	}

	/* Warm-up */
	rc = do_hash_compute(&ctx, bench_buf, msg_len, out);
	if (rc) {
		printk("bench %s: warmup failed rc=%d\n", name, rc);
		hash_free_session(dev, &ctx);
		return rc;
	}

	(void)cpu_load_get(true);
	t0 = k_cycle_get_32();
	for (int i = 0; i < loops; i++) {
		rc = do_hash_compute(&ctx, bench_buf, msg_len, out);
		if (rc) {
			printk("bench %s: loop failed rc=%d at i=%d\n", name, rc, i);
			hash_free_session(dev, &ctx);
			return rc;
		}
	}
	t1 = k_cycle_get_32();
	cpu_permille = cpu_load_get(true);

	hash_free_session(dev, &ctx);

	dt_cycles = (uint32_t)(t1 - t0);
	bps = (total_bytes * cps) / dt_cycles;

	if (cpu_permille >= 0) {
		printk("BENCH %s len=%u loops=%d cycles=%llu cpb=%u MBps=%u.%03u CPU=%d.%d%%\n",
		       name, (unsigned int)msg_len, loops, (unsigned long long)dt_cycles,
		       (unsigned int)(dt_cycles / total_bytes), (unsigned int)(bps / 1000000ULL),
		       (unsigned int)((bps % 1000000ULL) / 1000ULL), cpu_permille / 10,
		       cpu_permille % 10);
	} else {
		printk("BENCH %s len=%u loops=%d cycles=%llu cpb=%u MBps=%u.%03u CPU=n/a(rc=%d)\n",
		       name, (unsigned int)msg_len, loops, (unsigned long long)dt_cycles,
		       (unsigned int)(dt_cycles / total_bytes), (unsigned int)(bps / 1000000ULL),
		       (unsigned int)((bps % 1000000ULL) / 1000ULL), cpu_permille);
	}

	return 0;
}

static int run_bench_suite(const struct device *dev, enum hash_algo algo, const char *name)
{
	int rc;

	rc = run_bench_case(dev, algo, name, 64, 2000);
	if (rc) {
		return rc;
	}
	rc = run_bench_case(dev, algo, name, 256, 1000);
	if (rc) {
		return rc;
	}
	rc = run_bench_case(dev, algo, name, 1024, 500);
	if (rc) {
		return rc;
	}
	rc = run_bench_case(dev, algo, name, 4096, 200);
	if (rc) {
		return rc;
	}
	rc = run_bench_case(dev, algo, name, 16384, 100);
	if (rc) {
		return rc;
	}

	return run_bench_case(dev, algo, name, 65536, 40);
}

int main(void)
{
	const struct device *dev = get_crypto_dev();
	int rc;

	if (!device_is_ready(dev)) {
		printk("Crypto device not ready\n");
		return 0;
	}

#ifdef CONFIG_CRYPTO_MBEDTLS_SHIM
	printk("Backend: MBEDTLS_SHIM\n");
#else
	printk("Backend: HW_HASH_DRIVER\n");
#endif

	printk("Hash HW test on device: %s\n", dev->name);
	printk("HWCAPS: 0x%x\n", (unsigned int)crypto_query_hwcaps(dev));

	for (size_t i = 0; i < sizeof(bench_buf); i++) {
		bench_buf[i] = (uint8_t)(i & 0xff);
	}

	rc = run_kat(dev);
	if (rc) {
		printk("Functional tests FAILED (%d)\n", rc);
		return 0;
	}
	rc = run_sha3_512_kat(dev);
	if (rc) {
		printk("SHA3-512 functional test failed (%d)\n", rc);
		return 0;
	}

	printk("Starting SHA256 performance tests\n");
	rc = run_bench_suite(dev, CRYPTO_HASH_ALGO_SHA256, "SHA256");
	if (rc) {
		printk("SHA256 performance tests failed (rc=%d)\n", rc);
	}

	printk("Starting SHA3-512 performance tests\n");
	rc = run_bench_suite(dev, CRYPTO_HASH_ALGO_SHA3_512, "SHA3-512");
	if (rc == -ENOTSUP) {
		printk("SKIP: SHA3-512 performance tests are not supported\n");
	} else if (rc) {
		printk("SHA3-512 performance tests failed (rc=%d)\n", rc);
	}
	printk("Done\n");

	return 0;
}
