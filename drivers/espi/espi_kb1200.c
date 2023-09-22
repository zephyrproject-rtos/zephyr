/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_espi

#include <zephyr/kernel.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/logging/log.h>
#include "espi_utils.h"

#define PC_FREE		    BIT(0u)
#define NP_FREE		    BIT(1u)
#define VWIRE_FREE		    BIT(2u)
#define OOB_FREE		    BIT(3u)

#define FLASH_C_FREE		BIT(8u)
#define FLASH_NP_FREE		BIT(9u)

LOG_MODULE_REGISTER(espi_kb1200);

enum {
	EACPI_WRITE_SCID             = EACPI_START_OPCODE,  // 0x60
	EACPI_WRITE_ECIODP,
	EACPI_SET_BURST,
	EACPI_GET_SCI_PENDING,
	EACPI_GET_OBF_FLAG,
};

struct espi_kb1200_config {

	ESPI_T *espi;                  /* ESPI Struct       */
	EC_T  *eci; 
};

struct espi_kb1200_data {
	sys_slist_t callbacks;
	uint8_t eci_buff[8];
	uint8_t eci_step;
};

#ifdef CONFIG_ESPI_PERIPHERAL_HOST_IO_PVT
static void ibf_pvt_isr(const struct device *dev)
{
	struct espi_kb1200_data *data = (struct espi_kb1200_data *)(dev->data);
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_HOST_IO_PVT,
		.evt_data = ESPI_PERIPHERAL_NODATA
	};

	espi_send_callbacks(&data->callbacks, dev, evt);
}
#endif

////////////////////////////////////

static int espi_kb1200_configure(const struct device *dev, struct espi_cfg *cfg)
{
	printk("%s espi_cfg = { io_caps:%d, channel_caps:%d, max_freq=%d}\n", __func__,
		cfg->io_caps, cfg->channel_caps, cfg->max_freq);

	const struct espi_kb1200_config *config = dev->config;
	ESPI_T *espi = config->espi;

	uint8_t io_caps = 0;
	uint8_t channel_caps = 0;
	uint8_t max_freq = 0;

	/* Set frequency */
	switch (cfg->max_freq) {
	case 20:
		max_freq = ESPI_FREQ_MAX_20M;
		break;
	case 25:
		max_freq = ESPI_FREQ_MAX_25M;
		break;
	case 33:
		max_freq = ESPI_FREQ_MAX_33M;
		break;
	case 50:
		max_freq = ESPI_FREQ_MAX_50M;
		break;
	case 66:
		max_freq = ESPI_FREQ_MAX_66M;
		break;
	default:
		return -EINVAL;
	}

	/* Set IO mode */
	switch ((int32_t)cfg->io_caps) {
	case ESPI_IO_MODE_SINGLE_LINE:
		io_caps = ESPI_IO_SINGLE;
		break;
	case ESPI_IO_MODE_SINGLE_LINE | ESPI_IO_MODE_DUAL_LINES:
		io_caps = ESPI_IO_SINGLE_DUAL;
		break;
	case ESPI_IO_MODE_SINGLE_LINE | ESPI_IO_MODE_QUAD_LINES:
		io_caps = ESPI_IO_SINGLE_QUAD;
		break;
	case ESPI_IO_MODE_SINGLE_LINE | ESPI_IO_MODE_DUAL_LINES | ESPI_IO_MODE_QUAD_LINES:
		io_caps = ESPI_IO_SINGLE_DUAL_QUAD;
		break;
	default:
		return -EINVAL;
	}

	/* Validdate and translate eSPI API channels to MEC capabilities */
	if (cfg->channel_caps & ESPI_CHANNEL_PERIPHERAL) {
		channel_caps |= ESPI_SUPPORT_ESPIPH;
	}

	if (cfg->channel_caps & ESPI_CHANNEL_VWIRE) {
		channel_caps |= ESPI_SUPPORT_ESPIVW;
	}

	if (cfg->channel_caps & ESPI_CHANNEL_OOB) {
		channel_caps |= ESPI_SUPPORT_ESPIOOB;
	}

	if (cfg->channel_caps & ESPI_CHANNEL_FLASH) {
		channel_caps |= ESPI_SUPPORT_ESPIFA;
	}

	espi->ESPIGENCFG = ((io_caps &0x03)<<24)
					|(ESPI_ALERT_OD<<19)
					|((max_freq&0x07)<<16)
					|((channel_caps&0x0F)<<0);

	return 0;
}

static bool espi_kb1200_channel_ready(const struct device *dev,
				   enum espi_channel ch)
{
	const struct espi_kb1200_config *config = dev->config;
	ESPI_T *espi = config->espi;
	bool sts;

	LOG_INF("espi_kb1200_channel_ready");

	switch (ch) {
	case ESPI_CHANNEL_PERIPHERAL:
		sts = espi->ESPISTA & PC_FREE;
		break;
	case ESPI_CHANNEL_VWIRE:
		sts = espi->ESPISTA & NP_FREE;
		break;
	case ESPI_CHANNEL_OOB:
		sts = espi->ESPISTA & OOB_FREE;
		break;
	case ESPI_CHANNEL_FLASH:
		sts = espi->ESPISTA & FLASH_NP_FREE;
		break;
	default:
		sts = false;
		break;
	}

	return sts;
}


static int espi_kb1200_send_vwire(const struct device *dev,
			       enum espi_vwire_signal signal, uint8_t level)
{
	return -EINVAL;
}

static int espi_kb1200_receive_vwire(const struct device *dev,
				  enum espi_vwire_signal signal, uint8_t *level)
{
	return -EINVAL;
}

static int espi_kb1200_manage_callback(const struct device *dev,
					  struct espi_callback *callback, bool set)
{
	struct espi_kb1200_data *data = (struct espi_kb1200_data *)(dev->data);
	LOG_INF("espi_kb1200_manage_callback");
	return espi_manage_callback(&data->callbacks, callback, set);
}

static int espi_kb1200_read_lpc_request(const struct device *dev,
				     enum lpc_peripheral_opcode op,
				     uint32_t  *data)
{
#ifdef CONFIG_ESPI_ECI_PERIPHERAL_NOTIFICATION
	const struct espi_kb1200_config *config = dev->config;
	EC_T* eci = (EC_T *)config->eci;

	//LOG_INF("espi_kb1200_write_lpc_request");

	if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {

		switch (op) {
		case EACPI_GET_SCI_PENDING:
			*data = (eci->ECISTS & 0x20);  // SCI Pending Flag.
			break;
		case EACPI_GET_OBF_FLAG:
			*data = (eci->ECISTS & 0x01);  // OBF Flag.
		default:
			return -EINVAL;
		}
	}
#endif// CONFIG_ESPI_ECI_PERIPHERAL_NOTIFICATION

//	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
//		/* Make sure kbc 8042 is on */
//		if (!(KBC_REGS->ECICFG & ECI_enable)) {
//
//		LOG_INF("106x_kbc....exit");
//
//		
//			return -ENOTSUP;
//		}
//#if 1
//		switch (op) {
//		case E8042_OBF_HAS_CHAR:
//			/* EC has written data back to host. OBF is
//			 * automatically cleared after host reads
//			 * the data
//			 */
//			*data = KBC_REGS->ECISTS & ECI_OBF ? 1 : 0;
//		LOG_INF("E8042_OBF_HAS_CHAR");
//
//		
//			break;
//		case E8042_IBF_HAS_CHAR:
//			*data = KBC_REGS->ECISTS & ECI_IBF ? 1 : 0;
//			LOG_INF("E8042_IBF_HAS_CHAR");
//			
//			break;
//		case E8042_READ_KB_STS:
//			*data = KBC_REGS->ECISTS;
//			LOG_INF("E8042_READ_KB_STS");
//			
//			break;
//		default:
//			return -EINVAL;
//		}
//#endif
//
//		
//	} else {
//		return -ENOTSUP;
//	}
	return 0;
}

static int espi_kb1200_write_lpc_request(const struct device *dev,
				      enum lpc_peripheral_opcode op,
				      uint32_t *data)
{
#ifdef CONFIG_ESPI_ECI_PERIPHERAL_NOTIFICATION
	const struct espi_kb1200_config *config = dev->config;
	EC_T* eci = (EC_T *)config->eci;

	//LOG_INF("espi_kb1200_write_lpc_request");

	if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {

		switch (op) {
		case EACPI_WRITE_ECIODP:
			//printk("WrODP %02X\n", (uint8_t)*data);
			if (eci->ECISTS & 0x01)  {  // OBF
				return -1;
			}
			eci->ECIODP = (uint8_t)*data;
			break;
		case EACPI_WRITE_SCID:
			//printk("WrSCID %02X\n", (uint8_t)*data);
			if (eci->ECISTS & 0x20)  {  // SCI Pending Flag.
				return -1;
			}
			eci->ECISCID = (uint8_t)*data;
			break;
		case EACPI_SET_BURST:
			if (*data == 1) {  // Burst mode
				eci->ECISTS = 0x08;
			}
			else {
				eci->ECISTS = 0x00;
			}
			break;
		default:
			return -EINVAL;
		}
	}
#endif// CONFIG_ESPI_ECI_PERIPHERAL_NOTIFICATION

//	 if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
//		 /* Make sure kbc 8042 is on */
//		 if (!(KBC_REGS->ECICFG & ECI_enable)) {
//			 return -ENOTSUP;
//		 }
//
//		 switch (op) {
//		 case E8042_WRITE_KB_CHAR:
//			 KBC_REGS->ECIODP = *data & 0xff;
//			 break;
//		 case E8042_WRITE_MB_CHAR:
//			 KBC_REGS->ECIODP = *data & 0xff;
//			 break;
//		 case E8042_RESUME_IRQ:
////			 MCHP_GIRQ_SRC(config->pc_girq_id) = MCHP_KBC_IBF_GIRQ;
////			 MCHP_GIRQ_ENSET(config->pc_girq_id) = MCHP_KBC_IBF_GIRQ;
//			 break;
//		 case E8042_PAUSE_IRQ:
////			 MCHP_GIRQ_ENCLR(config->pc_girq_id) = MCHP_KBC_IBF_GIRQ;
//			 break;
//		 case E8042_CLEAR_OBF:
//			 dummy = KBC_REGS->ECISTS &= (0x10|0x02);
//			 break;
//		 case E8042_SET_FLAG:
//			 /* FW shouldn't modify these flags directly */
//
////			 KBC_REGS->ECISTS |= (0x10|0x20);
//			 break;
//		 case E8042_CLEAR_FLAG:
//			 /* FW shouldn't modify these flags directly */
//		 	KBC_REGS->ECISTS |= (0x10|0x20);
//
//			 break;
//		 default:
//			 return -EINVAL;
//		 }
//	 } else {
//		 return -ENOTSUP;
//
//	 }
//
	 return 0;
}

#ifndef CONFIG_ESPI_ECI_PERIPHERAL_NOTIFICATION
static unsigned char EC_RAM[255];
#endif// CONFIG_ESPI_ECI_PERIPHERAL_NOTIFICATION
static void EC_62_66_isr(const struct device *dev)
{
	const struct espi_kb1200_config *config = dev->config;
	struct espi_kb1200_data *data = dev->data;
	EC_T* eci = (EC_T *)config->eci;

	uint8_t ecipf = eci->ECIPF;
	if (ecipf & 0x01) { // OBF
		if (eci->ECISTS & 0x01) { // OBF;
			eci->ECISTS = (eci->ECISTS & 0x10) | 0x01;  // Clear OBF;
		}
		//printk("OBF issue\n");
		eci->ECIPF = 0x01;
	}
	if (ecipf & 0x02) { // IBF
		if (eci->ECISTS & 0x02) { // IBF;
#ifdef CONFIG_ESPI_ECI_PERIPHERAL_NOTIFICATION
			if (eci->ECISTS & 0x08)  { // Command port ready
				uint8_t cmd = eci->ECICMD;
				//printk("Cmd = %02X\n", cmd);
				struct espi_event evt = { .evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
					.evt_details = (1 << 16) | ESPI_PERIPHERAL_HOST_IO,   // (peripheral_index << 16) | peripheral_type 
					.evt_data = cmd };
				espi_send_callbacks(&data->callbacks, dev, evt);
			}
			else {  // Data port ready
				uint8_t dat = eci->ECIIDP;
				//printk("Data = %02X\n", dat);
				struct espi_event evt = { .evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
					.evt_details = (0 << 16) | ESPI_PERIPHERAL_HOST_IO,   // (peripheral_index << 16) | peripheral_type 
					.evt_data = dat };
				espi_send_callbacks(&data->callbacks, dev, evt);
			}
#else
			if (eci->ECISTS & 0x08)  { // Command port ready
				uint8_t cmd = eci->ECICMD;
				//printk("Command = %02X\n", cmd);
				data->eci_buff[0] = cmd;
				data->eci_step = 1;
				if (cmd == EC_Burst_Enable_CMD)  {
					eci->ECIODP = 0x90;  // Burst Ack
					eci->ECISTS = 0x10;  // Burst mode enable;
				}
				if (cmd == EC_Burst_Disable_CMD)  {
					eci->ECISTS = 0x00;  // Burst mode disable;
				}
			}
			else {  // Data port ready
				uint8_t dat = eci->ECIIDP;
				//printk("Data[%d] = %02X\n", data->eci_step, dat);
				if (data->eci_step == 1) {
					data->eci_buff[1] = dat;
					data->eci_step = 2;
					if (data->eci_buff[0] == EC_Read_CMD)  {
						eci->ECIODP = EC_RAM[data->eci_buff[1]];
					}
				}
				else if (data->eci_step == 2) {
					if (data->eci_buff[0] == EC_Write_CMD)  {
						EC_RAM[data->eci_buff[1]] = dat;
					}
				}
			}
#endif // CONFIG_ESPI_ECI_PERIPHERAL_NOTIFICATION
			eci->ECISTS = (eci->ECISTS & 0x10) | 0x02;  // Clear IBF;
		}
		eci->ECIPF = 0x02;
	}
	if (ecipf & 0x04) {
		eci->ECIPF = 0x04;
	}
}

#define POSTCODE_PORT80    0
#define POSTCODE_PORT81    1

static void espi_kb1200_dbi_isr(const struct device *dev)
{
	struct espi_kb1200_data *data = (struct espi_kb1200_data *)(dev->data);
	DBI_T *dbi0 = ((DBI_T *) DBI0_BASE);             /* DBI Struct       */
	DBI_T *dbi1 = ((DBI_T *) DBI1_BASE);             /* DBI Struct       */

	//printk("%s \n", __func__);
	if (dbi0->DBIPF & 0x01) {
		dbi0->DBIPF = 0x01;      // mClr_DBI0_Event
		uint8_t postcode = dbi0->DBIIDP;
		struct espi_event evt = { .evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
				  .evt_details = (POSTCODE_PORT80 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,   // (peripheral_index << 16) | peripheral_type 
				  .evt_data = postcode };
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
	if (dbi1->DBIPF & 0x01) {
		dbi1->DBIPF = 0x01;      // mClr_DBI1_Event
		uint8_t postcode = dbi1->DBIIDP;
		struct espi_event evt = { .evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
				  .evt_details = (POSTCODE_PORT81 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,   // (peripheral_index << 16) | peripheral_type 
				  .evt_data = postcode };
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}


static int espi_kb1200_init(const struct device *dev)
{
	const struct espi_kb1200_config *config = dev->config;
	ESPI_T *espi = config->espi;

	printk("@%s\n", __func__);

	espi->ESPIGENCFG = ((ESPI_IO_MODE &0x03)<<24)
						|(ESPI_ALERT_OD<<19)
						|((ESPI_FREQ_MAX&0x07)<<16)
						|((
						(1<<0)	  // SUPPORT_ESPIPH
						)&0x0F);

//	ESPI_Init();


#if 1
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, eci, irq),
		    DT_INST_IRQ_BY_NAME(0, eci, priority),
		    EC_62_66_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, eci, irq));
	EC_T *eci = config->eci;
	eci->ECICFG = (0x0662 << 16) | (1 << 8) | 1;
	eci->ECIIE  = 0x03;    // IBF 
	eci->ECIPF  |= 0x03;
	eci->ECISTS |= 0x23;
#endif

#if 1

//	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, vwire, irq),
//		    DT_INST_IRQ_BY_NAME(0, vwire, priority),
//		    Handle_ESPIVW_ISR,
//		    DEVICE_DT_INST_GET(0), 0);
//	irq_enable(DT_INST_IRQ_BY_NAME(0, vwire, irq));

#endif

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, dbi, irq),
		    DT_INST_IRQ_BY_NAME(0, dbi, priority),
		    espi_kb1200_dbi_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, dbi, irq));
	DBI_T *dbi0 = ((DBI_T *) DBI0_BASE);             /* DBI Struct       */
	dbi0->DBIPF = 0x01;      // mClr_DBI0_Event
	dbi0->DBICFG = 0x00800001;
	dbi0->DBIIE = 0x01;
	DBI_T *dbi1 = ((DBI_T *) DBI1_BASE);             /* DBI Struct       */
	dbi1->DBIPF = 0x01;      // mClr_DBI1_Event
	dbi1->DBICFG = 0x00810001;
	dbi1->DBIIE = 0x01;

	return 0;
}

static const struct espi_driver_api espi_kb1200_driver_api = {
	.config = espi_kb1200_configure,
	.get_channel_status = espi_kb1200_channel_ready,
	.send_vwire = espi_kb1200_send_vwire,
	.receive_vwire = espi_kb1200_receive_vwire,
#ifdef CONFIG_ESPI_OOB_CHANNEL
	.send_oob = espi_kb1200_send_oob,
	.receive_oob = espi_kb1200_receive_oob,
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	.flash_read = espi_kb1200_flash_read,
	.flash_write = espi_kb1200_flash_write,
	.flash_erase = espi_kb1200_flash_erase,
#endif
	.manage_callback = espi_kb1200_manage_callback,
	.read_lpc_request = espi_kb1200_read_lpc_request,
	.write_lpc_request = espi_kb1200_write_lpc_request,
};

static struct espi_kb1200_data espi_kb1200_data;

static const struct espi_kb1200_config espi_kb1200_config = {

	.espi = ((ESPI_T *) ESPI_BASE),                      /* ESPI Struct       */
	.eci = ((EC_T *) ECI_BASE),                      /* ECI_BASE       */
};


DEVICE_DT_INST_DEFINE(0, &espi_kb1200_init, NULL,
		    &espi_kb1200_data, &espi_kb1200_config,
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &espi_kb1200_driver_api);
