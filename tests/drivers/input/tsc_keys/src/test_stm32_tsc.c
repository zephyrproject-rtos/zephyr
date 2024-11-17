/*
 * Copyright (c) 2024 Arif Balik <arifbalik@outlook.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Verify STM32 TSC peripheral is configured properly and can be started
 *
 * @details
 * This test requires an external connection on the stm32u083c_dk board. The
 * pin GPIOA 10 should be connected to GPIOD 2 manually so that sync signal can
 * be generated. Also make sure to press TS1 pad on the board in order to
 * generate touch signal on test 5.
 * - Test Steps
 *   -# Get a TSC device
 *   -# Verify the device is ready
 *   -# Verify MIMO region with device tree values
 *   -# Test the acquisition in polling mode
 *   -# Test the acquisition in interrupt mode
 * - Expected Results
 *   -# The device is ready
 *   -# The device tree values are correctly mapped to the TSC registers
 *   -# The acquisition is successful in polling mode
 *   -# The acquisition is successful in interrupt mode
 *
 */

#include <soc.h>
#include <autoconf.h>
#include <inttypes.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/gpio.h>

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_tsc)
#define TSC_NODE DT_INST(0, st_stm32_tsc)
#else
#error "Could not find an st,stm32-tsc compatible device in DT"
#endif

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

const struct gpio_dt_spec signal_mock = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, signal_gpios);

ZTEST(stm32_tsc, test_1_device_ready)
{
	const struct device *dev = DEVICE_DT_GET(TSC_NODE);

	zassert_true(device_is_ready(dev), "STM32 TSC device is not ready");
}

ZTEST(stm32_tsc, test_2_cr_reg)
{
	TSC_TypeDef *tsc = (TSC_TypeDef *)DT_REG_ADDR(TSC_NODE);

	volatile const uint32_t *tsc_cr = &tsc->CR;
	const uint32_t pgpsc = LOG2CEIL(DT_PROP(TSC_NODE, st_pulse_generator_prescaler));
	const uint8_t ctph = DT_PROP(TSC_NODE, st_charge_transfer_pulse_high);
	const uint8_t ctpl = DT_PROP(TSC_NODE, st_charge_transfer_pulse_low);
	const uint8_t ssd = DT_PROP(TSC_NODE, st_spread_spectrum_deviation);
	const bool spread_spectrum = DT_PROP(TSC_NODE, st_spread_spectrum);
	const uint8_t sscpsc = DT_PROP(TSC_NODE, st_spread_spectrum_prescaler);
	const uint16_t max_count = LOG2CEIL(DT_PROP(TSC_NODE, st_max_count_value) + 1) - 8;
	const bool iodef = DT_PROP(TSC_NODE, st_iodef_float);
	const bool sync_pol = DT_PROP(TSC_NODE, st_syncpol_rising);
	const bool sync_acq = DT_PROP(TSC_NODE, st_synced_acquisition);

	/* check charge transfer pulse high value (bits 31:28) */
	zassert_equal((*tsc_cr & TSC_CR_CTPL_Msk) >> TSC_CR_CTPL_Pos, ctph - 1,
		      "CTPH value is not correct, expected %d, got %d", ctph - 1,
		      (*tsc_cr & TSC_CR_CTPL_Msk) >> TSC_CR_CTPL_Pos);

	/* check charge transfer pulse low value (bits 27:24) */
	zassert_equal((*tsc_cr & TSC_CR_CTPL_Msk) >> TSC_CR_CTPL_Pos, ctpl - 1,
		      "CTPL value is not correct, expected %d, got %d", ctpl - 1,
		      (*tsc_cr & TSC_CR_CTPL_Msk) >> TSC_CR_CTPL_Pos);

	/* check spread spectrum deviation value (bits 23:17) */
	zassert_equal((*tsc_cr & TSC_CR_SSD_Msk) >> TSC_CR_SSD_Pos, ssd,
		      "SSD value is not correct, expected %d, got %d", ssd,
		      (*tsc_cr & TSC_CR_SSD_Msk) >> TSC_CR_SSD_Pos);

	/* check spread spectrum enable bit (bit 16) */
	if (spread_spectrum) {
		zexpect_true(*tsc_cr & TSC_CR_SSE_Msk);
	} else {
		zexpect_false(*tsc_cr & TSC_CR_SSE_Msk);
	}

	/* check spread spectrum prescaler value (bits 15) */
	if (sscpsc == 2) {
		zexpect_true(*tsc_cr & TSC_CR_SSPSC_Msk);
	} else {
		zexpect_false(*tsc_cr & TSC_CR_SSPSC_Msk);
	}

	/* check pulse generator prescaler value (bits 14:12) */
	zassert_equal((*tsc_cr & TSC_CR_PGPSC_Msk), pgpsc << TSC_CR_PGPSC_Pos,
		      "PGPSC value is not correct, expected %d, got %d", pgpsc,
		      (*tsc_cr & TSC_CR_PGPSC_Msk));

	/* check max count value (bits 7:5) */
	zassert_equal((*tsc_cr & TSC_CR_MCV_Msk), max_count << TSC_CR_MCV_Pos,
		      "MCV value is not correct, expected %d, got %d", max_count,
		      (*tsc_cr & TSC_CR_MCV_Msk));

	/* check I/O default mode bit (bit 4) */
	if (iodef) {
		zexpect_true(*tsc_cr & TSC_CR_IODEF_Msk);
	} else {
		zexpect_false(*tsc_cr & TSC_CR_IODEF_Msk);
	}

	/* check sync polarity bit (bit 3) */
	if (sync_pol) {
		zexpect_true(*tsc_cr & TSC_CR_SYNCPOL_Msk);
	} else {
		zexpect_false(*tsc_cr & TSC_CR_SYNCPOL_Msk);
	}

	/* check sync acquisition bit (bit 2) */
	if (sync_acq) {
		zexpect_true(*tsc_cr & TSC_CR_AM_Msk);
	} else {
		zexpect_false(*tsc_cr & TSC_CR_AM_Msk);
	}

	/* check TSC enable bit (bit 0) */
	zexpect_true(*tsc_cr & TSC_CR_TSCE_Msk);
}

ZTEST(stm32_tsc, test_3_group_registers)
{
	TSC_TypeDef *tsc = (TSC_TypeDef *)DT_REG_ADDR(TSC_NODE);
	volatile const uint32_t *tsc_iohcr = &tsc->IOHCR;
	volatile const uint32_t *tsc_ioscr = &tsc->IOSCR;
	volatile const uint32_t *tsc_ioccr = &tsc->IOCCR;
	volatile const uint32_t *tsc_iogcsr = &tsc->IOGCSR;

#define GET_GROUP_BITS(val) (uint32_t)(((val) & 0x0f) << ((group - 1) * 4))

#define STM32_TSC_GROUP_TEST(node)                                                                 \
	do {                                                                                       \
		const uint8_t group = DT_PROP(node, group);                                        \
		const uint8_t channel_ios = DT_PROP(node, channel_ios);                            \
		const uint8_t sampling_io = DT_PROP(node, sampling_io);                            \
		const bool use_as_shield = DT_PROP(node, st_use_as_shield);                        \
                                                                                                   \
		/* check schmitt trigger hysteresis for enabled I/Os */                            \
		zassert_equal((*tsc_iohcr & GET_GROUP_BITS(channel_ios | sampling_io)), 0,         \
			      "Schmitt trigger hysteresis not disabled, expected %d, got %d", 0,   \
			      (*tsc_iohcr & GET_GROUP_BITS(channel_ios | sampling_io)));           \
                                                                                                   \
		/* check channel I/Os */                                                           \
		zassert_equal(                                                                     \
			(*tsc_ioccr & GET_GROUP_BITS(channel_ios)), GET_GROUP_BITS(channel_ios),   \
			"Channel I/Os value is not correct, expected %d, got %d",                  \
			GET_GROUP_BITS(channel_ios), (*tsc_ioccr & GET_GROUP_BITS(channel_ios)));  \
                                                                                                   \
		/* check sampling I/O */                                                           \
		zassert_equal(                                                                     \
			(*tsc_ioscr & GET_GROUP_BITS(sampling_io)), GET_GROUP_BITS(sampling_io),   \
			"Sampling I/O value is not correct, expected %d, got %d",                  \
			GET_GROUP_BITS(sampling_io), (*tsc_ioscr & GET_GROUP_BITS(sampling_io)));  \
                                                                                                   \
		/* check enabled groups */                                                         \
		if (use_as_shield) {                                                               \
			zassert_not_equal((*tsc_iogcsr & BIT(group - 1)), BIT(group - 1),          \
					  "Group %d is a shield group and should not be enabled",  \
					  group);                                                  \
		} else {                                                                           \
			zassert_equal((*tsc_iogcsr & BIT(group - 1)), BIT(group - 1),              \
				      "Group %d is not enabled", group);                           \
		}                                                                                  \
	} while (0)

#define GROUP_TEST_RUN(node) STM32_TSC_GROUP_TEST(node);

	DT_FOREACH_CHILD_STATUS_OKAY(TSC_NODE, GROUP_TEST_RUN);
}

static volatile bool tsc_input_received;

static void tsc_input_callback(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(evt);
	ARG_UNUSED(user_data);

	tsc_input_received = true;
}
INPUT_CALLBACK_DEFINE(NULL, tsc_input_callback, NULL);

ZTEST(stm32_tsc, test_5_acquisition_interrupt)
{
	TSC_TypeDef *tsc = (TSC_TypeDef *)DT_REG_ADDR(TSC_NODE);
	volatile const uint32_t *tsc_isr = &tsc->ISR;

	k_sleep(K_MSEC(100));

	/* test ISR register max count error flag */
	zexpect_false((*tsc_isr & TSC_ISR_MCEF_Msk) >> TSC_ISR_MCEF_Pos);

	/* this should fail because of the sync pin */
	zexpect_false(tsc_input_received);

	zexpect_ok(gpio_pin_toggle_dt(&signal_mock));

	/* press the TS1 pad */

	k_sleep(K_MSEC(3000));

	zexpect_true(tsc_input_received);
}
