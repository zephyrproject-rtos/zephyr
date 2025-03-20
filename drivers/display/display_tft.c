#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/drivers/display.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/dt-bindings/display/ili9xxx.h>
#include "display_ili9341.h"
#include "display_ili9xxx.h"


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_ili9341x, CONFIG_DISPLAY_LOG_LEVEL);

#define ILI9341_DC_PIN 5
#define ILI9341_CS_PIN 0
#define ILI9341_RES_PIN 4

/* Command/data GPIO level for commands. */
#define ILI9341_GPIO_LEVEL_CMD 0U

/* Command/data GPIO level for data. */
#define ILI9341_GPIO_LEVEL_DATA 1U


struct ili9xxx_data {
	uint8_t bytes_per_pixel;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
};

const struct device *dev2 = DEVICE_DT_GET(DT_NODELABEL(gpio0));

int ili9341_transmission(const struct device *dev, uint8_t cmd, int *tx_data,
		                    size_t tx_len)
{
    const struct ili9xxx_config *config = dev->config;
    uint8_t tx_buff1[1];
    tx_buff1[0] = cmd;
    // int len = 1;

    struct spi_buf cmdtx_buf = { .buf = tx_buff1, .len = 1 };
	struct spi_buf_set cmdtx_bufs = { .buffers = &cmdtx_buf, .count = 1};

    // struct spi_buf rx_buf = { .buf = rx_buff, .len = len};
    // struct spi_buf_set rx_bufs = { .buffers = &rx_buf, .count = 1};

    gpio_port_clear_bits_raw(dev2, ILI9341_DC_PIN);

    int ret = spi_transceive(dev, &config, &cmdtx_bufs, NULL);

    gpio_port_set_bits_raw(dev2, ILI9341_DC_PIN);

	if (tx_len == 0)
	{
		return ret ;
	}
	
    uint8_t tx_buff2[8];
    for (int i=0; i<tx_len; i++){
        tx_buff2[i] = tx_data[i];
    }
    // len = tx_len;

    struct spi_buf datatx_buf = { .buf = tx_buff2, .len = tx_len };
	struct spi_buf_set datatx_bufs = { .buffers = &datatx_buf, .count = 1};

    ret = spi_transceive(dev, &config, &datatx_bufs, NULL);

    return ret;

}

static int ili9341_exit_sleep(const struct device *dev)
{
	int r;

	r = ili9341_transmission(dev, ILI9XXX_SLPOUT, NULL, 0);
	if (r < 0) {
		return r;
	}

	k_sleep(K_MSEC(ILI9XXX_SLEEP_OUT_TIME));

	return 0;
}


static int ili9341_set_mem_area(const struct device *dev, const uint16_t x,
				const uint16_t y, const uint16_t w,
				const uint16_t h)
{
	int r;
	uint16_t spi_data[2];

	spi_data[0] = sys_cpu_to_be16(x);
	spi_data[1] = sys_cpu_to_be16(x + w - 1U);
	r = ili9341_transmission(dev, ILI9XXX_CASET, &spi_data[0], 4U);
	if (r < 0) {
		return r;
	}

	spi_data[0] = sys_cpu_to_be16(y);
	spi_data[1] = sys_cpu_to_be16(y + h - 1U);
	r = ili9341_transmission(dev, ILI9XXX_PASET, &spi_data[0], 4U);
	if (r < 0) {
		return r;
	}

	return 0;
}


static int ili9341_write(const struct device *dev, const uint16_t x, 
						const uint16_t y,
			 			const struct display_buffer_descriptor *desc, 
						const void *buf)
{
	const struct ili9xxx_config *config = dev->config;
	struct ili9xxx_data *data = dev->data;

	int ret;
	const uint8_t *write_data_start = (const uint8_t *)buf;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs;
	uint16_t write_cnt;
	uint16_t nbr_of_writes;
	uint16_t write_h;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT((desc->pitch * data->bytes_per_pixel * desc->height) <= desc->buf_size,
		 "Input buffer to small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)", desc->width, desc->height, x, y);
	ret = gc9x01x_set_mem_area(dev, x, y, desc->width, desc->height);
	if (ret < 0) {
		return ret;
	}

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
	} else {
		write_h = desc->height;
		nbr_of_writes = 1U;
	}

	ret = ili9341_transmission(dev, ILI9XXX_RAMWR, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	tx_bufs.buffers = &tx_buf;
	tx_bufs.count = 1U;

	write_data_start += desc->pitch * data->bytes_per_pixel;
	for (write_cnt = 1U; write_cnt < nbr_of_writes; ++write_cnt) {
		tx_buf.buf = (void *)write_data_start;
		tx_buf.len = desc->width * data->bytes_per_pixel * write_h;

		ret = spi_write_dt(&config->spi_dev, &tx_bufs);
		if (ret < 0) {
			return ret;
		}

		write_data_start += desc->pitch * data->bytes_per_pixel;
	}

	return 0;
}


// static int ili9341_hw_reset(const struct device *dev)
// {
// 	// const struct gc9x01x_config *config = dev->config;
// 	const struct ili9xxx_config *config = dev->config;

// 	if (config->reset.port == NULL) {
// 		return -ENODEV;
// 	}

// 	gpio_pin_set_dt(&config->reset, 1U);
// 	// k_msleep(100);
// 	gpio_pin_set_dt(&config->reset, 0U);
// 	// k_msleep(10);

// 	return 0;
// }


static int ili9341_set_pixel_format(const struct device *dev,
			 const enum display_pixel_format pixel_format)
{
	struct ili9xxx_data *data = dev->data;

	int r;
	uint8_t tx_data;
	uint8_t bytes_per_pixel;

	if (pixel_format == PIXEL_FORMAT_RGB_565) {
		bytes_per_pixel = 2U;
		tx_data = ILI9XXX_PIXSET_MCU_16_BIT | ILI9XXX_PIXSET_RGB_16_BIT;
	} else if (pixel_format == PIXEL_FORMAT_RGB_888) {
		bytes_per_pixel = 3U;
		tx_data = ILI9XXX_PIXSET_MCU_18_BIT | ILI9XXX_PIXSET_RGB_18_BIT;
	} else {
		LOG_ERR("Unsupported pixel format");
		return -ENOTSUP;
	}

	r = ili9341_transmission(dev, ILI9XXX_PIXSET, &tx_data, 1U);
	if (r < 0) {
		return r;
	}

	data->pixel_format = pixel_format;
	data->bytes_per_pixel = bytes_per_pixel;

	return 0;
}

static int ili9341_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	const struct ili9xxx_config *config = dev->config;
	struct ili9xxx_data *data = dev->data;

	int r;
	uint8_t tx_data = ILI9XXX_MADCTL_BGR;
	if (config->quirks->cmd_set == CMD_SET_1) {
		if (orientation == DISPLAY_ORIENTATION_NORMAL) {
			tx_data |= ILI9XXX_MADCTL_MX;
		} else if (orientation == DISPLAY_ORIENTATION_ROTATED_90) {
			tx_data |= ILI9XXX_MADCTL_MV;
		} else if (orientation == DISPLAY_ORIENTATION_ROTATED_180) {
			tx_data |= ILI9XXX_MADCTL_MY;
		} else if (orientation == DISPLAY_ORIENTATION_ROTATED_270) {
			tx_data |= ILI9XXX_MADCTL_MV | ILI9XXX_MADCTL_MX |
				   ILI9XXX_MADCTL_MY;
		}
	} else if (config->quirks->cmd_set == CMD_SET_2) {
		if (orientation == DISPLAY_ORIENTATION_NORMAL) {
			/* Do nothing */
		} else if (orientation == DISPLAY_ORIENTATION_ROTATED_90) {
			tx_data |= ILI9XXX_MADCTL_MV | ILI9XXX_MADCTL_MY;
		} else if (orientation == DISPLAY_ORIENTATION_ROTATED_180) {
			tx_data |= ILI9XXX_MADCTL_MY | ILI9XXX_MADCTL_MX;
		} else if (orientation == DISPLAY_ORIENTATION_ROTATED_270) {
			tx_data |= ILI9XXX_MADCTL_MV | ILI9XXX_MADCTL_MX;
		}
	}

	r = ili9341_transmission(dev, ILI9XXX_MADCTL, &tx_data, 1U);
	if (r < 0) {
		return r;
	}

	data->orientation = orientation;

	return 0;
}

static void ili9341_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	struct ili9xxx_data *data = dev->data;
	const struct ili9xxx_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->supported_pixel_formats =
		PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_RGB_888;
	capabilities->current_pixel_format = data->pixel_format;

	if (data->orientation == DISPLAY_ORIENTATION_NORMAL ||
	    data->orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		capabilities->x_resolution = config->x_resolution;
		capabilities->y_resolution = config->y_resolution;
	} else {
		capabilities->x_resolution = config->y_resolution;
		capabilities->y_resolution = config->x_resolution;
	}

	capabilities->current_orientation = data->orientation;
}

static int ili9341_configure(const struct device *dev)
{
	const struct ili9xxx_config *config = dev->config;

	int r;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;

	/* pixel format */
	if (config->pixel_format == ILI9XXX_PIXEL_FORMAT_RGB565) {
		pixel_format = PIXEL_FORMAT_RGB_565;
	} else {
		pixel_format = PIXEL_FORMAT_RGB_888;
	}

	r = ili9341_set_pixel_format(dev, pixel_format);
	if (r < 0) {
		return r;
	}

	/* orientation */
	if (config->rotation == 0U) {
		orientation = DISPLAY_ORIENTATION_NORMAL;
	} else if (config->rotation == 90U) {
		orientation = DISPLAY_ORIENTATION_ROTATED_90;
	} else if (config->rotation == 180U) {
		orientation = DISPLAY_ORIENTATION_ROTATED_180;
	} else {
		orientation = DISPLAY_ORIENTATION_ROTATED_270;
	}

	r = ili9341_set_orientation(dev, orientation);
	if (r < 0) {
		return r;
	}

	if (config->inversion) {
		r = ili9341_transmission(dev, ILI9XXX_DINVON, NULL, 0U);
		if (r < 0) {
			return r;
		}
	}

	r = config->regs_init_fn(dev);
	if (r < 0) {
		return r;
	}

	return 0;
}


void delay_us(uint32_t microseconds) {
    //1ms gives 2.35ms
    int temp = microseconds * 1;
    for (int i = 0; i < temp ; i++)
    {
        for (int j = 0; j < 10; j++){}
    }
}


void ILI9341_Unselect()
{
    gpio_port_set_bits_raw(dev2, ILI9341_CS_PIN);
}


void ILI9341_Select()
{
    gpio_port_clear_bits_raw(dev2, ILI9341_CS_PIN);
}


static void ILI9341_Reset()
{
    gpio_port_clear_bits_raw(dev2, ILI9341_RES_PIN);
	delay_us(1000);
	gpio_port_set_bits_raw(dev2, ILI9341_RES_PIN);
}


int ili9341_regs_initialization(const struct device *dev)
{
    const struct ili9xxx_config *config = dev->config;
    const struct ili9341_regs *regs = config->regs;

    gpio_pin_configure(dev, ILI9341_DC_PIN, 1);
	gpio_pin_configure(dev, ILI9341_CS_PIN, 1);
	gpio_pin_configure(dev, ILI9341_RES_PIN, 1);
    gpio_port_set_bits_raw(dev2, ILI9341_DC_PIN);

    int r;

    struct spi_config configure;
    configure.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8);

	ILI9341_Unselect();
	ILI9341_Select();
	ILI9341_Reset();

    LOG_HEXDUMP_DBG(regs->pwseqctrl, ILI9341_PWSEQCTRL_LEN, "PWSEQCTRL");
    r = ili9341_transmission(dev, ILI9341_PWSEQCTRL, regs->pwseqctrl, ILI9341_PWSEQCTRL_LEN);
    if (r != 0) {
		return r;
	}
    
    LOG_HEXDUMP_DBG(regs->timctrla, ILI9341_TIMCTRLA_LEN, "TIMCTRLA");
	r = ili9341_transmission(dev, ILI9341_TIMCTRLA, regs->timctrla, ILI9341_TIMCTRLA_LEN);
	if (r != 0) {
		return r;
	}
    
    LOG_HEXDUMP_DBG(regs->timctrlb, ILI9341_TIMCTRLB_LEN, "TIMCTRLB");
	r = ili9341_transmission(dev, ILI9341_TIMCTRLB, regs->timctrlb, ILI9341_TIMCTRLB_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->pumpratioctrl, ILI9341_PUMPRATIOCTRL_LEN, "PUMPRATIOCTRL");
	r = ili9341_transmission(dev, ILI9341_PUMPRATIOCTRL, regs->pumpratioctrl,
			     ILI9341_PUMPRATIOCTRL_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->pwctrla, ILI9341_PWCTRLA_LEN, "PWCTRLA");
	r = ili9341_transmission(dev, ILI9341_PWCTRLA, regs->pwctrla, ILI9341_PWCTRLA_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->pwctrlb, ILI9341_PWCTRLB_LEN, "PWCTRLB");
	r = ili9341_transmission(dev, ILI9341_PWCTRLB, regs->pwctrlb, ILI9341_PWCTRLB_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->gamset, ILI9341_GAMSET_LEN, "GAMSET");
	r = ili9341_transmission(dev, ILI9341_GAMSET, regs->gamset, ILI9341_GAMSET_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->frmctr1, ILI9341_FRMCTR1_LEN, "FRMCTR1");
	r = ili9341_transmission(dev, ILI9341_FRMCTR1, regs->frmctr1, ILI9341_FRMCTR1_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->disctrl, ILI9341_DISCTRL_LEN, "DISCTRL");
	r = ili9341_transmission(dev, ILI9341_DISCTRL, regs->disctrl, ILI9341_DISCTRL_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->pwctrl1, ILI9341_PWCTRL1_LEN, "PWCTRL1");
	r = ili9341_transmission(dev, ILI9341_PWCTRL1, regs->pwctrl1, ILI9341_PWCTRL1_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->pwctrl2, ILI9341_PWCTRL2_LEN, "PWCTRL2");
	r = ili9341_transmission(dev, ILI9341_PWCTRL2, regs->pwctrl2, ILI9341_PWCTRL2_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->vmctrl1, ILI9341_VMCTRL1_LEN, "VMCTRL1");
	r = ili9341_transmission(dev, ILI9341_VMCTRL1, regs->vmctrl1, ILI9341_VMCTRL1_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->vmctrl2, ILI9341_VMCTRL2_LEN, "VMCTRL2");
	r = ili9341_transmission(dev, ILI9341_VMCTRL2, regs->vmctrl2, ILI9341_VMCTRL2_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->pgamctrl, ILI9341_PGAMCTRL_LEN, "PGAMCTRL");
	r = ili9341_transmission(dev, ILI9341_PGAMCTRL, regs->pgamctrl, ILI9341_PGAMCTRL_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->ngamctrl, ILI9341_NGAMCTRL_LEN, "NGAMCTRL");
	r = ili9341_transmission(dev, ILI9341_NGAMCTRL, regs->ngamctrl, ILI9341_NGAMCTRL_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->enable3g, ILI9341_ENABLE3G_LEN, "ENABLE3G");
	r = ili9341_transmission(dev, ILI9341_ENABLE3G, regs->enable3g, ILI9341_ENABLE3G_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->ifmode, ILI9341_IFMODE_LEN, "IFMODE");
	r = ili9341_transmission(dev, ILI9341_IFMODE, regs->ifmode, ILI9341_IFMODE_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->ifctl, ILI9341_IFCTL_LEN, "IFCTL");
	r = ili9341_transmission(dev, ILI9341_IFCTL, regs->ifctl, ILI9341_IFCTL_LEN);
	if (r != 0) {
		return r;
	}

    LOG_HEXDUMP_DBG(regs->etmod, ILI9341_ETMOD_LEN, "ETMOD");
	r = ili9341_transmission(dev, ILI9341_ETMOD, regs->etmod, ILI9341_ETMOD_LEN);
	if (r != 0) {
		return r;
	}

	ILI9341_Unselect();

    return 0;
}

static int ili9341_display_blanking_on(const struct device *dev)
{
	LOG_DBG("Turning display blanking on");
	return ili9341_transmission(dev, ILI9XXX_DISPOFF, NULL, 0);
}

static int ili9341_display_blanking_off(const struct device *dev)
{
	LOG_DBG("Turning display blanking off");
	return ili9341_transmission(dev, ILI9XXX_DISPON, NULL, 0);
}


static const struct display_driver_api ili9341_api = {
	.blanking_on = ili9341_display_blanking_on,
	.blanking_off = ili9341_display_blanking_off,
	.write = ili9341_write,
	.get_capabilities = ili9341_get_capabilities,
	.set_pixel_format = ili9341_set_pixel_format,
	.set_orientation = ili9341_set_orientation,
};


// 	ili9xxx_write
//	ili9xxx_hw_reset
//	ili9xx_init

static const struct ili9xxx_quirks ili9341_quirks = {
	.cmd_set = CMD_SET_1,
};

#define GC9X01X_INIT(inst)                                                                         \
	GC9X01X_REGS_INIT(inst);                                                                   \
	static const struct gc9x01x_config gc9x01x_config_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0) ,        \
		.cmd_data = GPIO_DT_SPEC_INST_GET(inst, cmd_data_gpios),                           \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
		.pixel_format = DT_INST_PROP(inst, pixel_format),                                  \
		.orientation = DT_INST_ENUM_IDX(inst, orientation),                                \
		.x_resolution = DT_INST_PROP(inst, width),                                         \
		.y_resolution = DT_INST_PROP(inst, height),                                        \
		.inversion = DT_INST_PROP(inst, display_inversion),\
		.regs = &gc9x01x_regs_##inst,                                                      \
	};                                                                                         \
	static struct gc9x01x_data gc9x01x_data_##inst;                                            \
	PM_DEVICE_DT_INST_DEFINE(inst, gc9x01x_pm_action);                                         \
	DEVICE_DT_INST_DEFINE(inst, &gc9x01x_init, PM_DEVICE_DT_INST_GET(inst),                    \
			      &gc9x01x_data_##inst, &gc9x01x_config_##inst, POST_KERNEL,           \
			      CONFIG_DISPLAY_INIT_PRIORITY, &gc9x01x_api);

DT_INST_FOREACH_STATUS_OKAY(TFT_INIT)
// #define INST_DT_ILI9XXX(n, t) DT_INST(n, ilitek_ili##t)

// #define ILI9XXX_INIT(n, t)                                                     \
// 	ILI##t##_REGS_INIT(n);                                                 \
// 									       \
// 	static const struct ili9xxx_config ili9xxx_config_##n = {              \
// 		.quirks = &ili##t##_quirks,                                    \
// 		.mipi_dev = DEVICE_DT_GET(DT_PARENT(INST_DT_ILI9XXX(n, t))),   \
// 		.dbi_config = {                                                \
// 			.mode = MIPI_DBI_MODE_SPI_4WIRE,                       \
// 			.config = MIPI_DBI_SPI_CONFIG_DT(                      \
// 						INST_DT_ILI9XXX(n, t),         \
// 						SPI_OP_MODE_MASTER |           \
// 						SPI_WORD_SET(8),               \
// 						0),                            \
// 		},                                                             \
// 		.pixel_format = DT_PROP(INST_DT_ILI9XXX(n, t), pixel_format),  \
// 		.rotation = DT_PROP(INST_DT_ILI9XXX(n, t), rotation),          \
// 		.x_resolution = ILI##t##_X_RES,                                \
// 		.y_resolution = ILI##t##_Y_RES,                                \
// 		.inversion = DT_PROP(INST_DT_ILI9XXX(n, t), display_inversion),\
// 		.regs = &ili9xxx_regs_##n,                                     \
// 		.regs_init_fn = ili##t##_regs_init,                            \
// 	};                                                                     \
// 									       \
// 	static struct ili9xxx_data ili9xxx_data_##n;                           \
// 									       \
// 	DEVICE_DT_DEFINE(INST_DT_ILI9XXX(n, t), ili9xxx_init,                  \
// 			    NULL, &ili9xxx_data_##n,                           \
// 			    &ili9xxx_config_##n, POST_KERNEL,                  \
// 			    CONFIG_DISPLAY_INIT_PRIORITY, &ili9xxx_api)

// #define DT_INST_FOREACH_ILI9XXX_STATUS_OKAY(t)                                 \
// 	LISTIFY(DT_NUM_INST_STATUS_OKAY(ilitek_ili##t), ILI9XXX_INIT, (;), t)

	
// DT_INST_FOREACH_ILI9XXX_STATUS_OKAY(9341);