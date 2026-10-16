/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The packet level 2.4GHz radio model uses the Phy connection of the nRF HW
 * models (NRF_HWLowL), which the nRF RADIO model uses too, so only one of the
 * two radio models can be in use.
 */

#include <stdint.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "nsi_hw_scheduler.h"
#include "NRF_HWLowL.h"
#include "NHW_config.h"
#include "NHW_common_types.h"
#include "irq_ctrl.h"
#include "phy_sync_ctrl.h"
#include "bs_2g4_radio_platform.h"
#include "bs_2g4_radio_if.h"

/* The interrupts go to the CPU the nRF RADIO would interrupt */
static const struct nhw_irq_mapping radio_irq_map[] = NHW_RADIO_INT_MAP;

bs_time_t bsr_plat_phy_time_from_dev(bs_time_t dev_time)
{
	return hwll_phy_time_from_dev(dev_time);
}

bs_time_t bsr_plat_dev_time_from_phy(bs_time_t phy_time)
{
	return hwll_dev_time_from_phy(phy_time);
}

void bsr_plat_irq_raise(unsigned int irq)
{
	hw_irq_ctrl_raise_im(radio_irq_map[0].cntl_inst, irq);
}

void bsr_plat_phy_synced(bs_time_t dev_time)
{
	phy_sync_ctrl_set_last_phy_sync_time(dev_time);
}

void bsr_plat_phy_disconnected(void)
{
	bs_trace_raw_manual_time(3, nsi_hws_get_time(), "The phy disconnected us\n");
	hwll_disconnect_phy_and_exit();
}

/*
 * Tests change the behavior of the radio with the test cheats of the nRF HW
 * models (hw_testcheat_if.h). As the nRF RADIO model is not used with this
 * model, the build wraps them (-Wl,--wrap) to apply them to this model too.
 */
void __real_hw_radio_testcheat_set_tx_power_gain(double power_offset);
void __real_hw_radio_testcheat_set_rx_power_gain(double power_offset);
void __real_hw_radio_testcheat_disable_tx(int64_t count);
void __real_hw_radio_testcheat_disable_rx(int64_t count_dont_sync, int64_t count_fail_crc);

void __wrap_hw_radio_testcheat_set_tx_power_gain(double power_offset)
{
	bsr_testcheat_set_tx_power_gain(power_offset);
	__real_hw_radio_testcheat_set_tx_power_gain(power_offset);
}

void __wrap_hw_radio_testcheat_set_rx_power_gain(double power_offset)
{
	bsr_testcheat_set_rx_power_gain(power_offset);
	__real_hw_radio_testcheat_set_rx_power_gain(power_offset);
}

void __wrap_hw_radio_testcheat_disable_tx(int64_t count)
{
	bsr_testcheat_disable_tx(count);
	__real_hw_radio_testcheat_disable_tx(count);
}

void __wrap_hw_radio_testcheat_disable_rx(int64_t count_dont_sync, int64_t count_fail_crc)
{
	bsr_testcheat_disable_rx(count_dont_sync, count_fail_crc);
	__real_hw_radio_testcheat_disable_rx(count_dont_sync, count_fail_crc);
}
