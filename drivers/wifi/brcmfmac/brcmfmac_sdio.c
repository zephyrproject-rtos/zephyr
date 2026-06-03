/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * SDIO bus primitives + firmware/NVRAM upload helpers. Mirrors Linux's
 * brcmf_sdiod_set_backplane_window / brcmf_sdiod_ramrw and
 * brcmf_sdio_download_code_file / brcmf_sdio_download_nvram.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sd/sdio.h>
#include <zephyr/sys/util.h>

#include "brcmfmac_priv.h"

LOG_MODULE_DECLARE(brcmfmac, CONFIG_WIFI_LOG_LEVEL);

int brcmfmac_sdio_set_backplane_window(struct brcmfmac_data *data, uint32_t addr)
{
	uint32_t bar0 = addr & SBSDIO_SBWINDOW_MASK;
	uint32_t v = bar0 >> 8;
	int ret;

	for (int i = 0; i < 3; i++, v >>= 8) {
		ret = sdio_write_byte(&data->backplane,
				      SBSDIO_FUNC1_SBADDRLOW + i,
				      (uint8_t)(v & 0xFF));
		if (ret != 0) {
			LOG_ERR("SBADDR[%d] write failed: %d", i, ret);
			return ret;
		}
	}
	return 0;
}

int brcmfmac_sdio_backplane_read32(struct brcmfmac_data *data, uint32_t addr,
				   uint32_t *out)
{
	uint8_t buf[4];
	int ret;

	ret = brcmfmac_sdio_set_backplane_window(data, addr);
	if (ret != 0) {
		return ret;
	}

	uint32_t off = (addr & SBSDIO_SB_OFT_ADDR_MASK) | SBSDIO_SB_ACCESS_2_4B_FLAG;

	ret = sdio_read_addr(&data->backplane, off, buf, sizeof(buf));
	if (ret != 0) {
		LOG_ERR("backplane_read32(0x%08x): %d", addr, ret);
		return ret;
	}

	*out = (uint32_t)buf[0] |
	       ((uint32_t)buf[1] << 8) |
	       ((uint32_t)buf[2] << 16) |
	       ((uint32_t)buf[3] << 24);
	return 0;
}

int brcmfmac_sdio_backplane_write32(struct brcmfmac_data *data, uint32_t addr,
				    uint32_t val)
{
	uint8_t buf[4];
	int ret;

	ret = brcmfmac_sdio_set_backplane_window(data, addr);
	if (ret != 0) {
		return ret;
	}

	uint32_t off = (addr & SBSDIO_SB_OFT_ADDR_MASK) | SBSDIO_SB_ACCESS_2_4B_FLAG;

	buf[0] = (uint8_t)(val & 0xFF);
	buf[1] = (uint8_t)((val >> 8) & 0xFF);
	buf[2] = (uint8_t)((val >> 16) & 0xFF);
	buf[3] = (uint8_t)((val >> 24) & 0xFF);

	ret = sdio_write_addr(&data->backplane, off, buf, sizeof(buf));
	if (ret != 0) {
		LOG_ERR("backplane_write32(0x%08x): %d", addr, ret);
		return ret;
	}
	return 0;
}

int brcmfmac_sdio_backplane_read_bytes(struct brcmfmac_data *data, uint32_t addr,
				       uint8_t *buf, uint32_t len)
{
	int ret = brcmfmac_sdio_set_backplane_window(data, addr);

	if (ret != 0) {
		return ret;
	}
	uint32_t off = (addr & SBSDIO_SB_OFT_ADDR_MASK) | SBSDIO_SB_ACCESS_2_4B_FLAG;

	ret = sdio_read_addr(&data->backplane, off, buf, len);
	if (ret != 0) {
		LOG_ERR("backplane_read_bytes(0x%08x, %u): %d", addr, len, ret);
	}
	return ret;
}

int brcmfmac_sdio_backplane_write_bytes(struct brcmfmac_data *data, uint32_t addr,
					const uint8_t *buf, uint32_t len)
{
	int ret = brcmfmac_sdio_set_backplane_window(data, addr);

	if (ret != 0) {
		return ret;
	}
	uint32_t off = (addr & SBSDIO_SB_OFT_ADDR_MASK) | SBSDIO_SB_ACCESS_2_4B_FLAG;
	/* Zephyr's sdio_write_addr takes a non-const buffer but doesn't mutate. */
	ret = sdio_write_addr(&data->backplane, off, (uint8_t *)buf, len);
	if (ret != 0) {
		LOG_ERR("backplane_write_bytes(0x%08x, %u): %d", addr, len, ret);
	}
	return ret;
}

/* Bulk RAM read/write across the F1 backplane window. Each chunk:
 *   1. Slide SBADDR to cover the current chip-side address.
 *   2. Transfer up to one F1 window (32 KiB), capped at 511 blocks per
 *      CMD53 -- 512 blocks wedges this chip's SDIO state machine.
 *   3. Trail with a byte-mode 4-byte transfer for 1..3-byte remainders
 *      (F1->backplane 4B_ACCESS mode requires 4-aligned counts).
 */
int brcmfmac_sdio_ramrw(struct brcmfmac_data *data, bool write,
			uint32_t chip_addr, uint8_t *buf, uint32_t size)
{
	while (size > 0) {
		uint32_t sdaddr = chip_addr & SBSDIO_SB_OFT_ADDR_MASK;
		uint32_t window_left = SBSDIO_SB_OFT_ADDR_LIMIT - sdaddr;
		uint32_t chunk = MIN(size, window_left);

		if (chunk > BRCMFMAC_MAX_CMD53_BLOCK_BYTES) {
			chunk = BRCMFMAC_MAX_CMD53_BLOCK_BYTES;
		}
		uint32_t aligned_chunk = chunk & ~3u;
		uint32_t tail = chunk - aligned_chunk;

		int ret = brcmfmac_sdio_set_backplane_window(data, chip_addr);

		if (ret != 0) {
			LOG_ERR("ramrw: SBADDR set @ chip 0x%08x failed: %d",
				chip_addr, ret);
			return ret;
		}

		uint32_t off = sdaddr | SBSDIO_SB_ACCESS_2_4B_FLAG;

		if (aligned_chunk > 0) {
			ret = write
				? sdio_write_addr(&data->backplane, off, buf, aligned_chunk)
				: sdio_read_addr(&data->backplane, off, buf, aligned_chunk);
			if (ret != 0) {
				LOG_ERR("ramrw: %s @ chip 0x%08x len %u failed: %d",
					write ? "write" : "read",
					chip_addr, aligned_chunk, ret);
				return ret;
			}
		}

		if (tail > 0) {
			uint8_t tail_buf[4] __aligned(4) = {0};
			uint32_t tail_off = off + aligned_chunk;

			if (write) {
				memcpy(tail_buf, buf + aligned_chunk, tail);
				ret = sdio_write_addr(&data->backplane, tail_off,
						      tail_buf, sizeof(tail_buf));
			} else {
				ret = sdio_read_addr(&data->backplane, tail_off,
						     tail_buf, sizeof(tail_buf));
				if (ret == 0) {
					memcpy(buf + aligned_chunk, tail_buf, tail);
				}
			}
			if (ret != 0) {
				LOG_ERR("ramrw: %s tail @ chip 0x%08x len %u failed: %d",
					write ? "write" : "read",
					chip_addr + aligned_chunk, tail, ret);
				return ret;
			}
		}

		buf += chunk;
		chip_addr += chunk;
		size -= chunk;
	}
	return 0;
}

/* Read back chip RAM, compare against `expected`. Chunked through a
 * static scratch buffer to avoid stack pressure on the firmware verify.
 */
static int brcmfmac_sdio_verify_memory(struct brcmfmac_data *data,
				       uint32_t chip_addr,
				       const uint8_t *expected, uint32_t size)
{
	static uint8_t readback[1024] __aligned(CONFIG_DCACHE_LINE_SIZE);
	uint32_t verified = 0;

	while (size > 0) {
		uint32_t chunk = MIN(size, (uint32_t)sizeof(readback));
		int ret = brcmfmac_sdio_ramrw(data, false, chip_addr, readback, chunk);

		if (ret != 0) {
			return ret;
		}
		for (uint32_t i = 0; i < chunk; i++) {
			if (readback[i] != expected[i]) {
				LOG_ERR("verify: +%u chip=0x%08x exp=0x%02x got=0x%02x",
					verified + i, chip_addr + i,
					expected[i], readback[i]);
				return -EIO;
			}
		}
		chip_addr += chunk;
		expected += chunk;
		size -= chunk;
		verified += chunk;
	}
	return 0;
}

/* Convert key=value text NVRAM into the chip-format the firmware expects:
 *   - Skip '#' comments, empty lines, leading/trailing whitespace.
 *   - Concat valid lines NUL-separated.
 *   - Pad to 4-byte alignment with NULs.
 *   - Append 4-byte LE token: (~n << 16) | (n & 0xFFFF), n = padded_len/4.
 * Returns bytes written, or negative errno.
 */
static int brcmfmac_sdio_nvram_strip(const uint8_t *in, uint32_t in_len,
				     uint8_t *out, uint32_t out_capacity)
{
	uint32_t op = 0;
	uint32_t ip = 0;
	const uint32_t need_tail = 4 + 3;

	while (ip < in_len) {
		while (ip < in_len &&
		       (in[ip] == ' ' || in[ip] == '\t' ||
			in[ip] == '\r' || in[ip] == '\n')) {
			ip++;
		}
		if (ip >= in_len) {
			break;
		}
		if (in[ip] == '#') {
			while (ip < in_len && in[ip] != '\n') {
				ip++;
			}
			continue;
		}

		uint32_t line_start = op;

		while (ip < in_len && in[ip] != '\n' && in[ip] != '\r') {
			if (op + need_tail >= out_capacity) {
				LOG_ERR("nvram_strip: out overflow");
				return -EOVERFLOW;
			}
			out[op++] = in[ip++];
		}
		while (op > line_start &&
		       (out[op - 1] == ' ' || out[op - 1] == '\t')) {
			op--;
		}
		if (op > line_start) {
			out[op++] = '\0';
		}
	}

	uint32_t padded = (op + 3) & ~3u;

	if (padded + 4 > out_capacity) {
		return -EOVERFLOW;
	}
	while (op < padded) {
		out[op++] = '\0';
	}

	uint32_t n = padded / 4;
	uint32_t token = (~n << 16) | (n & 0xFFFF);

	out[op++] = (uint8_t)(token & 0xFF);
	out[op++] = (uint8_t)((token >> 8) & 0xFF);
	out[op++] = (uint8_t)((token >> 16) & 0xFF);
	out[op++] = (uint8_t)((token >> 24) & 0xFF);

	return (int)op;
}

int brcmfmac_sdio_fw_upload(struct brcmfmac_data *data)
{
	int64_t t0 = k_uptime_get();
	int ret = brcmfmac_sdio_ramrw(data, true, data->ram_base,
				      (uint8_t *)brcmfmac_fw, brcmfmac_fw_len);
	if (ret != 0) {
		LOG_ERR("fw upload failed: %d", ret);
		return ret;
	}
	int64_t t1 = k_uptime_get();

	LOG_INF("fw upload OK in %lld ms (%u bytes)",
		(long long)(t1 - t0), brcmfmac_fw_len);

	ret = brcmfmac_sdio_verify_memory(data, data->ram_base, brcmfmac_fw, brcmfmac_fw_len);
	if (ret != 0) {
		LOG_ERR("fw verify failed: %d", ret);
		return ret;
	}
	LOG_DBG("fw verify OK in %lld ms", (long long)(k_uptime_get() - t1));

	if (data->ram_base != 0 && brcmfmac_fw_len >= 4) {
		LOG_DBG("fw_upload: set reset vector");
		ret = brcmfmac_sdio_ramrw(data, true, 0, (uint8_t *)brcmfmac_fw, 4);
		if (ret != 0) {
			LOG_ERR("fw_upload: set reset vector failed: %d", ret);
			return ret;
		}
	}

	return 0;
}

int brcmfmac_sdio_nvram_upload(struct brcmfmac_data *data)
{
	/* Sized to fit the largest NVRAM currently shipped in linux-firmware
	 * (3099 bytes after the footer); rounds up to 4 KiB.
	 */
	static uint8_t nvram_buf[4096] __aligned(CONFIG_DCACHE_LINE_SIZE);
	int stripped = brcmfmac_sdio_nvram_strip(brcmfmac_nvram, brcmfmac_nvram_len,
						nvram_buf, sizeof(nvram_buf));
	if (stripped < 0) {
		LOG_ERR("nvram_strip failed: %d", stripped);
		return stripped;
	}
	LOG_DBG("nvram strip: %u -> %d bytes (incl. token footer)",
		brcmfmac_nvram_len, stripped);

	const uint32_t nvram_addr = data->ram_base + data->ram_size - (uint32_t)stripped;

	int64_t t0 = k_uptime_get();
	int ret = brcmfmac_sdio_ramrw(data, true, nvram_addr,
				      nvram_buf, (uint32_t)stripped);
	if (ret != 0) {
		LOG_ERR("nvram upload failed: %d", ret);
		return ret;
	}
	int64_t t1 = k_uptime_get();

	LOG_DBG("nvram upload OK @ chip 0x%08x in %lld ms",
		nvram_addr, (long long)(t1 - t0));

	ret = brcmfmac_sdio_verify_memory(data, nvram_addr, nvram_buf,
					  (uint32_t)stripped);
	if (ret != 0) {
		LOG_ERR("nvram verify failed: %d", ret);
		return ret;
	}
	LOG_DBG("nvram verify OK in %lld ms", (long long)(k_uptime_get() - t1));
	return 0;
}
