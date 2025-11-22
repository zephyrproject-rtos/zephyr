#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_hid, CONFIG_I2C_LOG_LEVEL);

/*
 * FIXME: Move these definitions to a shared hid header.
 */

#include <zephyr/usb/class/usbd_hid.h>

struct hid_device_driver_api {
	int (*enable_output)(const struct device *dev, const bool enable);
	int (*submit_report)(const struct device *dev, const uint16_t size,
			     const uint8_t *const report);
	int (*dev_register)(const struct device *dev, const uint8_t *const rdesc,
			    const uint16_t rsize, const struct hid_device_ops *const ops);
};

#pragma pack(push, 1)
/*
 * We want to be able to modify few fileds at the runtime, but most of
 * the structure should be immutable and populated at compbile time
 * from DT.
 */
struct i2c_hid_desc {
	const uint16_t wHIDDescLength;
	const uint16_t bcdVersion;
	uint16_t wReportDescLength;
	const uint16_t wReportDescRegister;
	const uint16_t wInputRegister;
	const uint16_t wMaxInputLength;
	const uint16_t wOutputRegister;
	const uint16_t wMaxOutputLength;
	const uint16_t wCommandRegister;
	const uint16_t wDataRegister;
	uint16_t wVendorID;
	uint16_t wProductID;
	uint16_t wVersionID;
	const uint32_t reserved;
};

#define I2C_HID_CMD_OPCODE GENMASK(4, 0)

struct i2c_hid_input_register {
	uint16_t length;
	uint8_t report[];
};

#pragma pack(pop)

enum i2c_hid_command_report_types {
	I2C_HID_REPORT_INPUT = 0b01,
	I2C_HID_REPORT_OUTPUT = 0b10,
	I2C_HID_REPORT_FEATURE = 0b10,
};

enum i2c_hid_opcodes {
	I2C_HID_OPCODE_RESET = 0b0001,
	I2C_HID_OPCODE_GET_REPORT = 0b0010,
	I2C_HID_OPCODE_SET_REPORT = 0b0011,
	I2C_HID_OPCODE_GET_IDLE = 0b0100,
	I2C_HID_OPCODE_SET_IDLE = 0b0101,
	I2C_HID_OPCODE_GET_PROTOCOL = 0b0110,
	I2C_HID_OPCODE_SET_PROTOCOL = 0b0111,
	I2C_HID_OPCODE_SET_POWER = 0b1000,
};

struct i2c_hid_target_config {
	struct gpio_dt_spec int_gpio;
	const struct device *bus;
	uint16_t hid_descr_addr;
};

struct i2c_hid_target_data {
	struct i2c_target_config target;
	struct i2c_hid_desc desc;
	const struct i2c_hid_target_config *const config;
	uint16_t reg;
	const struct hid_device_ops *ops;
	const uint8_t *rdesc;
	size_t rsize;
	struct k_msgq *msgq;
	struct i2c_hid_input_register *input;
};

static struct i2c_hid_target_data *to_i2c_hid_target_data(struct i2c_target_config *target)
{
	return CONTAINER_OF(target, struct i2c_hid_target_data, target);
}

static void i2c_hid_target_irq_set(struct i2c_target_config *target, int level)
{
	struct i2c_hid_target_data *data = to_i2c_hid_target_data(target);
	const int err = gpio_pin_set_dt(&data->config->int_gpio, level);
	if (err) {
		__ASSERT_NO_MSG(err == 0);
	}
}

static void i2c_hid_target_reset_input(struct i2c_target_config *target)
{
	struct i2c_hid_target_data *data = to_i2c_hid_target_data(target);
	memset(data->input, 0, data->desc.wMaxInputLength);
}

static void i2c_hid_target_reset(struct i2c_target_config *target)
{
	struct i2c_hid_target_data *data = to_i2c_hid_target_data(target);

	/*
	 * Purge any pending transfers
	 */
	k_msgq_purge(data->msgq);
	/*
	 * We default to reading Input Register
	 */
	data->reg = data->desc.wInputRegister;
	i2c_hid_target_irq_set(target, 1);
}

static int i2c_hid_target_stop(struct i2c_target_config *config)
{
	return 0;
}

static void i2c_hid_target_do_command(struct i2c_target_config *target, const uint8_t *buf,
				      uint32_t len)
{
	if (len < sizeof(uint8_t) + sizeof(uint8_t)) {
		LOG_ERR("Invalid command payload, dropping");
		return;
	}

	/*
	 * TODO: Implement report ID and type decoding once it's used
	 * somewhere
	 */
	buf++;
	len--;

	const uint8_t opcode = FIELD_GET(I2C_HID_CMD_OPCODE, *buf);
	buf++;
	len--;

	LOG_DBG("Command opcode: %x", opcode);

	switch (opcode) {
	case I2C_HID_OPCODE_RESET:
		i2c_hid_target_reset(target);
		break;
	case I2C_HID_OPCODE_SET_POWER:
		break;
	case I2C_HID_OPCODE_GET_IDLE:
	case I2C_HID_OPCODE_SET_IDLE:
	case I2C_HID_OPCODE_GET_PROTOCOL:
	case I2C_HID_OPCODE_SET_PROTOCOL:
	case I2C_HID_OPCODE_GET_REPORT:
	case I2C_HID_OPCODE_SET_REPORT:
		LOG_WRN("Opcode %x is not implemented", opcode);
	default:
		/*
		 * Per 7.2.9 RESERVERD COMMAND RANGE, we should ignore any
		 * commands from the reserved range
		 */
		break;
	}
}

static void i2c_hid_target_buf_write_received(struct i2c_target_config *target, uint8_t *buf,
					      uint32_t len)
{
	struct i2c_hid_target_data *data = to_i2c_hid_target_data(target);

	if (len < sizeof(uint16_t)) {
		LOG_ERR("Short write, at least 2 bytes are expected");
		return;
	}

	const uint16_t reg = sys_le16_to_cpu(*(uint16_t *)buf);

	buf += sizeof(reg);
	len -= sizeof(reg);

	data->reg = reg;

	if (reg == data->desc.wOutputRegister) {
	}

	if (data->reg == data->desc.wDataRegister) {
	}

	if (reg == data->desc.wCommandRegister) {
		i2c_hid_target_do_command(target, buf, len);
	}
}

static int i2c_hid_target_buf_read_requested(struct i2c_target_config *target, uint8_t **ptr,
					     uint32_t *len)
{
	struct i2c_hid_target_data *data = to_i2c_hid_target_data(target);
	int ret = 0;

	if (data->reg == data->config->hid_descr_addr) {
		*ptr = (uint8_t *)&data->desc;
		*len = sizeof(data->desc);
		goto done;
	}

	if (data->reg == data->desc.wReportDescRegister) {
		*ptr = (uint8_t *)data->rdesc;
		*len = data->rsize;

		LOG_HEXDUMP_INF(data->rdesc, data->rsize, "HID report descriptor:");
		goto done;
	}

	if (data->reg == data->desc.wDataRegister) {
		ret = -ENOTSUP;
		goto done;
	}

	/*
	 * Default reads are from the input register
	 */

	const int err = k_msgq_get(data->msgq, data->input, K_NO_WAIT);
	if (err < 0) {
		/*
		 * If there are no inpur reports to send, clear the input
		 * report register to return a no-op input data via this read
		 */
		i2c_hid_target_reset_input(target);
	}

	*ptr = (uint8_t *)data->input;
	*len = data->desc.wMaxInputLength;

	if (k_msgq_num_used_get(data->msgq) == 0) {
		i2c_hid_target_irq_set(target, 0);
	}

done:
	data->reg = data->desc.wInputRegister;
	return ret;
}

static int i2c_hid_target_dev_register(const struct device *dev, const uint8_t *const rdesc,
				       const uint16_t rsize, const struct hid_device_ops *const ops)
{
	const struct i2c_hid_target_config *config = dev->config;
	struct i2c_hid_target_data *data = dev->data;

	if (ops->get_report || ops->iface_ready || ops->set_report || ops->set_idle ||
	    ops->get_idle || ops->set_protocol || ops->input_report_done || ops->output_report) {
		LOG_ERR("FIXME: Unimplemented callbacks requested");
		return -ENOTSUP;
	}

	if (ops->sof) {
		LOG_WRN("HID over I2C doesn't have a concept of SoF. Ignoring the callback");
	}

	if (ops == NULL /* || ops->get_report == NULL */) {
		LOG_ERR("get_report callback is missing");
		return -EINVAL;
	}

	if (data->ops) {
		return -EALREADY;
	}

	data->ops = ops;
	data->rdesc = rdesc;
	data->rsize = rsize;
	data->desc.wReportDescLength = rsize;

	const int err = i2c_target_register(config->bus, &data->target);
	if (err) {
		LOG_ERR("Failed to register target\n");
		return err;
	}

	return 0;
}

static int i2c_hid_target_dev_submit_report(const struct device *dev, const uint16_t size,
					    const uint8_t *const report)
{
	struct i2c_hid_target_data *data = dev->data;
	/*
	 * wMaxInputLength includes the 2 byte header so we need to
	 * account for that.
	 */
	if (size > data->desc.wMaxInputLength - sizeof(uint16_t)) {
		return -EINVAL;
	}

	union {
		uint8_t raw[data->desc.wMaxInputLength];
		struct i2c_hid_input_register input;
	} buf;

	buf.input.length = size + sizeof(buf.input.length);
	memcpy(buf.input.report, report, size);

	const int err = k_msgq_put(data->msgq, buf.raw, K_NO_WAIT);
	if (err) {
		return -ENOMEM;
	}

	i2c_hid_target_irq_set(&data->target, 1);
	return 0;
}

static const struct hid_device_driver_api i2c_hid_target_device_api = {
	.submit_report = i2c_hid_target_dev_submit_report,
	.dev_register = i2c_hid_target_dev_register,
};

int i2c_hid_device_set_vid(const struct device *dev, const uint16_t vid)
{
	__ASSERT(dev->api == &i2c_hid_target_device_api, "Invalid device pointer used");

	struct i2c_hid_target_data *data = dev->data;

	if (data->ops) {
		return -EALREADY;
	}

	data->desc.wVendorID = vid;

	return 0;
}

int i2c_hid_device_set_pid(const struct device *dev, const uint16_t vid)
{
	__ASSERT(dev->api == &i2c_hid_target_device_api, "Invalid device pointer used");

	struct i2c_hid_target_data *data = dev->data;

	if (data->ops) {
		return -EALREADY;
	}

	data->desc.wProductID = vid;

	return 0;
}

static int i2c_hid_target_init(const struct device *dev)
{
	const struct i2c_hid_target_config *config = dev->config;

	int err = gpio_pin_configure_dt(&config->int_gpio, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("Failed to configure the interrupt GPIO: %d", err);
		return err;
	}

	return 0;
}

static const struct i2c_target_callbacks i2c_hid_target_callbacks = {
	.buf_write_received = i2c_hid_target_buf_write_received,
	.buf_read_requested = i2c_hid_target_buf_read_requested,
	.stop = i2c_hid_target_stop,
};

#define I2C_HID_DESCR_ADDR(inst) DT_INST_PROP(inst, hid_descr_addr)

#define DEFINE_I2C_HID_TARGET_DEVICE(inst)                                                         \
	const static struct i2c_hid_target_config i2c_hid_target_config_##inst = {                 \
		.bus = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                                \
		.hid_descr_addr = I2C_HID_DESCR_ADDR(inst),                                        \
	};                                                                                         \
	static uint8_t i2c_hid_target_input_##inst[DT_INST_PROP(inst, in_report_size)];            \
	K_MSGQ_DEFINE(i2c_hid_target_in_msgq_##inst, DT_INST_PROP(inst, in_report_size),           \
		      CONFIG_I2C_HID_TARGET_IN_BUF_COUNT, sizeof(uint32_t));                       \
	static struct i2c_hid_target_data i2c_hid_target_data_##inst = {                           \
		.target.address = DT_INST_REG_ADDR(inst),                                          \
		.target.callbacks = &i2c_hid_target_callbacks,                                     \
		.config = &i2c_hid_target_config_##inst,                                           \
                                                                                                   \
		.desc.wHIDDescLength = sizeof(struct i2c_hid_desc),                                \
		.desc.bcdVersion = 0x0100,                                                         \
		.desc.wMaxInputLength = DT_INST_PROP(inst, in_report_size),                        \
		.desc.wMaxOutputLength = 0,                                                        \
		.desc.wReportDescRegister = I2C_HID_DESCR_ADDR(inst) + 1,                          \
		.reg = I2C_HID_DESCR_ADDR(inst) + 2,                                               \
		.desc.wInputRegister = I2C_HID_DESCR_ADDR(inst) + 2,                               \
		.desc.wOutputRegister = I2C_HID_DESCR_ADDR(inst) + 3,                              \
		.desc.wCommandRegister = I2C_HID_DESCR_ADDR(inst) + 4,                             \
		.desc.wDataRegister = I2C_HID_DESCR_ADDR(inst) + 5,                                \
		.msgq = &i2c_hid_target_in_msgq_##inst,                                            \
		.input = (struct i2c_hid_input_register *)i2c_hid_target_input_##inst,             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &i2c_hid_target_init, NULL, &i2c_hid_target_data_##inst,       \
			      &i2c_hid_target_config_##inst, POST_KERNEL,                          \
			      CONFIG_APPLICATION_INIT_PRIORITY, &i2c_hid_target_device_api)

#define DT_DRV_COMPAT zephyr_hid_i2c_device
DT_INST_FOREACH_STATUS_OKAY(DEFINE_I2C_HID_TARGET_DEVICE);
