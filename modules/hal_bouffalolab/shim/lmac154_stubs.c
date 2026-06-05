/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Link-time stubs for CONFIG_BUILD_ONLY_NO_BLOBS=y builds of the
 * Bouffalo Lab IEEE 802.15.4 driver.  These satisfy the linker but
 * must never execute at runtime.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <zephyr/toolchain.h>

#include <lmac154.h>
#include <lmac154_fpt.h>
#if defined(CONFIG_SOC_SERIES_BL61X)
#include <wl_api.h>
#endif

void lmac154_init(void)
{
	/* Stub */
}

void lmac154_setChannel(lmac154_channel_t channel)
{
	ARG_UNUSED(channel);
}

void lmac154_setTxPower(lmac154_tx_power_t power)
{
	ARG_UNUSED(power);
}

void lmac154_setPanId(uint16_t pid)
{
	ARG_UNUSED(pid);
}

void lmac154_setShortAddr(uint16_t addr)
{
	ARG_UNUSED(addr);
}

void lmac154_setLongAddr(uint8_t *addr)
{
	ARG_UNUSED(addr);
}

void lmac154_enableRx(void)
{
	/* Stub */
}

void lmac154_disableRx(void)
{
	/* Stub */
}

uint8_t lmac154_runCCA(int *rssi)
{
	ARG_UNUSED(rssi);
	return 0;
}

void lmac154_triggerTx(uint8_t *buf, uint8_t len, uint8_t csma)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(csma);
}

void lmac154_setStd2015(bool enable)
{
	ARG_UNUSED(enable);
}

void lmac154_enableHwAutoTxAck(void)
{
	/* Stub */
}

void lmac154_disableFrameTypeFiltering(void)
{
	/* Stub */
}

void lmac154_setRxAcceptPolicy(lmac154_rx_accept_policy_t policy)
{
	ARG_UNUSED(policy);
}

void lmac154_setRxStateWhenIdle(bool enable)
{
	ARG_UNUSED(enable);
}

void lmac154_enableRxPromiscuousMode(uint8_t auto_ack, uint8_t crc_check)
{
	ARG_UNUSED(auto_ack);
	ARG_UNUSED(crc_check);
}

void lmac154_disableRxPromiscuousMode(void)
{
	/* Stub */
}

lmac154_isr_t lmac154_getInterruptHandler(void)
{
	return NULL;
}

#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL70XL)
bool bz_phy_set_tx_power(int dbm)
{
	ARG_UNUSED(dbm);
	return true;
}
#endif

#if defined(CONFIG_SOC_SERIES_BL70X)
void rf702_set_init_tsen_value(int16_t tsen)
{
	ARG_UNUSED(tsen);
}
#endif

#if defined(CONFIG_SOC_SERIES_BL70XL)
void rf_set_init_tsen_value(int16_t tsen)
{
	ARG_UNUSED(tsen);
}

bool bz_phy_optimize_tx_channel(uint32_t freq_mhz)
{
	ARG_UNUSED(freq_mhz);
	return true;
}

void rf_tx_pwr_init(void)
{
	/* Stub */
}
#endif

#if defined(CONFIG_SOC_SERIES_BL61X)
void lmac154_setStd2015Extra(bool isEnable)
{
	ARG_UNUSED(isEnable);
}

void lmac154_setTxRetry(uint32_t num)
{
	ARG_UNUSED(num);
}

void lmac154_fptClear(void)
{
	/* Stub */
}

void lmac154_setEnhAckWaitTime(uint16_t time_us)
{
	ARG_UNUSED(time_us);
}

void lmac154_setFramePendingMode(lmac154_fptMode_t mode)
{
	ARG_UNUSED(mode);
}

void lmac154_enable1stStack(void)
{
	/* Stub */
}

void lmac154_enableCoex(void)
{
	/* Stub */
}

bool lmac154_registerEventCallback(lmac154_stack_idx_t stack_idx,
				   lmac154_rxDoneCallback_t stack_rxDoneCallback)
{
	ARG_UNUSED(stack_idx);
	ARG_UNUSED(stack_rxDoneCallback);
	return true;
}

lmac154_isr_t lmac154_getInterruptCallback(void)
{
	return NULL;
}

char *lmac154_getVersionString(void)
{
	return "";
}

int lmac154_getRSSI(void)
{
	return 0;
}

int lmac154_getLQI(void)
{
	return 0;
}

void lmac154_readRxCrc(uint8_t crc[2])
{
	ARG_UNUSED(crc);
}

int lmac154_triggerParamTx(lmac154_txParam_t *txParam)
{
	ARG_UNUSED(txParam);
	return 0;
}

void lmac154_resetTx(void)
{
	/* Stub */
}

struct wl_cfg_t *wl_cfg_get(uint8_t *rmem)
{
	ARG_UNUSED(rmem);
	return NULL;
}

uint32_t wl_rmem_size_get(void)
{
	return 0;
}

int8_t wl_init(void)
{
	return 0;
}

void wl_rf_set_154_tx_power(uint32_t target_pwr_dbm)
{
	ARG_UNUSED(target_pwr_dbm);
}

void zb_timer_cfg(uint32_t init_time)
{
	ARG_UNUSED(init_time);
}
#endif /* CONFIG_SOC_SERIES_BL61X */
