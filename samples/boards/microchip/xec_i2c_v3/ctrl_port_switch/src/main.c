/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/target/eeprom.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/dt-bindings/i2c/mchp-xec-i2c.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

/* #define APP_TEST_LTC2489 */

#define PCA9555_CMD_PORT0_IN  0
#define PCA9555_CMD_PORT1_IN  1u
#define PCA9555_CMD_PORT0_OUT 2u
#define PCA9555_CMD_PORT1_OUT 3u
#define PCA9555_CMD_PORT0_POL 4u
#define PCA9555_CMD_PORT1_POL 5u
#define PCA9555_CMD_PORT0_CFG 6u
#define PCA9555_CMD_PORT1_CFG 7u

/* depends on board jumpers */
#define PCA9555_PORT0_IN_EXPECTED 0xfffcU

#define LTC2489_ADC_CONV_TIME_MS 150
#define LTC2489_ADC_READ_RETRIES 10

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

#define I2C_SMB_GET_DEV(nid) DEVICE_DT_GET(nid),

#define I2C_CTRL0_NODE DT_ALIAS(i2c0)
#define I2C_CTRL1_NODE DT_ALIAS(i2c1)

#define NODE_PCA9555 DT_NODELABEL(pca9555_evb)
#define NODE_LTC2489 DT_NODELABEL(ltc2489_evb)
#define NODE_FRAM    DT_NODELABEL(mb85rc256v_fram)

const struct i2c_dt_spec pca9555_spec = I2C_DT_SPEC_GET(NODE_PCA9555);
#ifdef APP_TEST_LTC2489
const struct i2c_dt_spec ltc2489_spec = I2C_DT_SPEC_GET(NODE_LTC2489);
#endif
const struct i2c_dt_spec mb_fram_spec = I2C_DT_SPEC_GET(NODE_FRAM);

static const struct device *i2c_smb_ctrls[] = {
	DT_FOREACH_STATUS_OKAY(microchip_xec_i2c_v3_bm, I2C_SMB_GET_DEV)};

/* Ports on the controllers */
static const struct device *i2c_smb_ports[] = {
	DT_FOREACH_STATUS_OKAY(microchip_xec_i2c_v3_bm_port, I2C_SMB_GET_DEV)};

struct k_timer minute_timer;

#define APP_I2C_PORT_SWITCH_LOOPS    10000U
#define APP_I2C_CB_PORT_SWITCH_LOOPS 10000U

#define I2C_MAX_MSGS    8
#define I2C_TX_BUF_SIZE 256
#define I2C_RX_BUF_SIZE 256

static struct i2c_msg msgs[I2C_MAX_MSGS];
static uint8_t i2c_tx_buf[I2C_TX_BUF_SIZE];
static uint8_t i2c_rx_buf[I2C_RX_BUF_SIZE];

#ifdef CONFIG_I2C_CALLBACK
static void get_random_fram_offset_size(uint16_t *ofs, uint32_t *nbytes);
#endif

static int pca9555_test1(const struct i2c_dt_spec *dts, uint8_t port, uint16_t *port_value);
#ifdef APP_TEST_LTC2489
static int ltc2489_test1(const struct i2c_dt_spec *dts);
#endif
static int fram_test1(const struct i2c_dt_spec *dts);
static int fram_test2(const struct i2c_dt_spec *dts);

static int test_read(const struct i2c_dt_spec *dts, uint32_t nread);

static volatile bool run;

#ifdef APP_TEST_LTC2489
static volatile bool app_dbg_halt = true;
#endif

#ifdef CONFIG_I2C_CALLBACK
struct app_i2c_cb_s {
	struct k_sem i2c_cb_sem;
	volatile uint32_t i2c_cb_count;
	volatile int i2c_cb_result;
	void *i2c_cb_ud;
};

struct app_i2c_cb_s pca9555_cb_data;
struct app_i2c_cb_s fram_cb_data;

static int app_i2c_cb_init(struct app_i2c_cb_s *p);
static int app_i2c_cb_prep(struct app_i2c_cb_s *p);
static void app_i2c_cb_func(const struct device *i2c_port_dev, int result, void *data);
static int app_i2c_cb_test_pca9555(const struct i2c_dt_spec *dts, uint8_t gpio_port);
static int app_i2c_cb_test_fram(const struct i2c_dt_spec *dts, uint16_t fram_offset,
				uint32_t nbytes);
static int app_i2c_cb_test_fram_mm(const struct i2c_dt_spec *dts, uint16_t fram_offset,
				   uint32_t nbytes);
static int test_read_async(const struct i2c_dt_spec *dts, uint32_t nread, struct app_i2c_cb_s *pcbs,
			   i2c_callback_t cb);
#endif

void minute_timer_cb(struct k_timer *kt)
{
	LOG_INF("10 minutes has elapsed");
}

int main(void)
{
	uint64_t test_loops = 0;
	uint64_t pca9555_errors = 0;
	uint64_t pca9555_errors_logged = 0;
	uint64_t fram_errors = 0;
	uint64_t fram_errors_logged = 0;
	int ctrl_ready_count = 0;
	int port_ready_count = 0;
	int rc = 0;
#ifdef CONFIG_I2C_CALLBACK
	uint32_t num_bytes = 0;
	uint16_t fram_offset;
#endif
	run = false;
	memset((void *)msgs, 0, sizeof(msgs));
	memset(i2c_tx_buf, 0x55, I2C_TX_BUF_SIZE);
	memset(i2c_rx_buf, 0xAA, I2C_RX_BUF_SIZE);

#ifdef CONFIG_BOARD_QUALIFIERS
	LOG_INF("Microchip XEC I2Cv3 ctrl_port_switch: board: %s/%s", CONFIG_BOARD,
		CONFIG_BOARD_QUALIFIERS);
#else
	LOG_INF("Microchip XEC I2Cv3 ctrl_port_switch: board: %s", CONFIG_BOARD);
#endif
#if DT_NODE_HAS_PROP(I2C_CTRL0_NODE, compatible)
	LOG_INF("I2C Ctrl0 compatible: %s", DT_PROP_BY_IDX(I2C_CTRL0_NODE, compatible, 0));
#else
	LOG_INF("I2C Ctrl0 does not have a compatible!");
#endif
#if DT_NODE_HAS_PROP(I2C_CTRL1_NODE, compatible)
	LOG_INF("I2C Ctrl1 compatible: %s", DT_PROP_BY_IDX(I2C_CTRL1_NODE, compatible, 0));
#else
	LOG_INF("I2C Ctrl1 does not have a compatible!");
#endif
	log_flush();

	k_timer_init(&minute_timer, minute_timer_cb, NULL);

#ifdef CONFIG_I2C_CALLBACK
	app_i2c_cb_init(&pca9555_cb_data);
	app_i2c_cb_init(&fram_cb_data);
#endif

	for (size_t i = 0; i < ARRAY_SIZE(i2c_smb_ctrls); i++) {
		const struct device *ctrl_dev = i2c_smb_ctrls[i];

		if (ctrl_dev == NULL) {
			continue;
		}

		if (device_is_ready(ctrl_dev)) {
			ctrl_ready_count++;
			LOG_INF("I2C Controller device %s is ready", ctrl_dev->name);
		} else {
			LOG_ERR("I2C Controller device %s is NOT ready", ctrl_dev->name);
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(i2c_smb_ports); i++) {
		const struct device *port_dev = i2c_smb_ports[i];

		if (port_dev == NULL) {
			continue;
		}

		if (device_is_ready(port_dev)) {
			port_ready_count++;
			LOG_INF("I2C Port device %s is ready", port_dev->name);
		} else {
			LOG_ERR("I2C Port device %s is NOT ready", port_dev->name);
		}
	}

	log_flush();

	if ((ctrl_ready_count > 0) && (port_ready_count > 0)) {
		LOG_INF("Enter test loop");
		run = true;
	} else {
		LOG_ERR("ctrl_ready_count = %u port_ready_count = %u", ctrl_ready_count,
			port_ready_count);
	}

	log_flush();

	rc = test_read(&mb_fram_spec, 1U);
	if (rc == 0) {
		LOG_INF("Single byte read OK");
	} else {
		LOG_ERR("Single byte read returned error (%d)", rc);
	}

	rc = test_read(&mb_fram_spec, 2U);
	if (rc == 0) {
		LOG_INF("Read two bytes OK");
	} else {
		LOG_ERR("Read two bytes returned error (%d)", rc);
	}

	rc = test_read(&mb_fram_spec, 3U);
	if (rc == 0) {
		LOG_INF("Read three bytes OK");
	} else {
		LOG_ERR("Read three bytes returned error (%d)", rc);
	}

	log_flush();

#ifdef CONFIG_I2C_CALLBACK
	(void)app_i2c_cb_test_pca9555(&pca9555_spec, PCA9555_CMD_PORT0_IN);
	log_flush();

	(void)app_i2c_cb_test_fram(&mb_fram_spec, 0x1024U, 64U);
	log_flush();

	(void)test_read_async(&mb_fram_spec, 10U, &fram_cb_data, app_i2c_cb_func);
	log_flush();

	(void)app_i2c_cb_test_fram_mm(&mb_fram_spec, 0x7654U, 32U);
	log_flush();
#endif

	if (run) {
		LOG_INF("Start kernel 10 minute timer and begin infinite port switch test loop");
		k_timer_start(&minute_timer, K_MINUTES(10), K_MINUTES(10));
	}

	while (run) {
		test_loops++;

		rc = pca9555_test1(&pca9555_spec, PCA9555_CMD_PORT0_IN, NULL);
		if (rc != 0) {
			pca9555_errors++;
		}

		rc = fram_test1(&mb_fram_spec);
		if (rc != 0) {
			fram_errors++;
		}

		rc = pca9555_test1(&pca9555_spec, PCA9555_CMD_PORT1_IN, NULL);
		if (rc != 0) {
			pca9555_errors++;
		}

		rc = fram_test2(&mb_fram_spec);
		if (rc != 0) {
			fram_errors++;
		}

#ifdef APP_TEST_LTC2489
		rc = ltc2489_test1(&ltc2489_spec);
		if (rc != 0) {
			LOG_ERR("LTC2489 test error (%d)", rc);
			break;
		}
#endif
		if (pca9555_errors != pca9555_errors_logged) {
			pca9555_errors_logged = pca9555_errors;
			LOG_ERR("PCA9555 test error count %llu", pca9555_errors_logged);
		}

		if (fram_errors != fram_errors_logged) {
			fram_errors_logged = fram_errors;
			LOG_ERR("FRAM test1 error count %llu", fram_errors_logged);
		}

		if (test_loops >= APP_I2C_PORT_SWITCH_LOOPS) {
			break;
		}
	};

	LOG_INF("Executed %llu synchronous port switch loops", test_loops);

#ifdef CONFIG_I2C_CALLBACK
	if (run) {
		pca9555_errors = 0;
		pca9555_errors_logged = 0;
		fram_errors = 0;
		fram_errors_logged = 0;
		test_loops = 0;
		LOG_INF("Begin async test loop");
	}

	while (run) {
		test_loops++;

		get_random_fram_offset_size(&fram_offset, &num_bytes);

		rc = app_i2c_cb_test_pca9555(&pca9555_spec, PCA9555_CMD_PORT0_IN);
		if (rc != 0) {
			pca9555_errors++;
		}

		rc = app_i2c_cb_test_fram(&mb_fram_spec, fram_offset, num_bytes);
		if (rc != 0) {
			fram_errors++;
		}

		if (pca9555_errors != pca9555_errors_logged) {
			pca9555_errors_logged = pca9555_errors;
			LOG_ERR("PCA9555 test error count %llu", pca9555_errors_logged);
		}

		if (fram_errors != fram_errors_logged) {
			fram_errors_logged = fram_errors;
			LOG_ERR("FRAM test1 error count %llu", fram_errors_logged);
		}

		if (test_loops >= APP_I2C_CB_PORT_SWITCH_LOOPS) {
			break;
		}
	};

	LOG_INF("Executed %llu async port switch loops", test_loops);
#endif /* CONFIG_I2C_CALLBACK */
	LOG_INF("Program End");
	log_flush();

	return 0;
}

#ifdef CONFIG_I2C_CALLBACK
static void get_random_fram_offset_size(uint16_t *ofs, uint32_t *nbytes)
{
	uint32_t nb = 512U;
	uint32_t temp = 0;
	uint16_t fram_ofs = 0;

	do {
		sys_rand_get((void *)&temp, 4U);
		fram_ofs = (uint16_t)temp;
	} while (fram_ofs < (0x8000U - 0x100U));

	while ((nb == 0) || (nb > 254U)) {
		sys_rand_get((void *)&temp, 4U);
		nb = temp & 0xfffu;
	}

	*ofs = fram_ofs;
	*nbytes = nb;
}
#endif

static int test_read(const struct i2c_dt_spec *dts, uint32_t nread)
{
	int rc = 0;

	if ((dts == NULL) || (nread > I2C_RX_BUF_SIZE)) {
		LOG_ERR("test read bad i2c DT spec or nread > I2C_RX_BUF_SIZE");
		return -EINVAL;
	}

	memset(i2c_rx_buf, 0xAA, sizeof(i2c_rx_buf));

	rc = i2c_read_dt(dts, i2c_rx_buf, nread);

	return rc;
}

static int pca9555_test1(const struct i2c_dt_spec *dts, uint8_t port, uint16_t *port_value)
{
	int rc = -ENOTSUP;

	if (dts == NULL) {
		LOG_ERR("PCA9555 test1 bad i2c DT spec");
		return -EINVAL;
	}

	memset(i2c_tx_buf, 0x55, sizeof(i2c_tx_buf));
	memset(i2c_rx_buf, 0xAA, sizeof(i2c_rx_buf));

	i2c_tx_buf[0] = port;

	rc = i2c_write_read_dt(dts, i2c_tx_buf, 1U, i2c_rx_buf, 2U);
	if (rc != 0) {
		LOG_ERR("PCA9555 test1 I2C write(1)-read(2) error (%d)", rc);
		return rc;
	}

	if (port_value != NULL) {
		*port_value = ((uint16_t)i2c_rx_buf[1] << 8) | i2c_rx_buf[0];
	}

	return rc;
}

#ifdef APP_TEST_LTC2489
/* LTC2489 is a troublesome device
 * It will NAK its address if it is busy. Power on reset causes it to be busy.
 */
static int ltc2489_test1(const struct i2c_dt_spec *dts)
{
	int rc = -ENOTSUP;
	uint32_t adc_retry_count = 0, reading = 0;

	if (dts == NULL) {
		return -EINVAL;
	}

	/* Read ADC channel 0. Selecting the channel initiates a reading */
	i2c_tx_buf[0] = 0xb0U; /* 1011_0000 selects channel 0 as single ended */

	rc = i2c_write_dt(dts, i2c_tx_buf, 1U);
	if (rc != 0) {
		while (app_dbg_halt) {
			;
		}
		LOG_ERR("I2C write to LTC2489 channel select error (%d)", rc);
		return rc;
	}

	/* LTC2489 will NAK while it is converting */
	k_sleep(K_MSEC(LTC2489_ADC_CONV_TIME_MS));

	do {
		i2c_rx_buf[0] = 0x55U;
		i2c_rx_buf[1] = 0x55U;
		i2c_rx_buf[2] = 0x55U;

		rc = i2c_read_dt(dts, i2c_rx_buf, 3U);
		if (rc != 0) {
			adc_retry_count++;
		}

	} while ((rc != 0) && (adc_retry_count < LTC2489_ADC_READ_RETRIES));

	if (rc == 0) {
		reading = ((uint32_t)i2c_rx_buf[0] + ((uint32_t)i2c_rx_buf[1] << 8) +
			   ((uint32_t)i2c_rx_buf[2] << 16));
		LOG_INF("LTC2489 conversion done: raw reading = 0x%0x", reading);
	} else {
		LOG_ERR("LTC2489 conversion timeout: FAIL");
	}

	return rc;
}
#endif

static int fram_test1(const struct i2c_dt_spec *dts)
{
	int rc = -ENOTSUP;

	if (dts == NULL) {
		LOG_ERR("FRAM test1 bad i2c DT spec");
		return -EINVAL;
	}

	memset(i2c_tx_buf, 0x55, sizeof(i2c_tx_buf));
	memset(i2c_rx_buf, 0xAA, sizeof(i2c_rx_buf));

	i2c_tx_buf[0] = 0x43U;
	i2c_tx_buf[1] = 0x21U;
	i2c_tx_buf[2] = 0x01U;
	i2c_tx_buf[3] = 0x02U;
	i2c_tx_buf[4] = 0x03U;
	i2c_tx_buf[5] = 0x04U;

	rc = i2c_write_dt(dts, i2c_tx_buf, 6U);
	if (rc != 0) {
		LOG_ERR("FRAM test1 write 6 bytes error: (%d)", rc);
		return rc;
	}

	i2c_tx_buf[0] = 0x43U;
	i2c_tx_buf[1] = 0x21U;

	rc = i2c_write_read_dt(dts, i2c_tx_buf, 2U, i2c_rx_buf, 4U);
	if (rc != 0) {
		LOG_ERR("FRAM test1 write(2)-read(4) error (%d)", rc);
		return rc;
	}

	rc = memcmp(&i2c_tx_buf[2], i2c_rx_buf, 4U);
	if (rc != 0) {
		LOG_ERR("FRAM data mismatch");
		rc = -EPERM;
	}

	return rc;
}

static int fram_test2(const struct i2c_dt_spec *dts)
{
	int rc = -ENOTSUP;

	if (dts == NULL) {
		return -EINVAL;
	}

	memset(i2c_tx_buf, 0x55, sizeof(i2c_tx_buf));
	memset(i2c_rx_buf, 0xAA, sizeof(i2c_rx_buf));

	i2c_tx_buf[0] = 0x12U;
	i2c_tx_buf[1] = 0x30U;
	for (uint32_t i = 0; i < 32U; i++) {
		i2c_tx_buf[i + 2U] = (uint8_t)(i % 256U);
	}

	rc = i2c_write_dt(dts, i2c_tx_buf, 34U);
	if (rc != 0) {
		return rc;
	}

	i2c_tx_buf[0] = 0x12U;
	i2c_tx_buf[1] = 0x30U;

	rc = i2c_write_read_dt(dts, i2c_tx_buf, 2U, i2c_rx_buf, 32U);
	if (rc != 0) {
		return rc;
	}

	rc = memcmp(&i2c_tx_buf[2], i2c_rx_buf, 32U);
	if (rc != 0) {
		rc = -EPERM;
	}

	return rc;
}

#ifdef CONFIG_I2C_CALLBACK

static int app_i2c_cb_init(struct app_i2c_cb_s *p)
{
	if (p == NULL) {
		return -EINVAL;
	}

	k_sem_init(&p->i2c_cb_sem, 0, 1);
	p->i2c_cb_count = 0;
	p->i2c_cb_result = 0;
	p->i2c_cb_ud = NULL;

	return 0;
}

static int app_i2c_cb_prep(struct app_i2c_cb_s *p)
{
	if (p == NULL) {
		return -EINVAL;
	}

	k_sem_reset(&p->i2c_cb_sem);
	p->i2c_cb_count = 0;
	p->i2c_cb_result = 0;
	p->i2c_cb_ud = NULL;

	return 0;
}

static void app_i2c_cb_func(const struct device *i2c_port_dev, int result, void *data)
{
	struct app_i2c_cb_s *cbs = data;

	if (cbs != NULL) {
		cbs->i2c_cb_count++;
		cbs->i2c_cb_result = result;
		cbs->i2c_cb_ud = data;

		k_sem_give(&cbs->i2c_cb_sem);
	}
}

static int app_i2c_cb_test_pca9555(const struct i2c_dt_spec *dts, uint8_t gpio_port)
{
	int rc = 0;
	uint16_t gpio_port_value = 0U;

	if (dts == NULL) {
		LOG_ERR("App I2C PCA9555 async test bad i2c_dt_spec");
		return -EINVAL;
	}

	memset(i2c_tx_buf, 0x55, sizeof(i2c_tx_buf));
	memset(i2c_rx_buf, 0xAA, sizeof(i2c_rx_buf));

	i2c_tx_buf[0] = gpio_port;

	app_i2c_cb_prep(&pca9555_cb_data);

	rc = i2c_write_read_cb_dt(dts, msgs, 2U, (const void *)i2c_tx_buf, 1U, (void *)i2c_rx_buf,
				  2U, app_i2c_cb_func, (void *)&pca9555_cb_data);
	if (rc != 0) {
		LOG_ERR("App I2C wr-rd-cb-dt error (%d)", rc);
		return rc;
	}

	rc = k_sem_take(&pca9555_cb_data.i2c_cb_sem, K_MSEC(2000));
	switch (rc) {
	case 0:
		gpio_port_value = ((uint16_t)i2c_rx_buf[1] << 8) | i2c_rx_buf[0];
		if (gpio_port_value != PCA9555_PORT0_IN_EXPECTED) {
			LOG_ERR("PCA9555 input port %u = 0x%04x expected 0x%04x", gpio_port,
				gpio_port_value, PCA9555_PORT0_IN_EXPECTED);
			rc = -EBADMSG;
		}
		break;
	case -EAGAIN:
		LOG_ERR("Take PCA9555 CB semaphore returned -EAGAIN which is timeout");
		rc = -ETIMEDOUT;
		break;
	case -EBUSY:
		LOG_ERR("Take PCA9555 CB semaphore returned -EBUSY");
		break;
	default:
		LOG_ERR("Take PCA9555 CB semaphore returned unexpected error (%d)", rc);
		break;
	}

	return rc;
}

static int app_i2c_cb_test_fram(const struct i2c_dt_spec *dts, uint16_t fram_offset,
				uint32_t nbytes)
{
	int rc = 0;

	if ((dts == NULL) || ((nbytes + 2U) > I2C_RX_BUF_SIZE)) {
		LOG_ERR("App I2C FRAM async test bad i2c_dt_spec or nbytes: %p, %u", dts, nbytes);
		return -EINVAL;
	}

	memset(i2c_tx_buf, 0x55, sizeof(i2c_tx_buf));
	memset(i2c_rx_buf, 0xAA, sizeof(i2c_rx_buf));

	i2c_tx_buf[0] = (uint8_t)(fram_offset >> 8);
	i2c_tx_buf[1] = (uint8_t)(fram_offset >> 0);

	sys_rand_get(&i2c_tx_buf[2], nbytes);

	app_i2c_cb_prep(&fram_cb_data);

	msgs[0].buf = i2c_tx_buf;
	msgs[0].len = nbytes + 2U;
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	rc = i2c_transfer_cb(dts->bus, msgs, 1U, dts->addr, app_i2c_cb_func, (void *)&fram_cb_data);
	if (rc != 0) {
		LOG_ERR("App I2C wr-cb for write error (%d)", rc);
		return rc;
	}

	rc = k_sem_take(&fram_cb_data.i2c_cb_sem, K_MSEC(2000));
	switch (rc) {
	case 0:
		break; /* success */
	case -EAGAIN:
		LOG_ERR("Take FRAM CB semaphore returned -EAGAIN which is timeout");
		rc = -ETIMEDOUT;
		break;
	case -EBUSY:
		LOG_ERR("Take FRAM CB semaphore returned -EBUSY");
		break;
	default:
		LOG_ERR("Take FRAM CB semaphore returned unexpected error (%d)", rc);
		break;
	}

	if (rc != 0) {
		return rc;
	}

	/* The sem is given regardless of outcome; the transfer's real result is
	 * delivered in the callback. Check it, else a failed write (e.g. the
	 * driver rejecting an oversized transfer) would masquerade as a data
	 * mismatch on the read-back below.
	 */
	if (fram_cb_data.i2c_cb_result != 0) {
		LOG_ERR("FRAM async write callback error (%d)", fram_cb_data.i2c_cb_result);
		return fram_cb_data.i2c_cb_result;
	}

	i2c_tx_buf[0] = (uint8_t)(fram_offset >> 8);
	i2c_tx_buf[1] = (uint8_t)(fram_offset >> 0);

	app_i2c_cb_prep(&fram_cb_data);

	rc = i2c_write_read_cb_dt(dts, msgs, 2U, (const void *)i2c_tx_buf, 2U, (void *)i2c_rx_buf,
				  nbytes, app_i2c_cb_func, (void *)&fram_cb_data);
	if (rc != 0) {
		LOG_ERR("App I2C wr-rd-cb-dt: write offset, read data error (%d)", rc);
		return rc;
	}

	rc = k_sem_take(&fram_cb_data.i2c_cb_sem, K_MSEC(2000));
	switch (rc) {
	case 0:
		break; /* success */
	case -EAGAIN:
		LOG_ERR("Read-back: Take FRAM CB semaphore returned -EAGAIN which is timeout");
		rc = -ETIMEDOUT;
		break;
	case -EBUSY:
		LOG_ERR("Read-back: Take FRAM CB semaphore returned -EBUSY");
		break;
	default:
		LOG_ERR("Read-back: Take FRAM CB semaphore returned unexpected error (%d)", rc);
		break;
	}

	if (rc != 0) {
		return rc;
	}

	if (fram_cb_data.i2c_cb_result != 0) {
		LOG_ERR("FRAM async read-back callback error (%d)", fram_cb_data.i2c_cb_result);
		return fram_cb_data.i2c_cb_result;
	}

	rc = memcmp(&i2c_tx_buf[2U], i2c_rx_buf, nbytes);
	if (rc != 0) {
		LOG_ERR("FAIL: data read back does not match");
	}

	return rc;
}

/* Use combine FRAM accesses into a single message array with I2C_MSG_STOP
 * delineating the complete messages.
 * Msg 0 : Write FRAM two byte offset plus data STOP. Use i2c_tx_buf
 * Msg 1 : Write FRAM two byte offset. Use local buffer
 * Msg 2:  Read RAM data STOP. Use i2c_rx_buf
 */
static int app_i2c_cb_test_fram_mm(const struct i2c_dt_spec *dts, uint16_t fram_offset,
				   uint32_t nbytes)
{
	uint8_t buf[4] = {0};
	int rc = 0;

	if ((dts == NULL) || ((nbytes + 2U) > I2C_RX_BUF_SIZE)) {
		LOG_ERR("App I2C FRAM async test bad i2c_dt_spec or nbytes");
		return -EINVAL;
	}

	LOG_INF("Test I2C multiple STOPs in message sequence");

	memset(i2c_tx_buf, 0x55, sizeof(i2c_tx_buf));
	memset(i2c_rx_buf, 0xAA, sizeof(i2c_rx_buf));

	buf[0] = (uint8_t)(fram_offset >> 8);
	buf[1] = (uint8_t)(fram_offset >> 0);
	i2c_tx_buf[0] = buf[0];
	i2c_tx_buf[1] = buf[1];

	sys_rand_get(&i2c_tx_buf[2], nbytes);

	msgs[0].buf = i2c_tx_buf;
	msgs[0].len = nbytes + 2U;
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
	msgs[1].buf = buf;
	msgs[1].len = 2U;
	msgs[1].flags = I2C_MSG_WRITE;
	msgs[2].buf = i2c_rx_buf;
	msgs[2].len = nbytes;
	msgs[2].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	app_i2c_cb_prep(&fram_cb_data);

	rc = i2c_transfer_cb(dts->bus, msgs, 3U, dts->addr, app_i2c_cb_func, (void *)&fram_cb_data);
	if (rc != 0) {
		LOG_ERR("App I2C xfr cb 3 messages with multiple STOPs (%d)", rc);
		return rc;
	}

	rc = k_sem_take(&fram_cb_data.i2c_cb_sem, K_MSEC(2000));
	switch (rc) {
	case 0:
		break;
	case -EAGAIN:
		LOG_ERR("Take FRAM CB semaphore returned -EAGAIN which is timeout");
		rc = -ETIMEDOUT;
		break;
	case -EBUSY:
		LOG_ERR("Take FRAM CB semaphore returned -EBUSY");
		break;
	default:
		LOG_ERR("Take FRAM CB semaphore returned unexpected error (%d)", rc);
		break;
	}

	if (rc != 0) {
		return rc;
	}

	if (fram_cb_data.i2c_cb_result != 0) {
		LOG_ERR("FRAM async multi-STOP callback error (%d)", fram_cb_data.i2c_cb_result);
		return fram_cb_data.i2c_cb_result;
	}

	rc = memcmp(&i2c_tx_buf[2U], i2c_rx_buf, nbytes);
	if (rc != 0) {
		LOG_ERR("FAIL: data read back does not match");
	}

	return rc;
}

static int test_read_async(const struct i2c_dt_spec *dts, uint32_t nread, struct app_i2c_cb_s *pcbs,
			   i2c_callback_t cb)
{
	int rc = 0;

	if ((dts == NULL) || (pcbs == NULL) || (cb == NULL) || (nread > I2C_RX_BUF_SIZE)) {
		LOG_ERR("Test async read bad parameter(s)");
		return -EINVAL;
	}

	memset(i2c_rx_buf, 0xAA, sizeof(i2c_rx_buf));

	msgs[0].buf = i2c_rx_buf;
	msgs[0].len = nread;
	msgs[0].flags = I2C_MSG_READ | I2C_MSG_STOP;

	app_i2c_cb_prep(pcbs);

	rc = i2c_transfer_cb_dt(dts, msgs, 1U, cb, (void *)pcbs);
	if (rc != 0) {
		LOG_ERR("I2C xfr cb error (%d)", rc);
		return rc;
	}

	rc = k_sem_take(&pcbs->i2c_cb_sem, K_MSEC(5000));
	if (rc == 0) {
		LOG_INF("I2C cb xfr start success");
	} else if (rc == -EAGAIN) {
		LOG_ERR("I2C cb xfr timeout (-EAGAIN)");
		rc = -ETIMEDOUT;
	} else if (rc == -EBUSY) {
		LOG_ERR("I2C cb xfr -EBUSY");
	} else {
		LOG_ERR("I2C cb xfr unexpected error (%d)", rc);
	}

	LOG_HEXDUMP_INF(i2c_rx_buf, nread, "I2C cb read data");

	return rc;
}

#endif
