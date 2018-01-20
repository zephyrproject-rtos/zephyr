#include <gpio.h>
#include <spi.h>

#include <lorawan/sx1276.h>

extern DioIrqHandler *DioIrq[];

static struct spi_cs_control spi_cs = {
	.gpio_pin = 3,
	.delay = 0,
};

static spi_config spi_config = {
	.frequency = 16000000,
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
		SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = 1,
	.cs = &spi_cs,
};

static char dio_gpio_dev_name_table[6] = {
	CONFIG_SX1272_DIO0_DEV_NAME,
	CONFIG_SX1272_DIO1_DEV_NAME,
	CONFIG_SX1272_DIO2_DEV_NAME,
	CONFIG_SX1272_DIO3_DEV_NAME,
	CONFIG_SX1272_DIO4_DEV_NAME,
	CONFIG_SX1272_DIO5_DEV_NAME,
};

static uint8_t dio_gpio_pin_table[6] = {
	CONFIG_SX1272_DIO0_PIN,
	CONFIG_SX1272_DIO1_PIN,
	CONFIG_SX1272_DIO2_PIN,
	CONFIG_SX1272_DIO3_PIN,
	CONFIG_SX1272_DIO4_PIN,
	CONFIG_SX1272_DIO5_PIN,
}

int bus_spi_init(void)
{
	spi_cs.gpio_dev = device_get_binding("GPIOA");
	if ( NULL == spi_cs.gpio_dev )
	{
		SYS_LOG_ERR("Can not find device GPIOA");
		return -1;
	}

	spi_config.dev = device_get_binding("SPI_DEV");
	if ( NULL == spi_config.dev )
	{
		SYS_LOG_ERR("Can not find device SPI_DEV");
		return -1;
	}

	return 0;
}

void sx1272_io_irq_handler(struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins)
{
	uint32_t pin = 0;

	while ( 1 != pins )
	{
		pins >>= 1;
		pin++;
	}

	for ( i = 0; i < 6; ++i )
	{
		if ( dio_gpio_pin_table[i] == pin )
		{
			dev_temp = device_get_binding(dio_gpio_dev_name_table[i]);
			if ( dev == dev_temp )
			{
				(*DioIrq[i])();
				break;
			}
		}
	}
}

void SX1272IoIrqInit( DioIrqHandler **irqHandlers )
{
	static struct gpio_callback gpio_cb[6];
	static int gpio_cb_number = 0;

	bool gpio_initialized_table[6] = {
		false, false, false, false, false, false,
	};

	struct device *dev = NULL;

	int i, j, pin_mask;

	for ( i = 0, gpio_cb_number = 0; i < 6; ++i )
	{
		if ( gpio_initialized_table[i] )
		{
			/* Already initialized, skip to next pin */
			continue;
		}

		dev = device_get_binding(gpio_dev_name_table[i]);
		pin_mask = 0;

		/* Get the same gpio device pins into pin_mask */
		for ( j = i; j < 6; ++j )
		{
			if ( 0 == strcmp(dio_gpio_dev_name_table[i], dio_gpio_dev_name_table[j]) )
			{
				gpio_pin_configure(dev, dio_gpio_pin_table[j],
						GPIO_DIR_IN | GPIO_INT | GPIO_INT_DEBOUNCE |	\
						GPIO_PUD_PULL_DOWN | GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH);
				pin_mask |= BIT(dio_gpio_pin_table[j]);

				/* Skip current pin on initialization */
				gpio_initialized_table[j] = true;
			}
		}

		/* Initial callback */
		gpio_init_callback(&gpio_cb[gpio_cb_number], sx1272_io_irq_handler, pin_mask);

		/* Add one particular callback structure */
		gpio_add_callback(dev, &gpio_cb[gpio_cb_number]);

		gpio_cb_number++;
	}
}

void SX1272Reset( void )
{
	struct device *dev = device_get_binding(CONFIG_SX1276_RESET_DEV_NAME);
	if ( NULL == dev )
	{
		return ;
	}

	gpio_pin_configure(dev, CONFIG_SX1276_RESET_PIN, GPIO_DIR_OUT | GPIO_PUD_NORMAL);
	gpio_pin_write(dev, CONFIG_SX1276_RESET_PIN, 0);

	k_sleep( 1 );

	gpio_pin_configure(dev, CONFIG_SX1276_RESET_PIN, GPIO_DIR_IN | GPIO_PUD_NORMAL);
	k_sleep( 6 );
}

void SX1272WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
	addr |= 0x80;

	struct spi_buf tx_bufs[] = {
		{
			.buf = &addr,
			.len = 1,
		},
		{
			.buf = buffer,
			.len = size,
		},
	};

	spi_write(spi_config, tx_bufs, ARRAY_SIZE(tx_bufs));
}

void SX1272ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
	addr &= 0x7F;

	struct spi_buf tx_bufs[] = {
		{
			.buf = &addr,
			.len = 1,
		},
	};

	spi_write(spi_config, tx_bufs, ARRAY_SIZE(tx_bufs));

	struct spi_buf rx_bufs[] = {
		{
			.buf = buffer,
			.len = size,
		},
	};

	spi_read(spi_config, rx_bufs, ARRAY_SIZE(rx_bufs));
}
