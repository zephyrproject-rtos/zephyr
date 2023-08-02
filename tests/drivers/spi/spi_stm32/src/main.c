/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/drivers/spi.h>

#include "stm32_spi_iface.h"
#include "spi_ll_stm32.h"

#include "mocks/stm32_spi_iface_mocks.h"
#include "mocks/gpio_mocks.h"

#include "spi_setup.h"

DEFINE_FFF_GLOBALS;

extern int spi_stm32_transceive(const struct device *dev,
                                const struct spi_config *config,
                                const struct spi_buf_set *tx_bufs,
                                const struct spi_buf_set *rx_bufs);

/*
 * Helper function to check if a ptr. is in a list of ptrs.
 *
 */
static bool is_ptr_in_list(const void* ptr,
                          const void* ptr_list[],
                          size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (ptr_list[i] == ptr) {
            return true;
        }
    }
    return false;
}

/*
 * Helper function to retrieve the idx. of the second occurence of a ptr. is in
 * a list of ptrs.
 *
 */
static int find_ptr_second_occurrence(const void* ptr,
                                      const void* ptr_list[],
                                      size_t len)
{
    bool first_found = false;
    for (size_t i = 0; i < len; i++) {
        if (!first_found && ptr_list[i] == ptr) {
            first_found = true;
        } else if (ptr_list[i] == ptr) {
            return i;
        }
    }
    return -1;
}

static void reset_fakes(void* args)
{
    RESET_FAKE(ll_func_is_active_master_transfer);
    RESET_FAKE(ll_func_tx_is_empty);
    RESET_FAKE(ll_func_rx_is_not_empty);
    RESET_FAKE(ll_func_set_polarity);
    RESET_FAKE(ll_func_set_clock_phase);
    RESET_FAKE(ll_func_set_bit_order);
    RESET_FAKE(ll_func_disable_crc);
    RESET_FAKE(ll_func_set_internal_ss_mode_high);
    RESET_FAKE(ll_func_set_nss_mode);
    RESET_FAKE(ll_func_set_mode);
    RESET_FAKE(ll_func_set_data_width);
    RESET_FAKE(ll_func_set_transfer_direction_full_duplex);
    RESET_FAKE(ll_func_set_fifo_threshold_8bit);
    RESET_FAKE(ll_func_enable_spi);
    RESET_FAKE(ll_func_start_master_transfer);
    RESET_FAKE(gpio_port_clear_bits_raw);
    RESET_FAKE(gpio_port_set_bits_raw);

    FFF_RESET_HISTORY();
}

static void setup_test_case(void* args)
{
    // This mocks must always return true or the transceive function will
    // loop for ever waiting for them.
    ll_func_is_active_master_transfer_fake.return_val = true;
    ll_func_tx_is_empty_fake.return_val = true;
    ll_func_rx_is_not_empty_fake.return_val = true;
}

static void* setup_test_suite(void)
{
    reset_fakes(NULL);
    return NULL;
}

ZTEST_SUITE(test_spi_common,
            NULL,
            setup_test_suite,
            setup_test_case,
            reset_fakes,
            NULL);

ZTEST(test_spi_common, test_transceive_can_set_cpol0)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.operation &= ~SPI_MODE_CPOL;

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_equal(ll_func_set_polarity_fake.call_count, 1, NULL);
    zassert_equal_ptr(ll_func_set_polarity_fake.arg0_val,
                      spi_setup_get_native_dev(&sps),
                      NULL);
    zassert_equal(ll_func_set_polarity_fake.arg1_val, STM32_SPI_CPOL_0, NULL);

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_set_cpol1)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.operation |= SPI_MODE_CPOL;

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_equal(ll_func_set_polarity_fake.call_count, 1, NULL);
    zassert_equal_ptr(ll_func_set_polarity_fake.arg0_val,
                      spi_setup_get_native_dev(&sps),
                      NULL);
    zassert_equal(ll_func_set_polarity_fake.arg1_val, STM32_SPI_CPOL_1, NULL);

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_set_cpha0)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.operation &= ~SPI_MODE_CPHA;

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_equal(ll_func_set_clock_phase_fake.call_count, 1, NULL);
    zassert_equal_ptr(ll_func_set_clock_phase_fake.arg0_val,
                      spi_setup_get_native_dev(&sps),
                      NULL);
    zassert_equal(
        ll_func_set_clock_phase_fake.arg1_val, STM32_SPI_CPHA_0, NULL);

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_set_cpha1)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.operation |= SPI_MODE_CPHA;

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_equal(ll_func_set_clock_phase_fake.call_count, 1, NULL);
    zassert_equal_ptr(ll_func_set_clock_phase_fake.arg0_val,
                      spi_setup_get_native_dev(&sps),
                      NULL);
    zassert_equal(
        ll_func_set_clock_phase_fake.arg1_val, STM32_SPI_CPHA_1, NULL);

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_set_bit_order_lsb_first)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.operation |= SPI_TRANSFER_LSB;

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_equal(ll_func_set_bit_order_fake.call_count, 1, NULL);
    zassert_equal_ptr(ll_func_set_bit_order_fake.arg0_val,
                      spi_setup_get_native_dev(&sps),
                      NULL);
    zassert_equal(
        ll_func_set_bit_order_fake.arg1_val, STM32_SPI_LSB_FIRST, NULL);

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_set_bit_order_msb_first)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.operation &= ~SPI_TRANSFER_LSB;

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_equal(ll_func_set_bit_order_fake.call_count, 1, NULL);
    zassert_equal_ptr(ll_func_set_bit_order_fake.arg0_val,
                      spi_setup_get_native_dev(&sps),
                      NULL);
    zassert_equal(
        ll_func_set_bit_order_fake.arg1_val, STM32_SPI_MSB_FIRST, NULL);

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_set_mode_master)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.operation |= SPI_OP_MODE_MASTER;

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_equal(ll_func_set_mode_fake.call_count, 1, NULL);
    zassert_equal_ptr(
        ll_func_set_mode_fake.arg0_val, spi_setup_get_native_dev(&sps), NULL);
    zassert_equal(
        ll_func_set_mode_fake.arg1_val, STM32_SPI_MASTER, NULL);

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_set_mode_slave)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.operation |= SPI_OP_MODE_SLAVE;

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_equal(ll_func_set_mode_fake.call_count, 1, NULL);
    zassert_equal_ptr(
        ll_func_set_mode_fake.arg0_val, spi_setup_get_native_dev(&sps), NULL);
    zassert_equal(
        ll_func_set_mode_fake.arg1_val, STM32_SPI_SLAVE, NULL);

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_set_data_width_8)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.operation &= ~(SPI_WORD_SIZE_MASK);
    sps.cfg.operation |= SPI_WORD_SET(8);

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_equal(ll_func_set_data_width_fake.call_count, 1, NULL);
    zassert_equal_ptr(ll_func_set_data_width_fake.arg0_val,
                      spi_setup_get_native_dev(&sps),
                      NULL);
    zassert_equal(
        ll_func_set_data_width_fake.arg1_val, STM32_SPI_DATA_WIDTH_8);

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_set_data_width_16)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.operation &= ~(SPI_WORD_SIZE_MASK);
    sps.cfg.operation |= SPI_WORD_SET(16);

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_equal(ll_func_set_data_width_fake.call_count, 1, NULL);
    zassert_equal_ptr(ll_func_set_data_width_fake.arg0_val,
                      spi_setup_get_native_dev(&sps),
                      NULL);
    zassert_equal(
        ll_func_set_data_width_fake.arg1_val, STM32_SPI_DATA_WIDTH_16);

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_set_nss_mode_soft)
{
    spi_setup_t sps = spi_setup_create(1);

    // This is a test case pre condition, so use the C assert
    assert(sps.cfg.cs.gpio.port != NULL);

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_equal(ll_func_set_nss_mode_fake.call_count, 1, NULL);
    zassert_equal_ptr(ll_func_set_nss_mode_fake.arg0_val,
                      spi_setup_get_native_dev(&sps),
                      NULL);
    zassert_equal(
        ll_func_set_nss_mode_fake.arg1_val, STM32_SPI_NSS_SOFT);

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceove_can_set_transfer_direction)
{
    spi_setup_t sps = spi_setup_create(1);

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_equal(ll_func_set_transfer_direction_full_duplex_fake.call_count,
                  1,
                  NULL);
    zassert_equal_ptr(ll_func_set_transfer_direction_full_duplex_fake.arg0_val,
                      spi_setup_get_native_dev(&sps),
                      NULL);

    spi_setup_free(&sps);
}

/*
 * This test checks that all the SPI configuration happens before the SPI device
 * has been enabled. This is very important as if this doesn't happen, SPI won't
 * work.
 *
 */
ZTEST(test_spi_common, test_transceive_all_config_happens_before_enabling_spi)
{
    // List of all configuration-related SPI functions currently used by the
    // SPI driver
    const void* spi_config_functions[] = {
        ll_func_set_baudrate_prescaler,
        ll_func_set_polarity,
        ll_func_set_clock_phase,
        ll_func_set_bit_order,
        ll_func_disable_crc,
        ll_func_set_internal_ss_mode_high,
        ll_func_set_nss_mode,
        ll_func_set_mode,
        ll_func_set_data_width,
        ll_func_set_transfer_direction_full_duplex,
        ll_func_set_fifo_threshold_8bit
    };
    const size_t spi_conf_functs_len =
        sizeof(spi_config_functions) / sizeof(*spi_config_functions);

    spi_setup_t sps = spi_setup_create(1);

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    // Find the idx of the second call to ll_func_enable_spi in the FFF general
    // call history. It's the second and not the first occurence because the SPI
    // driver does an (unnecessary) call to disable_spi before starting to
    // configure it. See SYSWFW-583
    int second_enable_idx = find_ptr_second_occurrence(
        ll_func_enable_spi,
        (const void**)fff.call_history,
        fff.call_history_idx + 1
    );

    // Check that no configure functions are executed after the SPI device has
    // been enabled.
    for (int i = second_enable_idx + 1; i < fff.call_history_idx + 1; i++) {

        bool config_func_found = is_ptr_in_list(
            fff.call_history[i], spi_config_functions, spi_conf_functs_len);

        zassert_false(
            config_func_found,
            "A config. function has been executed after enabling SPI");
    }

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_assert_slave_1)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.cs.gpio.pin = 1;

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_true(gpio_port_clear_bits_raw_fake.call_count == 1);
    zassert_true(
        (const struct device*)gpio_port_clear_bits_raw_fake.arg0_val ==
            sps.cfg.cs.gpio.port);
    zassert_true(
        (gpio_port_pins_t)gpio_port_clear_bits_raw_fake.arg1_val == BIT(1));

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_assert_slave_2)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.cs.gpio.pin = 2;

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_true(gpio_port_clear_bits_raw_fake.call_count == 1);
    zassert_true(
        (const struct device*)gpio_port_clear_bits_raw_fake.arg0_val ==
            sps.cfg.cs.gpio.port);
    zassert_true(
        (gpio_port_pins_t)gpio_port_clear_bits_raw_fake.arg1_val == BIT(2));

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_deassert_slave_1)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.cs.gpio.pin = 3;

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_true(gpio_port_set_bits_raw_fake.call_count == 1);
    zassert_true(
        (const struct device*)gpio_port_set_bits_raw_fake.arg0_val ==
            sps.cfg.cs.gpio.port);
    zassert_true(
        (gpio_port_pins_t)gpio_port_set_bits_raw_fake.arg1_val == BIT(3));

    spi_setup_free(&sps);
}

ZTEST(test_spi_common, test_transceive_can_deassert_slave_2)
{
    spi_setup_t sps = spi_setup_create(1);
    sps.cfg.cs.gpio.pin = 4;

    spi_stm32_transceive(&sps.spi, &sps.cfg, &sps.tx_bufs, &sps.rx_bufs);

    zassert_true(gpio_port_set_bits_raw_fake.call_count == 1);
    zassert_true(
        (const struct device*)gpio_port_set_bits_raw_fake.arg0_val ==
            sps.cfg.cs.gpio.port);
    zassert_true(
        (gpio_port_pins_t)gpio_port_set_bits_raw_fake.arg1_val == BIT(4));

    spi_setup_free(&sps);
}
