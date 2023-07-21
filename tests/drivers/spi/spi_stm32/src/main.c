/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/drivers/spi.h>

#include "stm32_spi_iface.h"
#include "spi_ll_stm32.h"

#include "mocks/stm32_spi_iface_mocks.h"

#include "spi_setup.h"

DEFINE_FFF_GLOBALS;

extern int spi_stm32_transceive(const struct device *dev,
                                const struct spi_config *config,
                                const struct spi_buf_set *tx_bufs,
                                const struct spi_buf_set *rx_bufs);

static void reset_fakes(void* args)
{
    RESET_FAKE(ll_func_is_active_master_transfer);
    FFF_RESET_HISTORY();
}

static void setup_test_case(void* args)
{
    // These mocks must always return true or the transceive function will
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
