/*
 * Copyright (c) 2022 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT kineis_kim1

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(kineis_kim1, CONFIG_SATELLITE_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <ctype.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdlib.h>
#include "modem_context.h"
#include "modem_cmd_handler.h"
#include "modem_iface_uart.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/satellite.h>

#include <zephyr/drivers/satellite/kim1.h>

#define PINCONFIG(name_, pin_, config_, irq_config_)                                       \
{                                                                                          \
	.dev_name = name_, .pin = pin_, .config = config_, .irq_config = irq_config_           \
}

#define STATE_FREE      0
#define STATE_BUSY      1
#define STATE_CLEANUP   2


struct kim1_send_work_data {
	uint8_t buf[KIM1_MAX_TX_MESSAGE_SIZE];
	uint32_t buf_length;
	uint8_t number_of_send;
	uint8_t send_counter;
	k_timeout_t time_between_send;
	satellite_api_send_result_cb result_cb;
};

/* driver data */
struct kim1_data {

	/* KIM1 modem related */
	char id[KIM1_ID_MAX_LENGTH];
	char fw_version[KIM1_FW_MAX_LENGTH];
	char serial_number[KIM1_SN_MAX_LENGTH];
	uint16_t tx_power;
	uint32_t tx_freq;

	/* GPIO related */
	const struct device *gpio_port_dev[NUM_PINS];
	struct gpio_callback kim_pgood_cb;
	struct gpio_callback kim_tx_status_cb;
	uint32_t pgood_state;
	uint32_t tx_status_state;

	/* protection from parallel use */
	atomic_t modem_usage;

	/* modem context */
	struct modem_context mctx;

	/* modem interface */
	struct modem_iface_uart_data iface_data;
	uint8_t iface_rb_buf[MDM_RING_BUF_SIZE];

	/* modem cmds */
	struct modem_cmd_handler_data cmd_handler_data;
	uint8_t cmd_match_buf[MDM_RECV_BUF_SIZE];

	/* work */
	struct k_work_q workq;
	struct k_work_delayable send_pool_work_wq;

	/* store pool send data */
	struct kim1_send_work_data pool_send_data;

	/* semaphores */
	struct k_sem sem_tx_done;
	struct k_sem sem_response;
	struct k_sem sem_ready;
};

struct kim_pinconfig {
	char *dev_name;
	gpio_pin_t pin;
	gpio_flags_t config;
	gpio_flags_t irq_config;
};

static const struct kim_pinconfig pinconfig[] = {

#if DT_INST_NODE_HAS_PROP(0, power_gpios)
	PINCONFIG(DT_INST_GPIO_LABEL(0, power_gpios),
		  DT_INST_GPIO_PIN(0, power_gpios),
		  DT_INST_GPIO_FLAGS(0, power_gpios) | GPIO_OUTPUT_INACTIVE,
		  0),
#endif

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	PINCONFIG(DT_INST_GPIO_LABEL(0, reset_gpios),
		  DT_INST_GPIO_PIN(0, reset_gpios),
		  DT_INST_GPIO_FLAGS(0, reset_gpios) | GPIO_OUTPUT_INACTIVE,
		  0),
#endif

#if DT_INST_NODE_HAS_PROP(0, pgood_gpios)
	PINCONFIG(DT_INST_GPIO_LABEL(0, pgood_gpios),
		  DT_INST_GPIO_PIN(0, pgood_gpios),
		  DT_INST_GPIO_FLAGS(0, pgood_gpios) | GPIO_INPUT,
		  GPIO_INT_EDGE_BOTH),
#endif

#if DT_INST_NODE_HAS_PROP(0, tx_status_gpios)
	PINCONFIG(DT_INST_GPIO_LABEL(0, tx_status_gpios),
		  DT_INST_GPIO_PIN(0, tx_status_gpios),
		  DT_INST_GPIO_FLAGS(0, tx_status_gpios) | GPIO_INPUT,
		  GPIO_INT_EDGE_BOTH),
#endif

};

NET_BUF_POOL_DEFINE(mdm_recv_pool, MDM_RECV_MAX_BUF, MDM_RECV_BUF_SIZE,
		    0, NULL);

/* RX thread structures */
K_KERNEL_STACK_DEFINE(kim1_rx_stack,
		      CONFIG_SATELLITE_KINEIS_KIM1_RX_STACK_SIZE);
struct k_thread kim1_rx_thread;

/* RX thread work queue */
K_KERNEL_STACK_DEFINE(kim1_workq_stack,
		      CONFIG_SATELLITE_KINEIS_KIM1_WORKQ_STACK_SIZE);

struct kim1_data m_drv_data;

#if DT_INST_NODE_HAS_PROP(0, pgood_gpios)
void km1_pgood_callback_isr(const struct device *port, struct gpio_callback *cb,
			    uint32_t pins)
{
	m_drv_data.pgood_state = (uint32_t)gpio_pin_get(
						m_drv_data.gpio_port_dev[KIM_PGOOD],
						pinconfig[KIM_PGOOD].pin);
	if (m_drv_data.pgood_state) {
		k_sem_give(&m_drv_data.sem_ready);
	}
	LOG_DBG("KIM_PGOOD:%d", m_drv_data.pgood_state);
}
#endif

#if DT_INST_NODE_HAS_PROP(0, tx_status_gpios)
void km1_tx_status_callback_isr(const struct device *port, struct gpio_callback *cb,
			    uint32_t pins)
{
	m_drv_data.tx_status_state = (uint32_t)gpio_pin_get(
						m_drv_data.gpio_port_dev[KIM_TX_STATUS],
						pinconfig[KIM_TX_STATUS].pin);
	LOG_DBG("KIM_TX_STATUS:%d", m_drv_data.tx_status_state);
}
#endif

#if DT_INST_NODE_HAS_PROP(0, power_gpios)
static void kim_set_power(bool assert)
{
	if (assert) {
		LOG_DBG("KIM_POWER -> true");
		gpio_pin_set(m_drv_data.gpio_port_dev[KIM_POWER],
			     pinconfig[KIM_POWER].pin, true);
	} else {
		LOG_DBG("KIM_POWER -> false");
		gpio_pin_set(m_drv_data.gpio_port_dev[KIM_POWER],
			     pinconfig[KIM_POWER].pin, false);
	}
}
#endif

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
static void kim_set_reset(bool assert)
{
	if (assert) {
		LOG_DBG("KIM_RESET -> true");
		gpio_pin_set(m_drv_data.gpio_port_dev[KIM_RESET],
			     pinconfig[KIM_RESET].pin, true);
	} else {
		LOG_DBG("KIM_RESET -> false");
		gpio_pin_set(m_drv_data.gpio_port_dev[KIM_RESET],
			     pinconfig[KIM_RESET].pin, false);
	}
}
#endif

static int kim1_cmd_send(struct kim1_data *p_data,
			       const struct modem_cmd *handlers,
			       size_t handlers_len, const char *buf,
			       k_timeout_t timeout)
{
	return modem_cmd_send(&p_data->mctx.iface, &p_data->mctx.cmd_handler,
			      handlers, handlers_len, buf, &p_data->sem_response,
			      timeout);
}

/**
 * @brief Attempt to acquire the modem for operations
 *
 * @param data kim1 data struct
 *
 * @retval true if modem was acquired
 * @retval false otherwise
 */
static inline bool modem_acquire(struct kim1_data *data)
{
	return atomic_cas(&data->modem_usage, STATE_FREE, STATE_BUSY);
}

/**
 * @brief Safely release the modem from any context
 *
 * This function can be called from any context and guarantees that the
 * release operations will only be run once.
 *
 * @param data ckim1 data struct
 *
 * @retval true if modem was released by this function
 * @retval false otherwise
 */
static bool modem_release(struct kim1_data *data)
{
	/* Increment atomic so both acquire and release will fail */
	if (!atomic_cas(&data->modem_usage, STATE_BUSY, STATE_CLEANUP)) {
		return false;
	}
	/* Completely release modem */
	atomic_clear(&data->modem_usage);
	return true;
}

/*
 * Modem Response Command Handlers
 */

/* Handler: OK */
MODEM_CMD_DEFINE(on_cmd_ok)
{
	struct kim1_data *dev = CONTAINER_OF(data, struct kim1_data,
					    cmd_handler_data);

	modem_cmd_handler_set_error(data, 0);

	k_sem_give(&dev->sem_response);
	k_sem_give(&dev->sem_ready);

	return 0;
}

/* Handler: ERROR */
MODEM_CMD_DEFINE(on_cmd_error)
{
	struct kim1_data *dev = CONTAINER_OF(data, struct kim1_data,
					    cmd_handler_data);

	modem_cmd_handler_set_error(data, -EIO);

	int error = 0;
	int error_parameter = 0;
	char *strToken = NULL;
	uint8_t i;

	/* Manage <error_id>,<parameter_index> */
	if (strchr(argv[0], ',')) {
		strToken = strtok(argv[0], ",");
		for (i = 0; strToken != NULL; strToken = strtok(NULL, ","), i++) {

			if (i == KIM1_ERROR_ID) {
				error = strtol(strToken, NULL, 10);
			} else if (i == KIM1_ERROR_PARAMETER_INDEX) {
				error_parameter = strtol(strToken, NULL, 10);
			} else {
				LOG_ERR("Error shall contains max. 2 parameters");
				break;
			}
		}
	/* Manage <error_id> */
	} else {
		error = strtol(argv[0], NULL, 10);
	}

	if (error < KIM1_ERROR_1 && error >= KIM1_ERROR_MAX) {
		LOG_ERR("Unknown error: %d", error);
	} else {
		LOG_ERR("Error received: %d | %s", error, kim_error_description[error]);
		LOG_DBG("Error parameter: %d", error_parameter);
	}

	k_sem_give(&dev->sem_response);

	return 0;
}

/* Handler: +ID=<ID_number>" */
MODEM_CMD_DEFINE(on_cmd_id_number)
{
	int ret = 0;
	struct kim1_data *dev = CONTAINER_OF(data, struct kim1_data,
					    cmd_handler_data);
	uint8_t id_length;

	if (argv[0]) {
		id_length = strlen(argv[0]);
		if (id_length > KIM1_ID_MAX_LENGTH) {
			id_length = KIM1_ID_MAX_LENGTH;
		}
		strncpy(dev->id, argv[0], id_length);
		LOG_DBG("KIM1 ID: %.*s", id_length, dev->id);
	} else {
		LOG_ERR("Error while reading ID");
		ret = -EFAULT;
	}

	k_sem_give(&dev->sem_response);

	return ret;
}

/* Handler: AT+FW=<fw_version>" */
MODEM_CMD_DEFINE(on_cmd_fw_version)
{
	int ret = 0;
	struct kim1_data *dev = CONTAINER_OF(data, struct kim1_data,
					    cmd_handler_data);
	uint8_t fw_length;

	if (argv[0]) {
		fw_length = strlen(argv[0]);
		if (fw_length > KIM1_FW_MAX_LENGTH) {
			fw_length = KIM1_FW_MAX_LENGTH;
		}
		strncpy(dev->fw_version, argv[0], fw_length);
		LOG_DBG("KIM1 FW: %.*s", fw_length, dev->fw_version);
	} else {
		LOG_ERR("Error while reading FW");
		ret = -EFAULT;
	}

	k_sem_give(&dev->sem_response);

	return ret;
}

/* Handler: AT+SN=<sn>" */
MODEM_CMD_DEFINE(on_cmd_serial_number)
{
	int ret = 0;

	struct kim1_data *dev = CONTAINER_OF(data, struct kim1_data,
					    cmd_handler_data);

	uint8_t sn_length;

	if (argv[0]) {
		sn_length = strlen(argv[0]);
		if (sn_length > KIM1_SN_MAX_LENGTH) {
			sn_length = KIM1_SN_MAX_LENGTH;
		}
		strncpy(dev->serial_number, argv[0], sn_length);
		LOG_DBG("KIM1 SN: %.*s", sn_length, dev->serial_number);
	} else {
		LOG_ERR("Error while reading SN");
		ret = -EFAULT;
	}

	k_sem_give(&dev->sem_response);

	return ret;
}

/* Handler: AT+PWR=<power>" */
MODEM_CMD_DEFINE(on_cmd_get_tx_power)
{
	int ret = 0;

	struct kim1_data *dev = CONTAINER_OF(data, struct kim1_data,
					    cmd_handler_data);

	if (argv[0]) {
		dev->tx_power = (uint16_t) strtol(argv[0], NULL, 10);
		LOG_DBG("KIM1 TX PWR: %"PRIu16"", dev->tx_power);
	} else {
		LOG_ERR("Error while reading TX PWR");
		ret = -EFAULT;
	}

	k_sem_give(&dev->sem_response);

	return ret;
}

/* Handler: AT+ATXFREQ=<frequency in hertz>" */
MODEM_CMD_DEFINE(on_cmd_get_tx_frequency)
{
	int ret = 0;

	struct kim1_data *dev = CONTAINER_OF(data, struct kim1_data,
					    cmd_handler_data);

	if (argv[0]) {
		dev->tx_freq = (uint32_t) strtoll(argv[0], NULL, 10);
		LOG_DBG("KIM1 TX FREQ: %"PRIu32"", dev->tx_freq);
	} else {
		LOG_ERR("Error while reading TX FREQ");
		ret = -EFAULT;
	}

	k_sem_give(&dev->sem_response);

	return ret;
}

static const struct modem_cmd response_cmds[] = {
	MODEM_CMD("+OK", on_cmd_ok, 0U, ""),
	MODEM_CMD("+ERROR=", on_cmd_error, 1U, ""),
};

/*
 * Handler: +TX=<Transmission>,<Data>
 */
MODEM_CMD_DEFINE(on_cmd_tx)
{
	struct kim1_data *dev = CONTAINER_OF(data, struct kim1_data,
					    cmd_handler_data);

	k_sem_give(&dev->sem_tx_done);
	return 0;
}

static const struct modem_cmd unsol_cmds[] = {
	MODEM_CMD("+OK", on_cmd_ok, 0U, ""),
	MODEM_CMD("+ERROR=", on_cmd_error, 1U, ""),
	MODEM_CMD("+TX=", on_cmd_tx, 0U, ""),
};

static int km1_set_tx_power(const struct device *dev, enum kim1_tx_power_enum tx_power)
{
	int ret;
	char cmd_con[sizeof("AT+"_PWR"=XXXX")];

	if (tx_power >= KIM1_TX_PWR_MAX) {
		LOG_ERR("Maximum transmit power is 1000mW");
		return -EINVAL;
	}

	snprintk(cmd_con, sizeof(cmd_con), "AT+"_PWR"=%"PRIu16"", kim1_tx_power_value[tx_power]);

	ret = kim1_cmd_send(&m_drv_data, NULL, 0U, cmd_con, KIM1_CMD_TIMEOUT);

	return ret;
}

static int km1_send_message(uint8_t *send_buf, uint32_t buf_len)
{
	int ret;
	char cmd_con[sizeof("AT+TX=") + KIM1_MAX_TX_MESSAGE_SIZE_HEXA + 1];
	char hex_buf[KIM1_MAX_TX_MESSAGE_SIZE_HEXA + 1];
	size_t hex_buf_length;

	if (!send_buf) {
		LOG_ERR("Send buffer is null");
		return -EFAULT;
	}

	if (buf_len > KIM1_MAX_TX_MESSAGE_SIZE) {
		LOG_ERR("Cannot send more than %d bytes", KIM1_MAX_TX_MESSAGE_SIZE);
		return -E2BIG;
	} else if (buf_len == 0) {
		LOG_ERR("Send buffer length is 0");
		return -EFAULT;
	}

	memset(hex_buf, 0, sizeof(hex_buf));
	memset(cmd_con, 0, sizeof(cmd_con));

	/* Convert data from binary into hexa */
	hex_buf_length = bin2hex(send_buf, buf_len, hex_buf, sizeof(hex_buf));

	if (hex_buf_length > 0) {
		snprintk(cmd_con, sizeof(cmd_con), "AT+TX=%.*s", hex_buf_length, hex_buf);
	} else {
		LOG_ERR("An error occurred while converting bin2hex buf");
		return -EIO;
	}

	LOG_DBG("AT+TX cmd= %s", cmd_con);

	ret = kim1_cmd_send(&m_drv_data, NULL, 0, cmd_con, KIM1_CMD_TIMEOUT);

	return ret;
}

static int send_message_sync(const struct device *dev,
							 uint8_t *send_buf, uint32_t buf_len)
{
	int ret = 0;

	/* Ensure available */
	if (!modem_acquire(&m_drv_data)) {
		return -EBUSY;
	}

	if (!send_buf) {
		LOG_ERR("Null data pointer");
		return -EFAULT;
	}

	k_sem_reset(&m_drv_data.sem_tx_done);

	ret = km1_send_message(send_buf, buf_len);
	if (ret != 0) {
		LOG_ERR("Error while sending data");
		goto error;
	}

	ret = k_sem_take(&m_drv_data.sem_tx_done, KIM1_TX_TIMEOUT);

error:
	modem_release(&m_drv_data);
	return ret;
}

static int send_message_pool_async(const struct device *dev,
						   uint8_t *send_buf, uint32_t buf_len,
						   uint8_t number_of_send,
						   k_timeout_t time_between_send,
						   satellite_api_send_result_cb *result_cb)
{
	int ret;

	/* Ensure available, decremented after configuration */
	if (!modem_acquire(&m_drv_data)) {
		return -EBUSY;
	}

	if (!send_buf) {
		LOG_ERR("Null data pointer");
		return -EFAULT;
	}

	/* Fill in send_pool_data structure */
	memcpy(&m_drv_data.pool_send_data.buf, send_buf, buf_len);
	m_drv_data.pool_send_data.buf_length = buf_len;
	m_drv_data.pool_send_data.number_of_send = number_of_send;
	m_drv_data.pool_send_data.send_counter = 0;
	m_drv_data.pool_send_data.time_between_send = time_between_send;
	m_drv_data.pool_send_data.result_cb = (satellite_api_send_result_cb) result_cb;

	/* Launch first send */
	ret = k_work_schedule_for_queue(&m_drv_data.workq, &m_drv_data.send_pool_work_wq,
									K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to initialize work");
		modem_release(&m_drv_data);
	}

	return ret;
}

static void kim1_send_pool_work(struct k_work *work)
{
	int ret;
	bool job_end;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct kim1_data *dev = CONTAINER_OF(dwork, struct kim1_data,
										send_pool_work_wq);

	k_sem_reset(&dev->sem_tx_done);

	/* Send message over KIM1 modem */
	ret = km1_send_message(dev->pool_send_data.buf, dev->pool_send_data.buf_length);
	if (ret != 0) {
		LOG_ERR("Error while sending data");
		goto error;
	}

	dev->pool_send_data.send_counter++;
	job_end = dev->pool_send_data.send_counter >= dev->pool_send_data.number_of_send;

	/* Reschedule works if remaining send job */
	if (!job_end) {
		if (k_work_schedule_for_queue(&dev->workq, &dev->send_pool_work_wq,
						dev->pool_send_data.time_between_send) < 0) {
			LOG_ERR("Failed to schedule next send");
			goto error;
		}
	}

	/* Wait for TX confirmation */
	if (k_sem_take(&dev->sem_tx_done, KIM1_TX_TIMEOUT) != 0) {
		LOG_ERR("TX timeout expires");
		goto error;
	}

	/* Execute callback when job is done */
	if (job_end) {
		if (dev->pool_send_data.result_cb) {
			dev->pool_send_data.result_cb(true);
		}
		modem_release(dev);
	}

	LOG_DBG("Send done");
	return;

error:
	if (dev->pool_send_data.result_cb) {
		dev->pool_send_data.result_cb(false);
	}
	modem_release(dev);
}

static enum kim1_tx_power_enum get_closest_tx_power(const uint16_t tx_power)
{
	uint16_t min = UINT16_MAX;
	enum kim1_tx_power_enum result = KIM1_TX_PWR_MAX;
	uint16_t item_min;

	if (tx_power > kim1_tx_power_value[KIM1_TX_PWR_MAX]) {
		LOG_ERR("Maximum tx power is 1000mW");
		return KIM1_TX_PWR_MAX;
	}

	/* Looking for closest enum value according to integer tx power in mW */
	for (enum kim1_tx_power_enum idx = 0;  idx < KIM1_TX_PWR_MAX; idx++) {

		item_min = abs(tx_power - kim1_tx_power_value[idx]);
		if (item_min < min) {
			min = item_min;
			result = idx;
		}
	}
	return result;
}

static int set_satellite_config(const struct device *dev,
			       struct satellite_modem_config *config)
{
	int ret;

	/* Ensure available, decremented after configuration */
	if (!modem_acquire(&m_drv_data)) {
		return -EBUSY;
	}

	ret = km1_set_tx_power(dev, get_closest_tx_power(config->tx_power));

	modem_release(&m_drv_data);

	return ret;
}

/* RX thread */
static void kim1_rx(struct kim1_data *drv_data)
{
	int ret;

	while (true) {
		/* wait for incoming data */
		ret = k_sem_take(&drv_data->iface_data.rx_sem, K_FOREVER);

		drv_data->mctx.cmd_handler.process(&drv_data->mctx.cmd_handler,
							&drv_data->mctx.iface);

		/* give up time in case of long stream (as coop thread) */
		k_yield();
	}
}

#if DT_INST_NODE_HAS_PROP(0, pgood_gpios)
static void kineis_check_hw_wakeup_state_init(void)
{
	k_sem_reset(&m_drv_data.sem_ready);
}

static int kineis_check_hw_wakeup_state(void)
{
	int ret;

	if (dev->pgood_state == state_to_check) {
		ret = k_sem_take(&m_drv_data.sem_ready,
				K_MSEC(CONFIG_SATELLITE_KINEIS_KIM1_RESET_TIMEOUT));
		if (ret == 0 || ret != -ETIMEDOUT) {
			break;
		}
	}

	if (ret < 0) {
		LOG_ERR("Failed to reset device: %d", ret);
		return -EAGAIN;
	}

	return ret;
}
#endif

static int kineis_check_sw_wakeup_state(void)
{
	int ret;
	uint8_t retries = 3U;

	k_sem_reset(&m_drv_data.sem_ready);

	while (retries--) {
		ret = modem_cmd_send(&m_drv_data.mctx.iface, &m_drv_data.mctx.cmd_handler,
				     NULL, 0, "AT+PING=?", &m_drv_data.sem_ready,
				     K_MSEC(CONFIG_SATELLITE_KINEIS_KIM1_RESET_TIMEOUT));
		if (ret == 0 || ret != -ETIMEDOUT) {
			break;
		}
	}
	return ret;
}

int kineis_switch_on(void)
{
	int ret = 0;
#if DT_INST_NODE_HAS_PROP(0, pgood_gpios)
	kineis_check_hw_wakeup_state_init();
#endif
#if DT_INST_NODE_HAS_PROP(0, power_gpios)
	kim_set_power(true);
#endif

#if DT_INST_NODE_HAS_PROP(0, pgood_gpios)
	ret = kineis_check_hw_wakeup_state();
	if (ret < 0) {
		LOG_ERR("Failed to reset device: %d", ret);
		return ret;
	}
#endif
#if DT_INST_NODE_HAS_PROP(0, power_gpios)
	ret = kineis_check_sw_wakeup_state();
#else
	LOG_ERR("Please consider connnecting power GPIO");
#endif

	return ret;
}

/** public functions */

int kineis_switch_off(void)
{
#if DT_INST_NODE_HAS_PROP(0, power_gpios)
	kim_set_power(false);
#else
	LOG_ERR("Please consider connnecting power GPIO");
#endif
	return 0;
}

char *kineis_get_modem_id(void)
{
	return m_drv_data.id;
}

char *kineis_get_modem_fw_version(void)
{
	return m_drv_data.fw_version;
}

char *kineis_get_modem_serial_number(void)
{
	return m_drv_data.serial_number;
}

/** end of public functions */

static int kineis_init(void)
{
	int ret;
	static const struct setup_cmd setup_cmds[] = {
		SETUP_CMD("AT+"_ID"=?", "+"_ID"=",
			  on_cmd_id_number, 1U, ""),
		SETUP_CMD("AT+"_FW"=?", "+"_FW"=",
			  on_cmd_fw_version, 1U, ""),
		SETUP_CMD("AT+"_SN"=?", "+"_SN"=",
			  on_cmd_serial_number, 1U, ""),
		SETUP_CMD("AT+"_PWR"=?", "+"_PWR"=",
			  on_cmd_get_tx_power, 1U, ""),
		SETUP_CMD("AT+"_FREQ"=?", "+"_FREQ"=",
			  on_cmd_get_tx_frequency, 1U, ""),
	};

	ret = modem_cmd_handler_setup_cmds(&m_drv_data.mctx.iface,
					   &m_drv_data.mctx.cmd_handler, setup_cmds,
					   ARRAY_SIZE(setup_cmds),
					   &m_drv_data.sem_response,
					   KIM1_INIT_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Init failed %d", ret);
		return -ENODEV;
	}

	LOG_DBG("Kinéis KIM1 ready");
	return 0;
}

static int kim1_reset(void)
{
	int ret = 0;
#if DT_INST_NODE_HAS_PROP(0, pgood_gpios)
	kineis_check_hw_wakeup_state_init();
#endif

#if DT_INST_NODE_HAS_PROP(0, power_gpios)
	kim_set_power(false);
	k_sleep(K_MSEC(100));
	kim_set_power(true);
#endif
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	kim_set_reset(true);
	k_sleep(K_MSEC(100));
	kim_set_reset(false);
#endif

#if DT_INST_NODE_HAS_PROP(0, pgood_gpios)
	ret = kineis_check_hw_wakeup_state();

	if (ret < 0) {
		LOG_ERR("Failed to reset device: %d", ret);
		return -EAGAIN;
	}
#endif

	ret = kineis_check_sw_wakeup_state();

	return ret;
}

static int kim1_init(const struct device *dev)
{
	int ret = 0;

	ARG_UNUSED(dev);

	/* check for valid pinconfig */
	__ASSERT(ARRAY_SIZE(pinconfig) == NUM_PINS,
		 "Incorrect Kineis KIM1 pinconfig!");

	k_sem_init(&m_drv_data.sem_tx_done, 0, 1);
	k_sem_init(&m_drv_data.sem_response, 0, 1);
	k_sem_init(&m_drv_data.sem_ready, 0, 1);

	k_work_init_delayable(&m_drv_data.send_pool_work_wq, kim1_send_pool_work);

	/* initialize the work queue */
	k_work_queue_start(&m_drv_data.workq, kim1_workq_stack,
			   K_KERNEL_STACK_SIZEOF(kim1_workq_stack),
			   K_PRIO_COOP(CONFIG_SATELLITE_KINEIS_KIM1_WORKQ_THREAD_PRIORITY),
			   NULL);
	k_thread_name_set(&m_drv_data.workq.thread, "kim1_workq");

	/* cmd handler */
	m_drv_data.cmd_handler_data.cmds[CMD_RESP] = response_cmds;
	m_drv_data.cmd_handler_data.cmds_len[CMD_RESP] = ARRAY_SIZE(response_cmds);
	m_drv_data.cmd_handler_data.cmds[CMD_UNSOL] = unsol_cmds;
	m_drv_data.cmd_handler_data.cmds_len[CMD_UNSOL] = ARRAY_SIZE(unsol_cmds);
	m_drv_data.cmd_handler_data.match_buf = &m_drv_data.cmd_match_buf[0];
	m_drv_data.cmd_handler_data.match_buf_len = sizeof(m_drv_data.cmd_match_buf);
	m_drv_data.cmd_handler_data.buf_pool = &mdm_recv_pool;
	m_drv_data.cmd_handler_data.alloc_timeout = K_NO_WAIT;
	m_drv_data.cmd_handler_data.eol = "\r\n";
	ret = modem_cmd_handler_init(&m_drv_data.mctx.cmd_handler,
				       &m_drv_data.cmd_handler_data);
	if (ret < 0) {
		LOG_ERR("Modem command handler failed");
		goto error;
	}

	/* setup port devices and pin directions */
	for (uint8_t i = 0; i < NUM_PINS; i++) {
		m_drv_data.gpio_port_dev[i] =
			device_get_binding(pinconfig[i].dev_name);
		if (!m_drv_data.gpio_port_dev[i]) {
			LOG_ERR("gpio port (%s) not found!",
				pinconfig[i].dev_name);
			return -ENODEV;
		}

		ret = gpio_pin_configure(m_drv_data.gpio_port_dev[i], pinconfig[i].pin,
					 pinconfig[i].config);
		if (ret) {
			LOG_ERR("Error configuring IO %s %d err: %d!", pinconfig[i].dev_name,
				pinconfig[i].pin, ret);
			return ret;
		}
	}

#if DT_INST_NODE_HAS_PROP(0, power_gpios)
	kim_set_power(false);
#endif
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	kim_set_reset(false);
#endif

	/* setup input pin callbacks */
#if DT_INST_NODE_HAS_PROP(0, pgood_gpios)
	/* PGOOD pin */
	gpio_init_callback(&m_drv_data.kim_pgood_cb, km1_pgood_callback_isr,
			   BIT(pinconfig[KIM_PGOOD].pin));

	ret = gpio_add_callback(m_drv_data.gpio_port_dev[KIM_PGOOD],
				&m_drv_data.kim_pgood_cb);
	if (ret) {
		LOG_ERR("Cannot setup PGOOD callback! (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure(m_drv_data.gpio_port_dev[KIM_PGOOD],
					   pinconfig[KIM_PGOOD].pin,
					   pinconfig[KIM_PGOOD].irq_config);
	if (ret) {
		LOG_ERR("Error config PGOOD interrupt! (%d)", ret);
		return ret;
	}
#endif

#if DT_INST_NODE_HAS_PROP(0, tx_status_gpios)
	/* PGOOD pin */
	gpio_init_callback(&m_drv_data.kim_pgood_cb, km1_tx_status_callback_isr,
			   BIT(pinconfig[KIM_TX_STATUS].pin));

	ret = gpio_add_callback(m_drv_data.gpio_port_dev[KIM_TX_STATUS],
				&m_drv_data.kim_tx_status_cb);
	if (ret) {
		LOG_ERR("Cannot setup TX_STATUS callback! (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure(m_drv_data.gpio_port_dev[KIM_TX_STATUS],
					   pinconfig[KIM_TX_STATUS].pin,
					   pinconfig[KIM_TX_STATUS].irq_config);
	if (ret) {
		LOG_ERR("Error config TX_STATUS interrupt! (%d)", ret);
		return ret;
	}
#endif

	/* modem interface */
	m_drv_data.iface_data.rx_rb_buf = &m_drv_data.iface_rb_buf[0];
	m_drv_data.iface_data.rx_rb_buf_len = sizeof(m_drv_data.iface_rb_buf);
	ret = modem_iface_uart_init(&m_drv_data.mctx.iface, &m_drv_data.iface_data,
				    DEVICE_DT_GET(DT_INST_BUS(0)));
	if (ret < 0) {
		LOG_ERR("Failed to setup UART");
		goto error;
	}

	m_drv_data.mctx.driver_data = &m_drv_data;

	ret = modem_context_register(&m_drv_data.mctx);
	if (ret < 0) {
		LOG_ERR("Error registering modem context: %d", ret);
		goto error;
	}

	/* start RX thread */
	k_thread_create(&kim1_rx_thread, kim1_rx_stack,
			K_KERNEL_STACK_SIZEOF(kim1_rx_stack),
			(k_thread_entry_t)kim1_rx,
			&m_drv_data, NULL, NULL,
			K_PRIO_COOP(CONFIG_SATELLITE_KINEIS_KIM1_RX_THREAD_PRIORITY), 0,
			K_NO_WAIT);
	k_thread_name_set(&kim1_rx_thread, "kim1_rx");

	/* reset the modem */
	ret = kim1_reset();

	/* initialize modem */
	ret = kineis_init();

#if DT_INST_NODE_HAS_PROP(0, power_gpios)
	/* switch off if power is controlled */
	kim_set_power(false);
#endif

error:
	return ret;
}

static const struct satellite_driver_api kim1_satellite_api = {
	.config = set_satellite_config,
	.send_sync = send_message_sync,
	.send_pool_async = send_message_pool_async,
};

DEVICE_DT_INST_DEFINE(0, &kim1_init, NULL, &m_drv_data,
		      NULL, POST_KERNEL, CONFIG_SATELLITE_INIT_PRIORITY,
		      &kim1_satellite_api);
