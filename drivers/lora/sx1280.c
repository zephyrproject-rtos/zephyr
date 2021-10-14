/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 * Copyright (c) 2020 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <drivers/lora.h>
#include <drivers/spi.h>
#include <zephyr.h>

#include "sx12xx_common.h"
#include "sx1280.h"
#include "sx1280-radio.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(sx1280, CONFIG_LORA_LOG_LEVEL);

#define DT_DRV_COMPAT semtech_sx1280

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(semtech_sx1280) <= 1,
	     "Multiple SX1280 instances in DT");

#define GPIO_RESET_PIN		DT_INST_GPIO_PIN(0, reset_gpios)
#define GPIO_CS_PIN		DT_INST_SPI_DEV_CS_GPIOS_PIN(0)
#define GPIO_CS_FLAGS		DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0)

#define GPIO_ANTENNA_ENABLE_PIN				\
	DT_INST_GPIO_PIN(0, antenna_enable_gpios)
#define GPIO_RFI_ENABLE_PIN			\
	DT_INST_GPIO_PIN(0, rfi_enable_gpios)
#define GPIO_RFO_ENABLE_PIN			\
	DT_INST_GPIO_PIN(0, rfo_enable_gpios)
#define GPIO_PA_BOOST_ENABLE_PIN			\
	DT_INST_GPIO_PIN(0, pa_boost_enable_gpios)

#define GPIO_TCXO_POWER_PIN	DT_INST_GPIO_PIN(0, tcxo_power_gpios)

#if DT_INST_NODE_HAS_PROP(0, tcxo_power_startup_delay_ms)
#define TCXO_POWER_STARTUP_DELAY_MS			\
	DT_INST_PROP(0, tcxo_power_startup_delay_ms)
#else
#define TCXO_POWER_STARTUP_DELAY_MS		0
#endif

bool mode_tx = true;
struct k_sem recv_sem;

struct sx1280_dio {
	const char *port;
	gpio_pin_t pin;
	gpio_dt_flags_t flags;
};

/* Helper macro that UTIL_LISTIFY can use and produces an element with comma */
#define SX1280_DIO_GPIO_ELEM(idx, inst) \
	{ \
		DT_INST_GPIO_LABEL_BY_IDX(inst, dio_gpios, idx), \
		DT_INST_GPIO_PIN_BY_IDX(inst, dio_gpios, idx), \
		DT_INST_GPIO_FLAGS_BY_IDX(inst, dio_gpios, idx), \
	},

#define SX1280_DIO_GPIO_INIT(n) \
	UTIL_LISTIFY(DT_INST_PROP_LEN(n, dio_gpios), SX1280_DIO_GPIO_ELEM, n)

static const struct sx1280_dio sx1280_dios[] = { SX1280_DIO_GPIO_INIT(0) };

static struct sx1280_data {
	const struct device *reset;
#if DT_INST_NODE_HAS_PROP(0, antenna_enable_gpios)
	const struct device *antenna_enable;
#endif
#if DT_INST_NODE_HAS_PROP(0, rfi_enable_gpios)
	const struct device *rfi_enable;
#endif
#if DT_INST_NODE_HAS_PROP(0, rfo_enable_gpios)
	const struct device *rfo_enable;
#endif
#if DT_INST_NODE_HAS_PROP(0, pa_boost_enable_gpios)
	const struct device *pa_boost_enable;
#endif
#if DT_INST_NODE_HAS_PROP(0, rfo_enable_gpios) &&	\
	DT_INST_NODE_HAS_PROP(0, pa_boost_enable_gpios)
	uint8_t tx_power;
#endif
#if DT_INST_NODE_HAS_PROP(0, tcxo_power_gpios)
	const struct device *tcxo_power;
	bool tcxo_power_enabled;
#endif
	const struct device *spi;
	struct spi_config spi_cfg;
	const struct device *dio_dev[1];
	struct k_work dio_work[1];
} dev_data;

bool SX127xCheckRfFrequency(uint32_t frequency)
{
	/* TODO */
	return true;
}

uint32_t SX127xGetBoardTcxoWakeupTime(void)
{
	return TCXO_POWER_STARTUP_DELAY_MS;
}

static inline void sx127x_antenna_enable(int val)
{
#if DT_INST_NODE_HAS_PROP(0, antenna_enable_gpios)
	gpio_pin_set(dev_data.antenna_enable, GPIO_ANTENNA_ENABLE_PIN, val);
#endif
}

static inline void sx127x_rfi_enable(int val)
{
#if DT_INST_NODE_HAS_PROP(0, rfi_enable_gpios)
	gpio_pin_set(dev_data.rfi_enable, GPIO_RFI_ENABLE_PIN, val);
#endif
}

static inline void sx127x_rfo_enable(int val)
{
#if DT_INST_NODE_HAS_PROP(0, rfo_enable_gpios)
	gpio_pin_set(dev_data.rfo_enable, GPIO_RFO_ENABLE_PIN, val);
#endif
}

static inline void sx127x_pa_boost_enable(int val)
{
#if DT_INST_NODE_HAS_PROP(0, pa_boost_enable_gpios)
	gpio_pin_set(dev_data.pa_boost_enable, GPIO_PA_BOOST_ENABLE_PIN, val);
#endif
}

void SX127xSetAntSwLowPower(bool low_power)
{
	if (low_power) {
		/* force inactive (low power) state of all antenna paths */
		sx127x_rfi_enable(0);
		sx127x_rfo_enable(0);
		sx127x_pa_boost_enable(0);

		sx127x_antenna_enable(0);
	} else {
		sx127x_antenna_enable(1);

		/* rely on SX127xSetAntSw() to configure proper antenna path */
	}
}

void SX127xSetBoardTcxo(uint8_t state)
{
#if DT_INST_NODE_HAS_PROP(0, tcxo_power_gpios)
	bool enable = state;

	if (enable == dev_data.tcxo_power_enabled) {
		return;
	}

	if (enable) {
		gpio_pin_set(dev_data.tcxo_power, GPIO_TCXO_POWER_PIN, 1);

		if (TCXO_POWER_STARTUP_DELAY_MS > 0) {
			k_sleep(K_MSEC(TCXO_POWER_STARTUP_DELAY_MS));
		}
	} else {
		gpio_pin_set(dev_data.tcxo_power, GPIO_TCXO_POWER_PIN, 0);
	}

	dev_data.tcxo_power_enabled = enable;
#endif
}

void sx1280_WriteCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
	// WaitOnBusy( ); // TODO

	int ret;

	const struct spi_buf buf[2] = {
		{
			.buf = &command,
			.len = sizeof(command)
		},
		{
			.buf = buffer,
			.len = size
		}
	};

	struct spi_buf_set tx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	ret = spi_write(dev_data.spi, &dev_data.spi_cfg, &tx);
	// printk("wc3: %x\n", ret);

	if (ret < 0) {
		LOG_ERR("Unable to write command: 0x%x", ( uint8_t ) command);
	}

	// if( command != RADIO_SET_SLEEP )
	// {
	// 	WaitOnBusy( ); // TODO
	// }
}


void sx1280_ReadCommand( RadioCommands_t command, void *buffer, size_t size )
{
//     WaitOnBusy( );
	int ret;
	// RadioNss = 0;
	// printk("size: %u\n", size);
	// printk("command size: %u\n", sizeof(command));
	const struct spi_buf buf[3] = {
		{
			.buf = &command,
			.len = 1
		},
		{
			.buf = NULL,
			.len = 1
		},
		{
			.buf = buffer,
			.len = size
		}
	};

	struct spi_buf_set tx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	const struct spi_buf_set rx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf)
	};

	// ret = spi_write(dev_data.spi, &dev_data.spi_cfg, &tx);
	ret = spi_transceive(dev_data.spi, &dev_data.spi_cfg, &tx, &rx);
	// k_sleep(K_MSEC(1000));
	// for (size_t i = 0; i < 100000; i++)
	// {
	// }

	// printk("ret: %d\n", ret); // This printk fixes spi_transceive sending too few bytes
	if (ret < 0) {
		LOG_ERR("Unable to write command: 0x%x", ( uint8_t ) command);
	}
	// if( command == RADIO_GET_STATUS )
	// {
	// 	buffer[0] = RadioSpi->write( ( uint8_t )command );
	// 	RadioSpi->write( 0 );
	// 	RadioSpi->write( 0 );
	// }
	// else
	// {
	// 	RadioSpi->write( ( uint8_t )command );
	// 	RadioSpi->write( 0 );
	// 	for( uint16_t i = 0; i < size; i++ )
	// 	{
	// 		buffer[i] = RadioSpi->write( 0 );
	// 	}
	// }
	// RadioNss = 1;

//     WaitOnBusy( );
}

void sx1280_WriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
//     WaitOnBusy( ); // TODO

	// printk("test1.2\n");
    	// GpioWrite( &SX1276.Spi.Nss, 0 ); // TODO
	// gpio_pin_set(dev_data.spi, GPIO_CS_PIN, 0); // TODO

	// printk("test2 %u\n", size);

	// RadioSpi->write( RADIO_WRITE_BUFFER );
	int ret;
	int8_t command = RADIO_WRITE_BUFFER;

	// ret = sx1280_write(addr, buffer, size);

	const struct spi_buf buf[3] = {
		{
			.buf = &command,
			.len = 1
		},
		{
			.buf = &offset,
			.len = sizeof(offset)
		},
		{
			.buf = buffer,
			.len = size
		}
	};

	struct spi_buf_set tx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	ret = spi_write(dev_data.spi, &dev_data.spi_cfg, &tx);

	if (ret < 0) {
		LOG_ERR("Unable to write address: 0x%x", offset);
	}

//     WaitOnBusy( );
}


void sx1280_ReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
//     WaitOnBusy( );

	int ret;
	uint8_t command = RADIO_READ_BUFFER;

	const struct spi_buf buf[4] = {
		{
			.buf = &command,
			.len = 1
		},
		{
			.buf = &offset,
			.len = sizeof(offset)
		},
		{
			.buf = NULL,
			.len = 1
		},
		{
			.buf = buffer,
			.len = size
		}
	};

	struct spi_buf_set tx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	const struct spi_buf_set rx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf)
	};

	ret = spi_transceive(dev_data.spi, &dev_data.spi_cfg, &tx, &rx);

	if (ret < 0) {
		LOG_ERR("Unable to read address: 0x%x", offset);
	}

//     WaitOnBusy( );
}

void sx1280_ClearIrqStatus( uint16_t irqMask )
{
    uint8_t buf[2];

    buf[0] = ( uint8_t )( ( ( uint16_t )irqMask >> 8 ) & 0x00FF );
    buf[1] = ( uint8_t )( ( uint16_t )irqMask & 0x00FF );
    sx1280_WriteCommand( RADIO_CLR_IRQSTATUS, buf, 2 );
}

void sx1280_SetTx( TickTime_t timeout )
{
	mode_tx = true;
    sx1280_ClearIrqStatus( IRQ_RADIO_ALL );

    uint8_t buf[3];
    buf[0] = timeout.PeriodBase;
    buf[1] = ( uint8_t )( ( timeout.PeriodBaseCount >> 8 ) & 0x00FF );
    buf[2] = ( uint8_t )( timeout.PeriodBaseCount & 0x00FF );

    // If the radio is doing ranging operations, then apply the specific calls
    // prior to SetTx
    // TODO
//     if( GetPacketType( true ) == PACKET_TYPE_RANGING )
//     {
//         SetRangingRole( RADIO_RANGING_ROLE_MASTER );
//     }
    sx1280_WriteCommand( RADIO_SET_TX, buf, 3 );
//     OperatingMode = MODE_TX; // TODO
}

void sx1280_SetPayload( uint8_t *buffer, uint8_t size, uint8_t offset )
{
    sx1280_WriteBuffer( offset, buffer, size );
}

uint16_t sx1280_readIrqStatus()
{
	uint16_t temp;
	uint8_t buffer[2];

	sx1280_ReadCommand(RADIO_GET_IRQSTATUS, &buffer, 2);
	// printk("buffers: %x %x\n", buffer[0], buffer[1]);
	// LOG_INF("%x %x\n", &buffer[0], &buffer[1]);
	temp = ((buffer[0] << 8) + buffer[1]);
	// printk("buffers shifted: %x\n", temp);
	return temp;
}

static void sx1280_dio_work_handle(struct k_work *work)
{
	if (mode_tx) {
		LOG_INF("transmitting done");
		if (sx1280_readIrqStatus() & IRQ_RX_TX_TIMEOUT )                        //check for timeout
		{
			LOG_ERR("timeout");
			return;
		}
		else
		{
			LOG_INF("no timeout");
			return;
		}
	} else {
		LOG_INF("receiving done");
		k_sem_give(&recv_sem);
	}
}

void dio0_cb_func(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	// uint16_t irqStatus;
	// uint8_t buffer[2];
	// uint8_t _RXPacketL;

	// if (mode_rx) {

	// } else {
	// 	sx1280_SetStandby(MODE_STDBY_RC);                                                   //ensure to stop further packet reception

	// 	irqStatus = sx1280_readIrqStatus();

	// 	if ( (irqStatus & IRQ_HEADER_ERROR) | (irqStatus & IRQ_CRC_ERROR) | (irqStatus & IRQ_RX_TX_TIMEOUT ) ) //check if any of the preceding IRQs is set
	// 	{
	// 		printk("rx error");
	// 		return 0;                          //packet is errored somewhere so return 0
	// 	}

	// 	sx1280_ReadCommand(RADIO_GET_RXBUFFERSTATUS, buffer, 2);
	// 	_RXPacketL = buffer[0];

	// 	if (_RXPacketL > size)               //check passed buffer is big enough for packet
	// 	{
	// 	_RXPacketL = size;                 //truncate packet if not enough space
	// 	}

	// 	RXstart = buffer[1];

	// 	RXend = RXstart + _RXPacketL;

	// 	checkBusy();

	// 	#ifdef USE_SPI_TRANSACTION           //to use SPI_TRANSACTION enable define at beginning of CPP file
	// 	SPI.beginTransaction(SPISettings(LTspeedMaximum, LTdataOrder, LTdataMode));
	// 	#endif

	// 	digitalWrite(_NSS, LOW);             //start the burst read
	// 	SPI.transfer(RADIO_READ_BUFFER);
	// 	SPI.transfer(RXstart);
	// 	SPI.transfer(0xFF);

	// 	for (index = RXstart; index < RXend; index++)
	// 	{
	// 	regdata = SPI.transfer(0);
	// 	rxbuffer[index] = regdata;
	// 	}

	// 	digitalWrite(_NSS, HIGH);

	// 	#ifdef USE_SPI_TRANSACTION
	// 	SPI.endTransaction();
	// 	#endif

	// 	return _RXPacketL;
	// }
	// printk("DIO interrupted at %" PRIu32 "\n", k_cycle_get_32());
	k_work_submit(&dev_data.dio_work[0]);
}

void sx1280_IoIrqInit()
{
	static struct gpio_callback dio0_callback;

	dev_data.dio_dev[0] = device_get_binding(sx1280_dios[0].port);
	if (dev_data.dio_dev[0] == NULL) {
		LOG_ERR("Cannot get pointer to %s device",
			sx1280_dios[0].port);
		return;
	}

	k_work_init(&dev_data.dio_work[0], sx1280_dio_work_handle);

	gpio_pin_configure(dev_data.dio_dev[0], sx1280_dios[0].pin,
				GPIO_INPUT | GPIO_INT_DEBOUNCE
				| sx1280_dios[0].flags);

	gpio_init_callback(&dio0_callback,
				dio0_cb_func,
				BIT(sx1280_dios[0].pin));

	if (gpio_add_callback(dev_data.dio_dev[0], &dio0_callback) < 0) {
		LOG_ERR("Could not set gpio callback.");
		return;
	}
	gpio_pin_interrupt_configure(dev_data.dio_dev[0],
					sx1280_dios[0].pin,
					GPIO_INT_EDGE_TO_ACTIVE);

	k_sem_init(&recv_sem, 0, K_SEM_MAX_LIMIT);
}

void sx1280_WriteRegisterSPI( uint16_t address, uint8_t *buffer, size_t size )
{
//     WaitOnBusy( ); // TODO

	// printk("wr1\n");
	// gpio_pin_set(dev_data.spi, GPIO_CS_PIN, 0); // TODO

	int ret;

	uint8_t command = RADIO_WRITE_REGISTER;
	uint8_t addr_l, addr_h;
	addr_h = address >> 8;
	addr_l = address & 0x00FF;

	const struct spi_buf buf[4] = {
		{
			.buf = &command,
			.len = 1
		},
		{
			.buf = &addr_h,
			.len = sizeof(addr_h)
		},
		{
			.buf = &addr_l,
			.len = sizeof(addr_l)
		},
		{
			.buf = buffer,
			.len = size
		}
	};

	struct spi_buf_set tx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	ret = spi_write(dev_data.spi, &dev_data.spi_cfg, &tx);

	if (ret < 0) {
		LOG_ERR("Unable to write address: 0x%x", address);
	}

	// printk("wr_pre_cs_1\n");
	// gpio_pin_set(dev_data.spi, GPIO_CS_PIN, 1); // TODO
	// printk("wr_post_cs_1\n");

//     WaitOnBusy( ); // TODO
}

void sx1280_WriteRegister( uint16_t address, uint8_t value )
{
    sx1280_WriteRegisterSPI( address, &value, 1 );
}

void sx1280_ReadRegisterSPI( uint16_t address, uint8_t *buffer, size_t size )
{
//     WaitOnBusy( ); // TODO

	// printk("rr_1\n");
    	// gpio_pin_set(dev_data.spi, GPIO_CS_PIN, 0); // TODO
	// printk("rr_2\n");

	int ret;

	uint8_t command = RADIO_READ_REGISTER;
	uint8_t addr_l, addr_h;
	addr_h = address >> 8;
	addr_l = address & 0x00FF;

	const struct spi_buf buf[5] = {
		{
			.buf = &command,
			.len = 1
		},
		{
			.buf = &addr_h,
			.len = sizeof(addr_h)
		},
		{
			.buf = &addr_l,
			.len = sizeof(addr_l)
		},
		{
			.buf = NULL,
			.len = 1
		},
		{
			.buf = buffer,
			.len = size
		},
	};

	struct spi_buf_set tx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	struct spi_buf_set rx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	ret = spi_transceive(dev_data.spi, &dev_data.spi_cfg, &tx, &rx);

	if (ret < 0) {
		LOG_ERR("Unable to write address: 0x%x", address);
	}

	// printk("rr_3\n");
	// gpio_pin_set(dev_data.spi, GPIO_CS_PIN, 1); // TODO
	// printk("rr_4\n");

//     WaitOnBusy( ); // TODO
}

uint8_t sx1280_ReadRegister( uint16_t address )
{
    uint8_t data;

    sx1280_ReadRegisterSPI( address, &data, 1 );
    return data;
}

void sx1280_SetDioIrqParams( uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask )
{
    uint8_t buf[8];

    buf[0] = ( uint8_t )( ( irqMask >> 8 ) & 0x00FF );
    buf[1] = ( uint8_t )( irqMask & 0x00FF );
    buf[2] = ( uint8_t )( ( dio1Mask >> 8 ) & 0x00FF );
    buf[3] = ( uint8_t )( dio1Mask & 0x00FF );
    buf[4] = ( uint8_t )( ( dio2Mask >> 8 ) & 0x00FF );
    buf[5] = ( uint8_t )( dio2Mask & 0x00FF );
    buf[6] = ( uint8_t )( ( dio3Mask >> 8 ) & 0x00FF );
    buf[7] = ( uint8_t )( dio3Mask & 0x00FF );
    sx1280_WriteCommand( RADIO_SET_DIOIRQPARAMS, buf, 8 );
}

void SendPayload( uint8_t *payload, uint8_t size, TickTime_t timeout, uint8_t offset )
{
	sx1280_SetPayload( payload, size, offset );
	sx1280_WriteRegister(REG_LR_PAYLOADLENGTH, size);                           //only seems to work for lora
	sx1280_SetDioIrqParams(IRQ_RADIO_ALL, (IRQ_TX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);
	sx1280_SetTx( timeout );
}

int sx1280_lora_send(const struct device *dev, uint8_t *data,
		     uint32_t data_len)
{
	// printk("lora_send1\n");
	SendPayload(data, data_len, (TickTime_t) { .PeriodBase = RADIO_TICK_SIZE_1000_US, .PeriodBaseCount = 10000 }, 0x00);
	// printk("lora_send2\n");
	// Radio.SetMaxPayloadLength(MODEM_LORA, data_len);

	// Radio.Send(data, data_len);

	return 0;
}

void sx1280_SetTxContinuousWave( void )
{
    sx1280_WriteCommand( RADIO_SET_TXCONTINUOUSWAVE, 0, 0 );
}

int sx1280_lora_test_cw(const struct device *dev, uint32_t frequency,
			int8_t tx_power,
			uint16_t duration)
{
	sx1280_SetTxContinuousWave(); // TODO: use parameters?
	return 0;
}


void sx1280_SetStandby( RadioStandbyModes_t standbyConfig )
{
    sx1280_WriteCommand( RADIO_SET_STANDBY, ( uint8_t* )&standbyConfig, 1 );
// TODO
//     if( standbyConfig == STDBY_RC )
//     {
//         OperatingMode = MODE_STDBY_RC;
//     }
//     else
//     {
//         OperatingMode = MODE_STDBY_XOSC;
//     }
}

void sx1280_SetRegulatorMode( RadioRegulatorModes_t mode )
{
    sx1280_WriteCommand( RADIO_SET_REGULATORMODE, ( uint8_t* )&mode, 1 );
}


RadioStatus_t sx1280_GetStatus( void )
{
    uint8_t stat = 0;
    RadioStatus_t status;

    sx1280_ReadCommand( RADIO_GET_STATUS, ( uint8_t * )&stat, 1 );
    status.Value = stat;
    return( status );
}

uint16_t sx1280_GetFirmwareVersion( void )
{
    return ( ( ( sx1280_ReadRegister( REG_LR_FIRMWARE_VERSION_MSB ) ) << 8 ) | ( sx1280_ReadRegister( REG_LR_FIRMWARE_VERSION_MSB + 1 ) ) );
}

/*!
 * \brief Radio registers definition
 *
 */
typedef struct
{
    uint16_t      Addr;                             //!< The address of the register
    uint8_t       Value;                            //!< The value of the register
}RadioRegisters_t;

/*!
 * \brief Radio hardware registers initialization definition
 */
#define RADIO_INIT_REGISTERS_VALUE  { }

/*!
 * \brief Radio hardware registers initialization
 */
const RadioRegisters_t RadioRegsInit[] = RADIO_INIT_REGISTERS_VALUE;

void sx1280_SetRegistersDefault( void )
{
    for( int16_t i = 0; i < sizeof( RadioRegsInit ) / sizeof( RadioRegisters_t ); i++ )
    {
        sx1280_WriteRegister( RadioRegsInit[i].Addr, RadioRegsInit[i].Value );
    }
}

void testReadWriteRegister() {
	//check there is a device out there, writes a register and reads back
	uint8_t Regdata1, Regdata2;
	Regdata1 = sx1280_ReadRegister(0x0908);               //low byte of frequency setting
	sx1280_WriteRegister(0x0908, (Regdata1 + 1));
	Regdata2 = sx1280_ReadRegister(0x0908);               //read changed value back
	sx1280_WriteRegister(0x0908, Regdata1);             //restore register to original value
	// printk("data: %x -- %x\n", Regdata1, Regdata2);
	if (Regdata2 == (Regdata1 + 1))
	{
		LOG_INF("Device found");
	}
	else
	{
		LOG_ERR("No device found");
	}
}

RadioPacketTypes_t sx1280_GetPacketType()
{
	RadioPacketTypes_t packetType = PACKET_TYPE_NONE;
	sx1280_ReadCommand( RADIO_GET_PACKETTYPE, ( uint8_t* )&packetType, 1 );
	return packetType;
}

void sx1280_SetPacketType( RadioPacketTypes_t packetType )
{
    // Save packet type internally to avoid questioning the radio
//     this->PacketType = packetType; // TODO

    sx1280_WriteCommand( RADIO_SET_PACKETTYPE, ( uint8_t* )&packetType, 1 );
}

void testReadWriteCommand() {
	RadioPacketTypes_t packetType1, packetType2, packetType3;
	packetType1 = sx1280_GetPacketType();
	sx1280_SetPacketType(PACKET_TYPE_LORA);
	packetType2 = sx1280_GetPacketType();
	sx1280_SetPacketType(packetType1);
	packetType3 = sx1280_GetPacketType();
	// printk("packetTypes: %x -- %x -- %x\n", packetType1, packetType2, packetType3);
	if (packetType2 == PACKET_TYPE_LORA && packetType1 == packetType3)
	{
		// printk("true\n");
	}
	else
	{
		// printk("false\n");
	}
}

static int sx1280_lora_init(const struct device *dev)
{
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	// printk("test init cs\n");
	static struct spi_cs_control spi_cs;
#endif
	int ret;
	// uint8_t regval;
	// RadioStatus_t status;

	dev_data.spi = device_get_binding(DT_INST_BUS_LABEL(0));
	if (!dev_data.spi) {
		LOG_ERR("Cannot get pointer to %s device",
			DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}
	// printk("test init 2\n");

	dev_data.spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
	dev_data.spi_cfg.frequency = DT_INST_PROP(0, spi_max_frequency);
	dev_data.spi_cfg.slave = DT_INST_REG_ADDR(0);

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	spi_cs.gpio_dev = device_get_binding(DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	if (!spi_cs.gpio_dev) {
		LOG_ERR("Cannot get pointer to %s device",
			DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
		return -EIO;
	}

	spi_cs.gpio_pin = GPIO_CS_PIN;
	spi_cs.gpio_dt_flags = GPIO_CS_FLAGS;
	spi_cs.delay = 0U;

	dev_data.spi_cfg.cs = &spi_cs;
	// printk("test init 3\n");
#endif

// 	ret = sx12xx_configure_pin(tcxo_power, GPIO_OUTPUT_INACTIVE);
// 	if (ret) {
// 		return ret;
// 	}

	/* Setup Reset gpio and perform soft reset */
	ret = sx12xx_configure_pin(reset, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}
	// printk("test init 4\n");
	// printk("test init 4.0\n");

	k_sleep(K_MSEC(50));
	// printk("test init 4.1\n");
	gpio_pin_set(dev_data.reset, GPIO_RESET_PIN, 0);
	// printk("test init 4.2\n");
	k_sleep(K_MSEC(20));
	// printk("test init 5\n");

	// regval = sx1280_GetFirmwareVersion();
	// if (ret < 0) {
	// 	LOG_ERR("Unable to read version info");
	// 	return -EIO;
	// }

	// // printk("SX1280 version 0x%02x found", regval);

	// status = sx1280_GetStatus();
	// // printk("status: %x", status.Value);

	sx1280_SetRegistersDefault();

	// // printk("regval-pre: %x", sx1280_ReadRegister( REG_MANUAL_GAIN_VALUE ));
	// sx1280_WriteRegister( REG_MANUAL_GAIN_VALUE, 13 );
	// // printk("regval-post: %x", sx1280_ReadRegister( REG_MANUAL_GAIN_VALUE ));

	testReadWriteRegister();
	testReadWriteCommand();


	sx1280_IoIrqInit();
	// CS = 0
	// WriteRegister
	// sx1280_ReadRegister
	// cs = 1


// 	ret = sx127x_antenna_configure();
// 	if (ret < 0) {
// 		LOG_ERR("Unable to configure antenna");
// 		return -EIO;
// 	}

// 	ret = sx12xx_init(dev);
// 	if (ret < 0) {
// 		LOG_ERR("Failed to initialize SX12xx common");
// 		return ret;
// 	}

	return 0;
}


void sx1280_SetLNAGainSetting( const RadioLnaSettings_t lnaSetting )
{
    switch(lnaSetting)
    {
        case LNA_HIGH_SENSITIVITY_MODE:
        {
            sx1280_WriteRegister( REG_LNA_REGIME, sx1280_ReadRegister( REG_LNA_REGIME ) | MASK_LNA_REGIME );
            break;
        }
        case LNA_LOW_POWER_MODE:
        {
            sx1280_WriteRegister( REG_LNA_REGIME, sx1280_ReadRegister( REG_LNA_REGIME ) & ~MASK_LNA_REGIME );
            break;
        }
    }
}

void sx1280_SetRfFrequency( uint32_t rfFrequency )
{
    uint8_t buf[3];
    uint32_t freq = 0;

    freq = ( uint32_t )( ( double )rfFrequency / ( double )FREQ_STEP );
    buf[0] = ( uint8_t )( ( freq >> 16 ) & 0xFF );
    buf[1] = ( uint8_t )( ( freq >> 8 ) & 0xFF );
    buf[2] = ( uint8_t )( freq & 0xFF );
    sx1280_WriteCommand( RADIO_SET_RFFREQUENCY, buf, 3 );
}

void sx1280_SetBufferBaseAddresses( uint8_t txBaseAddress, uint8_t rxBaseAddress )
{
    uint8_t buf[2];

    buf[0] = txBaseAddress;
    buf[1] = rxBaseAddress;
    sx1280_WriteCommand( RADIO_SET_BUFFERBASEADDRESS, buf, 2 );
}

void sx1280_SetModulationParams( ModulationParams_t *modParams )
{
    uint8_t buf[3];

    // Check if required configuration corresponds to the stored packet type
    // If not, silently update radio packet type
//     if( this->PacketType != modParams->PacketType ) // TODO
//     {
//         this->SetPacketType( modParams->PacketType );
//     }

    switch( modParams->PacketType )
    {
        case PACKET_TYPE_GFSK:
            buf[0] = modParams->Params.Gfsk.BitrateBandwidth;
            buf[1] = modParams->Params.Gfsk.ModulationIndex;
            buf[2] = modParams->Params.Gfsk.ModulationShaping;
            break;
        case PACKET_TYPE_LORA:
        case PACKET_TYPE_RANGING:
            buf[0] = modParams->Params.LoRa.SpreadingFactor;
            buf[1] = modParams->Params.LoRa.Bandwidth;
            buf[2] = modParams->Params.LoRa.CodingRate;
        //     this->LoRaBandwidth = modParams->Params.LoRa.Bandwidth; // TODO
            break;
        case PACKET_TYPE_FLRC:
            buf[0] = modParams->Params.Flrc.BitrateBandwidth;
            buf[1] = modParams->Params.Flrc.CodingRate;
            buf[2] = modParams->Params.Flrc.ModulationShaping;
            break;
        case PACKET_TYPE_BLE:
            buf[0] = modParams->Params.Ble.BitrateBandwidth;
            buf[1] = modParams->Params.Ble.ModulationIndex;
            buf[2] = modParams->Params.Ble.ModulationShaping;
            break;
        case PACKET_TYPE_NONE:
            buf[0] = 0;
            buf[1] = 0;
            buf[2] = 0;
            break;
    }
    sx1280_WriteCommand( RADIO_SET_MODULATIONPARAMS, buf, 3 );
}

void sx1280_SetPacketParams( PacketParams_t *packetParams )
{
    uint8_t buf[7];
    // Check if required configuration corresponds to the stored packet type
    // If not, silently update radio packet type
//     if( this->PacketType != packetParams->PacketType ) // TODO
//     {
//         this->SetPacketType( packetParams->PacketType );
//     }

    switch( packetParams->PacketType )
    {
        case PACKET_TYPE_GFSK:
            buf[0] = packetParams->Params.Gfsk.PreambleLength;
            buf[1] = packetParams->Params.Gfsk.SyncWordLength;
            buf[2] = packetParams->Params.Gfsk.SyncWordMatch;
            buf[3] = packetParams->Params.Gfsk.HeaderType;
            buf[4] = packetParams->Params.Gfsk.PayloadLength;
            buf[5] = packetParams->Params.Gfsk.CrcLength;
            buf[6] = packetParams->Params.Gfsk.Whitening;
            break;
        case PACKET_TYPE_LORA:
        case PACKET_TYPE_RANGING:
            buf[0] = packetParams->Params.LoRa.PreambleLength;
            buf[1] = packetParams->Params.LoRa.HeaderType;
            buf[2] = packetParams->Params.LoRa.PayloadLength;
            buf[3] = packetParams->Params.LoRa.Crc;
            buf[4] = packetParams->Params.LoRa.InvertIQ;
            buf[5] = 0;
            buf[6] = 0;
            break;
        case PACKET_TYPE_FLRC:
            buf[0] = packetParams->Params.Flrc.PreambleLength;
            buf[1] = packetParams->Params.Flrc.SyncWordLength;
            buf[2] = packetParams->Params.Flrc.SyncWordMatch;
            buf[3] = packetParams->Params.Flrc.HeaderType;
            buf[4] = packetParams->Params.Flrc.PayloadLength;
            buf[5] = packetParams->Params.Flrc.CrcLength;
            buf[6] = packetParams->Params.Flrc.Whitening;
            break;
        case PACKET_TYPE_BLE:
            buf[0] = packetParams->Params.Ble.ConnectionState;
            buf[1] = packetParams->Params.Ble.CrcLength;
            buf[2] = packetParams->Params.Ble.BleTestPayload;
            buf[3] = packetParams->Params.Ble.Whitening;
            buf[4] = 0;
            buf[5] = 0;
            buf[6] = 0;
            break;
        case PACKET_TYPE_NONE:
            buf[0] = 0;
            buf[1] = 0;
            buf[2] = 0;
            buf[3] = 0;
            buf[4] = 0;
            buf[5] = 0;
            buf[6] = 0;
            break;
    }
    sx1280_WriteCommand( RADIO_SET_PACKETPARAMS, buf, 7 );
}

void sx1280_SetTxParams( int8_t power, RadioRampTimes_t rampTime )
{
    uint8_t buf[2];

    // The power value to send on SPI/UART is in the range [0..31] and the
    // physical output power is in the range [-18..13]dBm
    buf[0] = power + 18;
    buf[1] = ( uint8_t )rampTime;
    sx1280_WriteCommand( RADIO_SET_TXPARAMS, buf, 2 );
}

#define LTUNUSED(v) (void) (v)    //add LTUNUSED(variable); to avoid compiler warnings

uint32_t sx1280_getFreqInt()
{
  //get the current set device frequency, return as long integer
  uint8_t Msb = 0;
  uint8_t Mid = 0;
  uint8_t Lsb = 0;

  uint32_t uinttemp;
  float floattemp;

  LTUNUSED(Msb);           //to prevent a compiler warning
  LTUNUSED(Mid);           //to prevent a compiler warning
  LTUNUSED(Lsb);           //to prevent a compiler warning

//   if (savedPacketType == PACKET_TYPE_LORA)
//   {
  Msb = sx1280_ReadRegister(0x906);
  Mid = sx1280_ReadRegister(0x907);
  Lsb = sx1280_ReadRegister(0x908);
//   }

//   if (savedPacketType == PACKET_TYPE_FLRC)
//   {
//   Msb = readRegister(REG_FLRC_RFFrequency23_16);
//   Mid = readRegister(REG_FLRC_RFFrequency15_8);
//   Lsb = readRegister(REG_FLRC_RFFrequency7_0);
//   }

  floattemp = ((Msb * 0x10000ul) + (Mid * 0x100ul) + Lsb);
  floattemp = ((floattemp * FREQ_STEP) / 1000000ul);
  uinttemp = (uint32_t)(floattemp * 1000000);
  return uinttemp;
}


void sx1280_printRegisters(uint16_t Start, uint16_t End)
{
  //prints the contents of SX1280 registers to serial monitor

  uint16_t Loopv1, Loopv2, RegData;

  // printk("Reg    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

  for (Loopv1 = Start; Loopv1 <= End;)           //32 lines
  {
    // printk("0x%X  ", Loopv1);
    for (Loopv2 = 0; Loopv2 <= 15; Loopv2++)
    {
      RegData = sx1280_ReadRegister(Loopv1);
      if (RegData < 0x10)
      {
        // printk("0");
      }
      // printk("%X ", RegData);                //print the register number
      Loopv1++;
    }
    // printk("\n");
  }
}

int sx1280_lora_config(const struct device *dev,
		       struct lora_modem_config *config)
{
	// printk("config1\n");
	sx1280_SetStandby(STDBY_RC);
	// printk("config2\n");
	sx1280_SetRegulatorMode(USE_LDO);
	// printk("regval-pre: %x\n", sx1280_ReadRegister( REG_LNA_REGIME ));
        sx1280_SetLNAGainSetting(LNA_HIGH_SENSITIVITY_MODE);
	// printk("regval-post: %x\n", sx1280_ReadRegister( REG_LNA_REGIME ));
	// printk("config3\n");

	PacketParams_t PacketParams;
	ModulationParams_t ModulationParams;

        ModulationParams.PacketType = PACKET_TYPE_LORA;
        PacketParams.PacketType     = PACKET_TYPE_LORA;

        ModulationParams.Params.LoRa.SpreadingFactor = ( RadioLoRaSpreadingFactors_t )  config->datarate;
        ModulationParams.Params.LoRa.Bandwidth       = ( RadioLoRaBandwidths_t )        config->bandwidth;
        ModulationParams.Params.LoRa.CodingRate      = ( RadioLoRaCodingRates_t )       config->coding_rate;
        PacketParams.Params.LoRa.PreambleLength      =                                  config->preamble_len;
        PacketParams.Params.LoRa.HeaderType          = LORA_PACKET_VARIABLE_LENGTH;
        PacketParams.Params.LoRa.PayloadLength       = 255;
        PacketParams.Params.LoRa.Crc                 = LORA_CRC_ON;
        PacketParams.Params.LoRa.InvertIQ            = LORA_IQ_NORMAL;

        sx1280_SetStandby( STDBY_RC );
	// printk("config4\n");
        sx1280_SetPacketType( ModulationParams.PacketType );
        sx1280_SetRfFrequency( config->frequency );
        sx1280_SetBufferBaseAddresses( 0x00, 0x00 );
        sx1280_SetModulationParams( &ModulationParams );
        sx1280_SetPacketParams( &PacketParams );
	sx1280_SetDioIrqParams(IRQ_RADIO_ALL, (IRQ_TX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);

	// printk("frequency: %u\n", sx1280_getFreqInt());
        // only used in GFSK, FLRC (4 bytes max) and BLE mode
        // SetSyncWord( 1, ( uint8_t[] ){ 0xDD, 0xA0, 0x96, 0x69, 0xDD } ); // TODO: non LORA
        // only used in GFSK, FLRC
        // uint8_t crcSeedLocal[2] = {0x45, 0x67}; // TODO
        // SetCrcSeed( crcSeedLocal );
        // SetCrcPolynomial( 0x0123 );
        sx1280_SetTxParams( config->tx_power, RADIO_RAMP_02_US );
	// sx1280_printRegisters(0x900, 0x9FF);
        // SetPollingMode( );

	// Radio.SetChannel(config->frequency);

	// if (config->tx) {
	// 	Radio.SetTxConfig(MODEM_LORA, config->tx_power, 0,
	// 			  config->bandwidth, config->datarate,
	// 			  config->coding_rate, config->preamble_len,
	// 			  false, true, 0, 0, false, 4000);
	// } else {
	// 	/* TODO: Get symbol timeout value from config parameters */
	// 	Radio.SetRxConfig(MODEM_LORA, config->bandwidth,
	// 			  config->datarate, config->coding_rate,
	// 			  0, config->preamble_len, 10, false, 0,
	// 			  false, 0, 0, false, true);
	// }

	return 0;
}


void sx1280_GetRxBufferStatus( uint8_t *rxPayloadLength, uint8_t *rxStartBufferPointer )
{
    uint8_t status[2];

    sx1280_ReadCommand( RADIO_GET_RXBUFFERSTATUS, status, 2 );

    // In case of LORA fixed header, the rxPayloadLength is obtained by reading
    // the register REG_LR_PAYLOADLENGTH
    if( ( sx1280_GetPacketType() == PACKET_TYPE_LORA ) && ( sx1280_ReadRegister( REG_LR_PACKETPARAMS ) >> 7 == 1 ) )
    {
        *rxPayloadLength = sx1280_ReadRegister( REG_LR_PAYLOADLENGTH );
    }
    else if( sx1280_GetPacketType() == PACKET_TYPE_BLE )
    {
        // In the case of BLE, the size returned in status[0] do not include the 2-byte length PDU header
        // so it is added there
        *rxPayloadLength = status[0] + 2;
    }
    else
    {
        *rxPayloadLength = status[0];
    }

    *rxStartBufferPointer = status[1];
}


void sx1280_setRx(TickTime_t timeout)
{
	mode_tx = false;
	sx1280_ClearIrqStatus( IRQ_RADIO_ALL );

	uint8_t buf[3];
	buf[0] = timeout.PeriodBase;
	buf[1] = ( uint8_t )( ( timeout.PeriodBaseCount >> 8 ) & 0x00FF );
	buf[2] = ( uint8_t )( timeout.PeriodBaseCount & 0x00FF );
	sx1280_WriteCommand( RADIO_SET_RX, buf, 3 );
}

uint8_t sx1280_readPacketSNR()
{
	uint8_t packetSNR;
	uint8_t status[5];

	sx1280_ReadCommand(RADIO_GET_PACKETSTATUS, status, 5) ;

	if ( status[1] < 128 )
	{
		packetSNR = status[1] / 4 ;
	}
	else
	{
		packetSNR = (( status[1] - 256 ) / 4);
	}

	return packetSNR;
}

int8_t sx1280_GetRssiInst( void )
{
    uint8_t raw = 0;

    sx1280_ReadCommand( RADIO_GET_RSSIINST, &raw, 1 );

    return ( int8_t ) ( -raw / 2 );
}

int sx1280_lora_recv(const struct device *dev, uint8_t *data, uint8_t size,
		     k_timeout_t timeout, int16_t *rssi, int8_t *snr)
{
	uint16_t irqStatus;
	uint8_t buffer[2];
	uint8_t _RXPacketL;
	uint8_t RXstart, RXend;
	int ret;

	sx1280_SetDioIrqParams(IRQ_RADIO_ALL, (IRQ_RX_DONE + IRQ_RX_TX_TIMEOUT), 0, 0);
	sx1280_setRx(RX_TX_CONTINUOUS);

	ret = k_sem_take(&recv_sem, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("Receive timeout!");
		return ret;
	}

	sx1280_SetStandby(MODE_STDBY_RC);                                                   //ensure to stop further packet reception

	irqStatus = sx1280_readIrqStatus();

	if ( (irqStatus & IRQ_HEADER_ERROR) | (irqStatus & IRQ_CRC_ERROR) | (irqStatus & IRQ_RX_TX_TIMEOUT ) ) //check if any of the preceding IRQs is set
	{
		LOG_ERR("rx error");
		return -1;
	}

	sx1280_ReadCommand(RADIO_GET_RXBUFFERSTATUS, buffer, 2);
	_RXPacketL = buffer[0];

	if (_RXPacketL > size)               //check passed buffer is big enough for packet
	{
	_RXPacketL = size;                 //truncate packet if not enough space
	}

	RXstart = buffer[1];

	RXend = RXstart + _RXPacketL;


	if (rssi != NULL) {
		*rssi = sx1280_GetRssiInst();
	}

	if (snr != NULL) {
		*snr = sx1280_readPacketSNR();
	}

	// checkBusy();

	// digitalWrite(_NSS, LOW);             //start the burst read
	// SPI.transfer(RADIO_READ_BUFFER);
	// SPI.transfer(RXstart);
	// SPI.transfer(0xFF);

	// for (index = RXstart; index < RXend; index++)
	// {
	// regdata = SPI.transfer(0);
	// rxbuffer[index] = regdata;
	// }

	// digitalWrite(_NSS, HIGH);




	// sx1280_GetRxBufferStatus( &size, &offset );
//     if( size > maxSize )
//     {
//         return 1;
//     }
	sx1280_ReadBuffer( RXstart, data, size );
	return _RXPacketL;
}
	// int ret;

	// Radio.SetMaxPayloadLength(MODEM_LORA, 255);
	// Radio.Rx(0);

	// ret = k_sem_take(&dev_data.data_sem, timeout);
	// if (ret < 0) {
	// 	LOG_ERR("Receive timeout!");
	// 	return ret;
	// }

	// /* Only copy the bytes that can fit the buffer, drop the rest */
	// if (dev_data.rx_len > size)
	// 	dev_data.rx_len = size;

	// /*
	//  * FIXME: We are copying the global buffer here, so it might get
	//  * overwritten inbetween when a new packet comes in. Use some
	//  * wise method to fix this!
	//  */
	// memcpy(data, dev_data.rx_buf, dev_data.rx_len);

	// if (rssi != NULL) {
	// 	*rssi = dev_data.rssi;
	// }

	// if (snr != NULL) {
	// 	*snr = dev_data.snr;
	// }

	// return dev_data.rx_len;
// }

static const struct lora_driver_api sx1280_lora_api = {
	.config = sx1280_lora_config,
	.send = sx1280_lora_send,
	.recv = sx1280_lora_recv,
	.test_cw = sx1280_lora_test_cw,
};

DEVICE_DT_INST_DEFINE(0, &sx1280_lora_init, NULL, NULL,
		      NULL, POST_KERNEL, CONFIG_LORA_INIT_PRIORITY,
		      &sx1280_lora_api);
