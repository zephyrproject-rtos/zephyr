/**
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, Tonies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <logging/log.h>
#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/fff.h>
#include <zephyr/ztest_assert.h>

#include "i2c_mock.h"
#include "kernel_mock.h"
#include "sys/util_macro.h"
#include "tc_util.h"
#include "toolchain/gcc.h"
#include "uut.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

LOG_MODULE_REGISTER(test_audio, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define TEST_REG_PAGE_SELECT   0x00
#define TEST_VAL_NORMAL_PAGE   0x00
#define TEST_VAL_DVC_PAGE      0x02
#define TEST_I2C_DATA_BUF_SIZE (32u)

struct audio_fixture {
    struct device dev;
    struct codec_driver_config dev_config;
    struct codec_driver_data dev_data;
    uint8_t i2c_burst_write_dt_fake_data[TEST_I2C_DATA_BUF_SIZE];
};

static void *suite_setup(void)
{
    struct audio_fixture *fixture = malloc(sizeof(struct audio_fixture) + TEST_I2C_DATA_BUF_SIZE);

    zassume_not_null(fixture, NULL);

    fixture->dev.name = "CoDec device";
    fixture->dev.data = &fixture->dev_data;
    fixture->dev.config = &fixture->dev_config;
    fixture->dev_config.i2c.burst_buf = fixture->i2c_burst_write_dt_fake_data;

    return fixture;
}

static void suite_teardown(void *f)
{
    free(f);
}

static void suite_before_rule(void *f)
{
    struct audio_fixture *fixture = (struct audio_fixture *)f;

    struct codec_driver_data *data = fixture->dev.data;
    data->page_sem.count = 1;
}

static int i2c_burst_write_dt_custom_fake(const struct i2c_dt_spec *spec,
                                          uint8_t start_addr __unused, const uint8_t *buf,
                                          uint32_t num_bytes)
{
    zassert_true(num_bytes <= TEST_I2C_DATA_BUF_SIZE);

    memcpy(spec->burst_buf, buf, num_bytes);

    return 0;
}

static void k_sem_give_custom_fake(struct k_sem *sem)
{
    zassert_equal(sem->count, 0);

    sem->count++;
}

static int k_sem_take_custom_fake(struct k_sem *sem, k_timeout_t timeout __unused)
{
    zassert_equal(sem->count, 1);

    sem->count--;

    return 0;
}

void static check_set_output_volume_dvc(int vol, struct audio_fixture *fixture)
{
    const uint32_t expected_len = 4;
    const uint32_t calculated_data = (uint32_t)round(pow(10, ((float)vol / 40.0F)) * (1 << 30));

    i2c_burst_write_dt_fake.custom_fake = i2c_burst_write_dt_custom_fake;
    k_sem_take_fake.custom_fake = k_sem_take_custom_fake;
    k_sem_give_fake.custom_fake = k_sem_give_custom_fake;

    const int result = codec_set_output_volume_dvc(&fixture->dev, vol);
    zassert_equal(result, 0, "Unexpected result");

    // page select
    zassert_equal(i2c_reg_write_byte_dt_fake.call_count, 2);
    zassert_equal(i2c_reg_write_byte_dt_fake.arg1_history[0], TEST_REG_PAGE_SELECT);
    zassert_equal(i2c_reg_write_byte_dt_fake.arg2_history[0], TEST_VAL_DVC_PAGE);
    zassert_equal(i2c_reg_write_byte_dt_fake.arg1_history[1], TEST_REG_PAGE_SELECT);
    zassert_equal(i2c_reg_write_byte_dt_fake.arg2_history[1], TEST_VAL_NORMAL_PAGE);

    zassert_equal(k_sem_take_fake.call_count, 1);
    zassert_equal(k_sem_give_fake.call_count, 1);

    // register
    zassert_equal(i2c_burst_write_dt_fake.call_count, 1);
    zassert_equal(i2c_burst_write_dt_fake.arg3_history[0], expected_len);
    zassert_mem_equal(fixture->i2c_burst_write_dt_fake_data, (uint8_t *)&calculated_data,
                      expected_len, "Volume failed: %.01f", (float)vol / 2.0F);

    if (-220 == vol) {
        const uint8_t expected_data[4] = {0x43, 0x0D, 0x00, 0x00};
        zassert_mem_equal(fixture->i2c_burst_write_dt_fake_data, expected_data, expected_len);
    } else if (0 == vol) {
        const uint8_t expected_data[4] = {0x00, 0x00, 0x00, 0x40};
        zassert_mem_equal(fixture->i2c_burst_write_dt_fake_data, expected_data, expected_len);
    } else if (4 == vol) {
        const uint8_t expected_data[4] = {0xE4, 0x3B, 0x92, 0x50};
        zassert_mem_equal(fixture->i2c_burst_write_dt_fake_data, expected_data, expected_len);
    }

    RESET_FAKE(i2c_burst_write_dt);
    RESET_FAKE(i2c_reg_write_byte_dt);
    RESET_FAKE(k_sem_take);
    RESET_FAKE(k_sem_give);
}

ZTEST_F(audio, test_set_output_volume_dvc)
{
    for (int vol = -220; vol <= 4; vol++) {
        check_set_output_volume_dvc(vol, fixture);
    }
};

void static check_claim_page(int page, struct audio_fixture *fixture)
{
    k_sem_take_fake.custom_fake = k_sem_take_custom_fake;
    k_sem_give_fake.custom_fake = k_sem_give_custom_fake;

    int result = codec_claim_page(&fixture->dev, page);
    zassert_equal(i2c_burst_write_dt_fake.call_count, 0);

    if (page == 0) {
        zassert_equal(k_sem_take_fake.call_count, 0);
        zassert_equal(result, 0, "Unexpected result");
    } else if (page == 2) {
        zassert_equal(result, 0, "Unexpected result");
        zassert_equal(k_sem_take_fake.call_count, 1);
        zassert_equal(i2c_reg_write_byte_dt_fake.call_count, 1);
        zassert_equal(i2c_reg_write_byte_dt_fake.arg1_history[0], TEST_REG_PAGE_SELECT);
        zassert_equal(i2c_reg_write_byte_dt_fake.arg2_history[0], page);
    } else {
        zassert_equal(result, -EINVAL, "Unexpected result");
        zassert_equal(k_sem_take_fake.call_count, 0);
    }

    RESET_FAKE(i2c_reg_write_byte_dt);
    RESET_FAKE(k_sem_take);

    struct codec_driver_data *data = fixture->dev.data;
    data->page_sem.count = 1;
}

ZTEST_F(audio, test_codec_claim_page)
{
    for (int page = -2; page < 4; page++) {
        check_claim_page(page, fixture);
    }
}

void static check_release_page(int page, struct audio_fixture *fixture)
{
    k_sem_take_fake.custom_fake = k_sem_take_custom_fake;
    k_sem_give_fake.custom_fake = k_sem_give_custom_fake;

    struct codec_driver_data *data = fixture->dev.data;
    data->page_sem.count = 0;

    int result = codec_release_page(&fixture->dev, page);
    zassert_equal(i2c_burst_write_dt_fake.call_count, 0);

    if (page == 0) {
        zassert_equal(k_sem_take_fake.call_count, 0);
        zassert_equal(result, 0, "Unexpected result");
    } else if (page == 2) {
        zassert_equal(result, 0, "Unexpected result");
        zassert_equal(k_sem_give_fake.call_count, 1);
        zassert_equal(i2c_reg_write_byte_dt_fake.call_count, 1);
        zassert_equal(i2c_reg_write_byte_dt_fake.arg1_history[0], TEST_REG_PAGE_SELECT);
        zassert_equal(i2c_reg_write_byte_dt_fake.arg2_history[0], TEST_VAL_NORMAL_PAGE);
    } else {
        zassert_equal(result, -EINVAL, "Unexpected result");
        zassert_equal(k_sem_take_fake.call_count, 0);
    }

    RESET_FAKE(i2c_reg_write_byte_dt);
    RESET_FAKE(k_sem_give);
}

ZTEST_F(audio, test_codec_release_page)
{
    for (int page = -2; page < 4; page++) {
        check_release_page(page, fixture);
    }
}

ZTEST_SUITE(audio, NULL, suite_setup, suite_before_rule, NULL, suite_teardown);
