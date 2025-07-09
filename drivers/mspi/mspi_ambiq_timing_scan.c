/*
 * Copyright (c) 2025, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mspi.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/flash.h>

#define LOG_LEVEL CONFIG_MSPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mspi_ambiq_timing_scan);

#include "mspi_ambiq.h"

BUILD_ASSERT(CONFIG_MSPI_AMBIQ_TIMING_SCAN_DATA_SIZE %
	     CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE == 0);
#if defined(CONFIG_SOC_SERIES_APOLLO4X)
BUILD_ASSERT(CONFIG_MSPI_AMBIQ_BUFF_ALIGNMENT == 16);
#elif defined(CONFIG_SOC_SERIES_APOLLO5X)
#if CONFIG_DCACHE
BUILD_ASSERT(CONFIG_MSPI_AMBIQ_BUFF_ALIGNMENT == CONFIG_DCACHE_LINE_SIZE);
#endif
#endif

struct longest_ones {
	int start;
	int length;
};

uint8_t txdata_buff[CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE]
__attribute__((section(".ambiq_dma_buff")));
uint8_t rxdata_buff[CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE]
__attribute__((section(".ambiq_dma_buff")));

static int flash_write_data(const struct device *dev,
			    uint32_t             device_addr)
{
	int ret;
	uint32_t num_bytes_left = CONFIG_MSPI_AMBIQ_TIMING_SCAN_DATA_SIZE;
	uint32_t test_bytes = 0;

	ret = flash_erase(dev, device_addr, num_bytes_left);
	if (ret) {
		LOG_ERR("timing scan flash erase failed.\n");
		return ret;
	}

	while (num_bytes_left) {
		if (num_bytes_left > CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE) {
			test_bytes = CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE;
			num_bytes_left -= CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE;
		} else {
			test_bytes = num_bytes_left;
			num_bytes_left = 0;
		}

		LOG_DBG("Write at %08x, size %08x\n", device_addr, test_bytes);
		ret = flash_write(dev, device_addr, txdata_buff, test_bytes);
		if (ret) {
			LOG_ERR("timing scan flash write failed.\n");
			return ret;
		}
		device_addr += test_bytes;
	}
	return 0;
}

static int flash_read_scan(const struct device *dev,
			   uint32_t             device_addr)
{
	int ret;
	uint32_t num_bytes_left = CONFIG_MSPI_AMBIQ_TIMING_SCAN_DATA_SIZE;
	uint32_t test_bytes = 0;

	while (num_bytes_left) {
		if (num_bytes_left > CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE) {
			test_bytes = CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE;
			num_bytes_left -= CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE;
		} else {
			test_bytes = num_bytes_left;
			num_bytes_left = 0;
		}

		LOG_DBG("Read at %08x, size %08x\n", device_addr, test_bytes);
		ret = flash_read(dev, device_addr, rxdata_buff, test_bytes);
		if (ret) {
			LOG_ERR("timing scan flash read failed.\n");
			return ret;
		}
		sys_cache_data_flush_and_invd_all();
		if (memcmp(txdata_buff, rxdata_buff, test_bytes)) {
			return 1;
		}
		device_addr += test_bytes;
	}
	return 0;
}

#define SECTOR_SIZE 1024

static void prepare_test_pattern(uint8_t *buff, uint32_t len)
{
	uint32_t *ui32_tx_ptr = (uint32_t *)buff;
	uint8_t  *ui8_tx_ptr  = (uint8_t *)buff;
	uint32_t pattern_index = 0;
	uint32_t byte_left = len - len % SECTOR_SIZE;

	while (byte_left > 0) {

		switch (pattern_index) {
		case 0:
			/* 0x5555AAAA */
			for (uint32_t i = 0; i < SECTOR_SIZE / 4; i++) {
				ui32_tx_ptr[i] = (0x5555AAAA);
			}
			break;
		case 1:
			/* 0xFFFF0000 */
			for (uint32_t i = 0; i < SECTOR_SIZE / 4; i++) {
				ui32_tx_ptr[i] = (0xFFFF0000);
			}
			break;
		case 2:
			/* walking */
			for (uint32_t i = 0; i < SECTOR_SIZE; i++) {
				ui8_tx_ptr[i] = 0x01 << (i % 8);
			}
			break;
		case 3:
			/* incremental from 1 */
			for (uint32_t i = 0; i < SECTOR_SIZE; i++) {
				ui8_tx_ptr[i] = ((i + 1) & 0xFF);
			}
			break;
		case 4:
			/* decremental from 0xff */
			for (uint32_t i = 0; i < SECTOR_SIZE; i++) {
				ui8_tx_ptr[i] = (0xff - i) & 0xFF;
			}
			break;
		default:
			/* incremental from 1 */
			for (uint32_t i = 0; i < SECTOR_SIZE; i++) {
				ui8_tx_ptr[i] = ((i + 1) & 0xFF);
			}
			break;

		}

		byte_left -= SECTOR_SIZE;
		ui32_tx_ptr += SECTOR_SIZE / 4;
		ui8_tx_ptr += SECTOR_SIZE;
		pattern_index++;
		pattern_index = pattern_index % 5;
	}
}

static void find_longest_ones(const unsigned char *data, int bit_len,
			      struct longest_ones *result)
{
	/* Default result if no 1s are found */
	int current_len   = 0;
	int current_start = -1;

	result->start  = -1;
	result->length = 0;

	for (int i = 0; i < bit_len; i++) {
		int byte_index = i / 8;
		int bit_index  = i % 8;

		if (data[byte_index] & (1 << bit_index)) {
			if (current_len == 0) {
				current_start = i;
			}
			current_len++;
		} else {
			if (current_len > result->length) {
				result->start  = current_start;
				result->length = current_len;
			}
			current_len = 0;
		}
	}
	if (current_len > result->length) {
		result->start  = current_start;
		result->length = current_len;
	}
}

static int find_mid_point(const unsigned char *data, int bit_len)
{
	struct longest_ones result;

	find_longest_ones(data, bit_len, &result);

	if (result.start == -1) {
		return 0;
	}

	return result.start + (result.length - 1) / 2;
}

static int timing_scan_write_read_memc(const struct device             *dev,
				       uint32_t                         device_addr)
{
	uint32_t num_bytes_left = CONFIG_MSPI_AMBIQ_TIMING_SCAN_DATA_SIZE;
	uint32_t test_bytes = 0;

	while (num_bytes_left) {
		if (num_bytes_left > CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE) {
			test_bytes = CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE;
			num_bytes_left -= CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE;
		} else {
			test_bytes = num_bytes_left;
			num_bytes_left = 0;
		}

		LOG_DBG("Write read at %08x, size %08x\n", device_addr, test_bytes);
		memcpy((void *)device_addr, txdata_buff, test_bytes);
		sys_cache_data_flush_and_invd_all();
		memcpy(rxdata_buff, (void *)device_addr, test_bytes);
		if (memcmp(txdata_buff, rxdata_buff, test_bytes)) {
			return 1;
		}
	}
	return 0;
}

static int check_param(struct mspi_ambiq_timing_scan *scan, uint32_t param_mask)
{
	struct mspi_ambiq_timing_scan_range *range = &scan->range;

	if (scan->min_window > range->rxdqs_end - range->rxdqs_start) {
		LOG_ERR("invalid min_window or txdqs, rxdqs scan range.\n");
		return 1;
	}

	if (!(param_mask & MSPI_AMBIQ_SET_RLC) &&
	     (range->rlc_start != 0) && (range->rlc_end != 0)) {
		LOG_ERR("invalid RLC range.\n");
		return 1;
	}

	if (!(param_mask & MSPI_AMBIQ_SET_TXNEG) &&
	     (range->txneg_start != 0) && (range->txneg_end != 0)) {
		LOG_ERR("invalid TXNEG range.\n");
		return 1;
	}

	if (!(param_mask & MSPI_AMBIQ_SET_RXNEG) &&
	     (range->rxneg_start != 0) && (range->rxneg_end != 0)) {
		LOG_ERR("invalid RXNEG range.\n");
		return 1;
	}

	if (!(param_mask & MSPI_AMBIQ_SET_RXCAP) &&
	     (range->rxcap_start != 0) && (range->rxcap_end != 0)) {
		LOG_ERR("invalid RXCAP range.\n");
		return 1;
	}

	if (!(param_mask & MSPI_AMBIQ_SET_TXDQSDLY) &&
	     (range->txdqs_start != 0) && (range->txdqs_end != 0)) {
		LOG_ERR("invalid TXDQSDLY range.\n");
		return 1;
	}

	if (!(param_mask & MSPI_AMBIQ_SET_RXDQSDLY) &&
	     (range->rxdqs_start != 0) && (range->rxdqs_end != 0)) {
		LOG_ERR("invalid RXDQSDLY range.\n");
		return 1;
	}

	return 0;
}

static inline int timing_scan(const struct device           *dev,
			      const struct device           *bus,
			      const struct mspi_dev_id      *dev_id,
			      uint32_t                       param_mask,
			      struct mspi_ambiq_timing_scan *scan,
			      struct mspi_ambiq_timing_cfg  *param,
			      uint32_t                      *max_window)
{
	int ret = 0;
	uint32_t txdqsdelay = 0;
	uint32_t rxdqsdelay = 0;
	uint32_t tx_result = 0;
	uint32_t address = scan->device_addr;
	struct mspi_ambiq_timing_scan_range *range = &scan->range;
	struct longest_ones longest;
	uint32_t rx_res[32];

	memset(rx_res, 0, sizeof(rx_res));

	if (scan->scan_type == MSPI_AMBIQ_TIMING_SCAN_FLASH) {
		ret = flash_write_data(dev, address);
		if (ret) {
			LOG_ERR("Flash write failed, code:%d\n", ret);
			return ret;
		}
	}

	/* LOOP_TXDQSDELAY */
	for (param->ui32TxDQSDelay  = (param_mask & MSPI_AMBIQ_SET_TXDQSDLY) ?
					range->txdqs_start : 0;
		param->ui32TxDQSDelay <= ((param_mask & MSPI_AMBIQ_SET_TXDQSDLY) ?
					   range->txdqs_end : 0);
		param->ui32TxDQSDelay++) {

		/* LOOP_RXDQSDELAY */
		for (param->ui32RxDQSDelay  = (param_mask & MSPI_AMBIQ_SET_RXDQSDLY) ?
						range->rxdqs_start : 0;
			param->ui32RxDQSDelay <= ((param_mask & MSPI_AMBIQ_SET_RXDQSDLY) ?
						   range->rxdqs_end : 0);
			param->ui32RxDQSDelay++) {
			if (scan->scan_type == MSPI_AMBIQ_TIMING_SCAN_MEMC) {
				address = scan->device_addr;
				address += (param->bTxNeg + param->bRxNeg + param->bRxCap
					    + param->ui8TurnAround) *
					    CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE +
					    (param->ui32TxDQSDelay + param->ui32RxDQSDelay) * 2;

				ret = mspi_dev_config(bus, dev_id, MSPI_DEVICE_CONFIG_NONE, NULL);
				if (ret) {
					LOG_ERR("failed to acquire controller, code:%d\n", ret);
					return ret;
				}
				ret = mspi_timing_config(bus, dev_id, param_mask, param);
				if (ret) {
					LOG_ERR("failed to configure mspi timing!!\n");
					return ret;
				}
				/* run data check */
				ret = timing_scan_write_read_memc(dev, address);
			} else if (scan->scan_type == MSPI_AMBIQ_TIMING_SCAN_FLASH) {

				ret = mspi_dev_config(bus, dev_id, MSPI_DEVICE_CONFIG_NONE, NULL);
				if (ret) {
					LOG_ERR("failed to acquire controller, code:%d\n", ret);
					return ret;
				}
				ret = mspi_timing_config(bus, dev_id, param_mask, param);
				if (ret) {
					LOG_ERR("failed to configure mspi timing!!\n");
					return ret;
				}

				/* run data check */
				ret = flash_read_scan(dev, address);
			}
			if (ret == 0) {
				/* data check pass */
				rx_res[param->ui32TxDQSDelay] |= 0x01 << param->ui32RxDQSDelay;
			} else if (ret != 1) {
				return ret;
			}
		}

		if (range->rxdqs_start != range->rxdqs_end &&
		    (param_mask & MSPI_AMBIQ_SET_RXDQSDLY)) {
			find_longest_ones((const unsigned char *)&rx_res[param->ui32TxDQSDelay],
					  32, &longest);
			if (longest.start != -1 && longest.length >= scan->min_window) {
				tx_result |= 0x01 << param->ui32TxDQSDelay;
			}
			LOG_INF("    TxDQSDelay: %d, RxDQSDelay Scan = 0x%08X, Window size = %d\n",
				param->ui32TxDQSDelay, rx_res[param->ui32TxDQSDelay],
				longest.length);
		} else {
			if (rx_res[param->ui32TxDQSDelay] != 0) {
				tx_result |= 0x01 << param->ui32TxDQSDelay;
			}
			LOG_INF("    TxDQSDelay: %d, RxDQSDelay Scan = 0x%08X\n",
				param->ui32TxDQSDelay, rx_res[param->ui32TxDQSDelay]);
		}
	}

	/* Find TXDQSDELAY Value */
	if (range->txdqs_start != range->txdqs_end &&
	    (param_mask & MSPI_AMBIQ_SET_TXDQSDLY)) {
		txdqsdelay = find_mid_point((const unsigned char *)&tx_result, 32);
	} else {
		txdqsdelay = param->ui32TxDQSDelay;
	}

	/* Find RXDQSDELAY Value */
	if (range->rxdqs_start != range->rxdqs_end &&
	    (param_mask & MSPI_AMBIQ_SET_RXDQSDLY)) {
		rxdqsdelay = find_mid_point((const unsigned char *)&rx_res[txdqsdelay], 32);
	} else {
		rxdqsdelay = param->ui32RxDQSDelay;
	}

	find_longest_ones((const unsigned char *)&tx_result, 32, &longest);
	if (*max_window < longest.length ||
	    (range->txdqs_start == range->txdqs_end &&
	     range->rxdqs_start == range->rxdqs_end) ||
	     ((param_mask & (MSPI_AMBIQ_SET_TXDQSDLY | MSPI_AMBIQ_SET_RXDQSDLY)) == 0)) {
		*max_window = longest.length;
		scan->result = *param;
		scan->result.ui32TxDQSDelay = txdqsdelay;
		scan->result.ui32RxDQSDelay = rxdqsdelay;
		LOG_INF("Selected setting: TxNeg=%d, RxNeg=%d, RxCap=%d, Turnaround=%d,"
			"TxDQSDelay=%d, RxDQSDelay=%d\n", param->bTxNeg, param->bRxNeg,
							  param->bRxCap, param->ui8TurnAround,
							  txdqsdelay, rxdqsdelay);
	} else {
		LOG_INF("Candidate setting: TxNeg=%d, RxNeg=%d, RxCap=%d, Turnaround=%d,"
			"TxDQSDelay=%d, RxDQSDelay=%d\n", param->bTxNeg, param->bRxNeg,
							  param->bRxCap, param->ui8TurnAround,
							  txdqsdelay, rxdqsdelay);
	}

	return 0;
}

int mspi_ambiq_timing_scan(const struct device           *dev,
			   const struct device           *bus,
			   const struct mspi_dev_id      *dev_id,
			   uint32_t                       param_mask,
			   struct mspi_ambiq_timing_cfg  *timing,
			   struct mspi_ambiq_timing_scan *scan)
{
	int ret;
	int txneg;
	int rxneg;
	int rxcap;
	int max_window = 0;
	struct mspi_ambiq_timing_cfg param;
	struct mspi_ambiq_timing_scan_range *range = &scan->range;

	if (check_param(scan, param_mask)) {
		return -EINVAL;
	}

	/* Generate data into the buffer */
	prepare_test_pattern(txdata_buff, CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE);
	if (CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE > 64*1024) {
		sys_cache_data_flush_all();
	} else {
		sys_cache_data_flush_range(txdata_buff, CONFIG_MSPI_AMBIQ_TIMING_SCAN_BUFFER_SIZE);
	}

	memset(&param, 0, sizeof(struct mspi_ambiq_timing_cfg));

	/* LOOP_TXNEG */
	for (txneg  = (param_mask & MSPI_AMBIQ_SET_TXNEG) ? range->txneg_start : 0;
	     txneg <= ((param_mask & MSPI_AMBIQ_SET_TXNEG) ? range->txneg_end : 0); txneg++) {
		param.bTxNeg = (bool)txneg;

	/* LOOP_RXNEG */
		for (rxneg  = (param_mask & MSPI_AMBIQ_SET_RXNEG) ? range->rxneg_start : 0;
		     rxneg <= ((param_mask & MSPI_AMBIQ_SET_RXNEG) ? range->rxneg_end : 0);
		     rxneg++) {
			param.bRxNeg = (bool)rxneg;

	/* LOOP_RXCAP */
			for (rxcap  = (param_mask & MSPI_AMBIQ_SET_RXCAP) ? range->rxcap_start : 0;
			     rxcap <= ((param_mask & MSPI_AMBIQ_SET_RXCAP) ? range->rxcap_end : 0);
			     rxcap++) {
				param.bRxCap = (bool)rxcap;
	/* LOOP_TURNAROUND */
				for (param.ui8TurnAround  = (param_mask & MSPI_AMBIQ_SET_RLC) ?
						range->rlc_start + timing->ui8TurnAround : 0;
				     param.ui8TurnAround <= ((param_mask & MSPI_AMBIQ_SET_RLC) ?
						range->rlc_end + timing->ui8TurnAround : 0);
				     param.ui8TurnAround++) {
					param.ui8WriteLatency = timing->ui8WriteLatency;
					LOG_INF("TxNeg=%d, RxNeg=%d, RxCap=%d, Turnaround=%d\n",
						param.bTxNeg, param.bRxNeg, param.bRxCap,
						param.ui8TurnAround);
					ret = timing_scan(dev, bus, dev_id, param_mask,
							  scan, &param, &max_window);
					if (ret) {
						LOG_ERR("Timing scan failed, code:%d\n", ret);
						return ret;
					}

					if (((range->txdqs_start == range->txdqs_end &&
					      range->rxdqs_start == range->rxdqs_end) ||
					      ((param_mask & (MSPI_AMBIQ_SET_TXDQSDLY |
							      MSPI_AMBIQ_SET_RXDQSDLY)) == 0)) &&
						max_window != 0) {
						return 0;
					}
				}

			}
		}
	}

	return 0;
}
