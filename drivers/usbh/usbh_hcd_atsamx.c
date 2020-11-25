/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is based on usbh_hcd_atsamx.c from uC/Modbus Stack.
 *
 *                                uC/Modbus
 *                         The Embedded USB Host Stack
 *
 *      Copyright 2003-2020 Silicon Laboratories Inc. www.silabs.com
 *
 *                   SPDX-License-Identifier: APACHE-2.0
 *
 * This software is subject to an open source license and is distributed by
 *  Silicon Laboratories Inc. pursuant to the terms of the Apache License,
 *      Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
 */

/*
 * Note(s)  : (1) Due to hardware limitations, the ATSAM D5x/E5x host controller
 *		does not support a
 *		combination of Full-Speed HUB + Low-Speed device
 */

#define DT_DRV_COMPAT atmel_sam0_usbh

#include <drivers/usbh/usbh_ll.h>
#include <usbh_hub.h>
#include <soc.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <sys/math_extras.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(sam0_usbh, CONFIG_USBH_LOG_LEVEL);

/* HOST Operation mode*/
#define USBH_ATSAMX_CTRLA_HOST_MODE BIT(7)
/* Run in standby mode*/
#define USBH_ATSAMX_CTRLA_RUNSTBY BIT(2)
/* Enable*/
#define USBH_ATSAMX_CTRLA_ENABLE BIT(1)
/* Software reset*/
#define USBH_ATSAMX_CTRLA_SWRST BIT(0)
/* Synchronization enable status*/
#define USBH_ATSAMX_SYNCBUSY_ENABLE BIT(1)
/* Synchronization Software reset status*/
#define USBH_ATSAMX_SYNCBUSY_SWRST BIT(0)
#define USBH_ATSAMX_SYNCBUSY_MSK \
	(USBH_ATSAMX_SYNCBUSY_ENABLE | USBH_ATSAMX_SYNCBUSY_SWRST)

/* Data quality of service set to HIGH critical latency */
#define USBH_ATSAMX_QOSCTRL_DQOS_HIGH (3u << 2)
/* Cfg  quality of service set to HIGH critical latency */
#define USBH_ATSAMX_QOSCTRL_CQOS_HIGH (3u << 0)

#define USBH_ATSAMX_FSMSTATE_OFF 0x01u
#define USBH_ATSAMX_FSMSTATE_ON 0x02u
#define USBH_ATSAMX_FSMSTATE_SUSPEND 0x04u
#define USBH_ATSAMX_FSMSTATE_SLEEP 0x08u
#define USBH_ATSAMX_FSMSTATE_DNRESUME 0x10u
#define USBH_ATSAMX_FSMSTATE_UPRESUME 0x20u
#define USBH_ATSAMX_FSMSTATE_RESET 0x40u
/* Send USB L1 resume*/
#define USBH_ATSAMX_CTRLB_L1RESUME BIT(11)
/* VBUS is ok*/
#define USBH_ATSAMX_CTRLB_VBUSOK BIT(10)
/* Send USB reset*/
#define USBH_ATSAMX_CTRLB_BUSRESET BIT(9)
/* Start-of-Frame enable*/
#define USBH_ATSAMX_CTRLB_SOFE BIT(8)
/* Speed configuration for host*/
#define USBH_ATSAMX_CTRLB_SPDCONF_LSFS (0x0u << 2)
#define USBH_ATSAMX_CTRLB_SPDCONF_MSK (0x3u << 2)
/* Send USB resume*/
#define USBH_ATSAMX_CTRLB_RESUME BIT(1)
/* Frame length control enable*/
#define USBH_ATSAMX_HSOFC_FLENCE BIT(7)

#define USBH_ATSAMX_STATUS_SPEED_POS 2
#define USBH_ATSAMX_STATUS_SPEED_MSK 0xCu
#define USBH_ATSAMX_STATUS_SPEED_FS 0x0u
#define USBH_ATSAMX_STATUS_SPEED_LS 0x1u
#define USBH_ATSAMX_STATUS_LINESTATE_POS 6u
#define USBH_ATSAMX_STATUS_LINESTATE_MSK 0xC0u

#define USBH_ATSAMX_FNUM_MSK 0x3FF8u
#define USBH_ATSAMX_FNUM_POS 3u
/* Device disconnection interrupt*/
#define USBH_ATSAMX_INT_DDISC BIT(9)
/* Device connection interrtup*/
#define USBH_ATSAMX_INT_DCONN BIT(8)
/* RAM acces interrupt*/
#define USBH_ATSAMX_INT_RAMACER BIT(7)
/* Upstream resume from the device interrupt*/
#define USBH_ATSAMX_INT_UPRSM BIT(6)
/* Downstream resume interrupt*/
#define USBH_ATSAMX_INT_DNRSM BIT(5)
/* Wake up interrupt*/
#define USBH_ATSAMX_INT_WAKEUP BIT(4)
/* Bus reset interrupt*/
#define USBH_ATSAMX_INT_RST BIT(3)
/* Host Start-of-Frame interrupt*/
#define USBH_ATSAMX_INT_HSOF BIT(2)
#define USBH_ATSAMX_INT_MSK				   \
	(USBH_ATSAMX_INT_DDISC | USBH_ATSAMX_INT_DCONN |   \
	 USBH_ATSAMX_INT_RAMACER | USBH_ATSAMX_INT_UPRSM | \
	 USBH_ATSAMX_INT_DNRSM | USBH_ATSAMX_INT_WAKEUP |  \
	 USBH_ATSAMX_INT_RST | USBH_ATSAMX_INT_HSOF)

#define USBH_ATSAMX_PCFG_PTYPE_MSK (0x7u << 3u)
/* Pipe is disabled*/
#define USBH_ATSAMX_PCFG_PTYPE_DISABLED (0x0u << 3u)
/* Pipe is enabled and configured as CONTROL*/
#define USBH_ATSAMX_PCFG_PTYPE_CONTROL (0x1u << 3u)
/* Pipe is enabled and configured as ISO*/
#define USBH_ATSAMX_PCFG_PTYPE_ISO (0x2u << 3u)
/* Pipe is enabled and configured as BULK*/
#define USBH_ATSAMX_PCFG_PTYPE_BULK (0x3u << 3u)
/* Pipe is enabled and configured as INTERRUPT*/
#define USBH_ATSAMX_PCFG_PTYPE_INTERRUPT (0x4u << 3u)
/* Pipe is enabled and configured as EXTENDED*/
#define USBH_ATSAMX_PCFG_PTYPE_EXTENDED (0x5u << 3u)
/* Pipe bank*/
#define USBH_ATSAMX_PCFG_BK BIT(2)
#define USBH_ATSAMX_PCFG_PTOKEN_SETUP 0x0u
#define USBH_ATSAMX_PCFG_PTOKEN_IN 0x1u
#define USBH_ATSAMX_PCFG_PTOKEN_OUT 0x2u
#define USBH_ATSAMX_PCFG_PTOKEN_MSK 0x3u
/* Bank 1 ready*/
#define USBH_ATSAMX_PSTATUS_BK1RDY BIT(7)
/* Bank 0 ready*/
#define USBH_ATSAMX_PSTATUS_BK0RDY BIT(6)
/* Pipe freeze*/
#define USBH_ATSAMX_PSTATUS_PFREEZE BIT(4)
/* Current bank*/
#define USBH_ATSAMX_PSTATUS_CURBK BIT(2)
/* Data toggle*/
#define USBH_ATSAMX_PSTATUS_DTGL BIT(0)
/* Pipe stall received interrupt*/
#define USBH_ATSAMX_PINT_STALL BIT(5)
/* Pipe transmitted setup interrupt*/
#define USBH_ATSAMX_PINT_TXSTP BIT(4)
/* Pipe error interrupt*/
#define USBH_ATSAMX_PINT_PERR BIT(3)
/* Pipe transfer fail interrupt*/
#define USBH_ATSAMX_PINT_TRFAIL BIT(2)
/* Pipe transfer complete x interrupt*/
#define USBH_ATSAMX_PINT_TRCPT BIT(0)
#define USBH_ATSAMX_PINT_ALL				   \
	(USBH_ATSAMX_PINT_STALL | USBH_ATSAMX_PINT_TXSTP | \
	 USBH_ATSAMX_PINT_PERR | USBH_ATSAMX_PINT_TRFAIL | \
	 USBH_ATSAMX_PINT_TRCPT)

#define USBH_ATSAMX_PDESC_BYTE_COUNT_MSK 0x3FFFu
#define USBH_ATSAMX_PDESC_PCKSIZE_BYTE_COUNT_MSK 0x0FFFFFFFu

#define USBH_ATSAMX_GET_BYTE_CNT(reg) \
	((reg & USBH_ATSAMX_PDESC_BYTE_COUNT_MSK) >> 0)
#define USBH_ATSAMX_GET_DPID(reg) ((reg & USBH_ATSAMX_PSTATUS_DTGL) >> 0)
#define USBH_ATSAMX_PCFG_PTYPE(value) ((value + 1) << 3u)

/*
 * ATSAMD5X/ATSAME5X DEFINES
 */
/* NVM software calibration area address*/
#define ATSAMX_NVM_SW_CAL_AREA ((volatile uint32_t *)0x00800080u)
#define ATSAMX_NVM_USB_TRANSN_POS 32u
#define ATSAMX_NVM_USB_TRANSP_POS 37u
#define ATSAMX_NVM_USB_TRIM_POS 42u

#define ATSAMX_NVM_USB_TRANSN_SIZE 5u
#define ATSAMX_NVM_USB_TRANSP_SIZE 5u
#define ATSAMX_NVM_USB_TRIM_SIZE 3u

#define ATSAMX_NVM_USB_TRANSN						  \
	((*(ATSAMX_NVM_SW_CAL_AREA + (ATSAMX_NVM_USB_TRANSN_POS / 32)) >> \
	  (ATSAMX_NVM_USB_TRANSN_POS % 32)) &				  \
	 ((1 << ATSAMX_NVM_USB_TRANSN_SIZE) - 1))
#define ATSAMX_NVM_USB_TRANSP						  \
	((*(ATSAMX_NVM_SW_CAL_AREA + (ATSAMX_NVM_USB_TRANSP_POS / 32)) >> \
	  (ATSAMX_NVM_USB_TRANSP_POS % 32)) &				  \
	 ((1 << ATSAMX_NVM_USB_TRANSP_SIZE) - 1))
#define ATSAMX_NVM_USB_TRIM						\
	((*(ATSAMX_NVM_SW_CAL_AREA + (ATSAMX_NVM_USB_TRIM_POS / 32)) >>	\
	  (ATSAMX_NVM_USB_TRIM_POS % 32)) &				\
	 ((1 << ATSAMX_NVM_USB_TRIM_SIZE) - 1))

#define ATSAMX_NVM_USB_TRANSN_MSK 0x1Fu
#define ATSAMX_NVM_USB_TRANSP_MSK 0x1Fu
#define ATSAMX_NVM_USB_TRIM_MSK 0x07u

#define ATSAMX_MAX_NBR_PIPE 8           /* Maximum number of host pipes*/
#define ATSAMX_INVALID_PIPE 0xFFu       /* Invalid pipe number.*/
#define ATSAMX_DFLT_EP_ADDR 0xFFFFu     /* Default endpoint address.*/

/* If necessary, re-define these values in 'usbh_cfg.h' */
#ifndef ATSAMX_URB_PROC_TASK_STK_SIZE
#define ATSAMX_URB_PROC_TASK_STK_SIZE 256u
#endif

#ifndef ATSAMX_URB_PROC_TASK_PRIO
#define ATSAMX_URB_PROC_TASK_PRIO 15u
#endif

#ifndef ATSAMX_URB_PROC_Q_MAX
#define ATSAMX_URB_PROC_Q_MAX 8
#endif

struct usbh_atsamx_desc_bank {
	/* Address of the data buffer*/
	volatile uint32_t addr;
	/* Packet size*/
	volatile uint32_t pck_size;
	/* Extended register*/
	volatile uint16_t ext_reg;
	/* Host status bank*/
	volatile uint8_t stat_bnk;
	volatile uint8_t rsvd_0;
	/* Host control pipe*/
	volatile uint16_t ctrl_pipe;
	/* Host Status pipe*/
	volatile uint16_t status_pipe;
};

struct usbh_atsamx_pipe_desc {
	struct usbh_atsamx_desc_bank desc_bank[2];
};

struct usbh_atsamx_pipe_reg {
	/* Host pipe configuration*/
	volatile uint8_t p_cfg;
	volatile uint8_t rsvd_0[2];
	/* interval for the Bulk-Out/Ping transaction*/
	volatile uint8_t b_interval;
	/* Pipe status clear*/
	volatile uint8_t p_status_clr;
	/* Pipe status set*/
	volatile uint8_t p_status_set;
	/* Pipe status register*/
	volatile uint8_t p_status;
	/* Host pipe interrupt flag register*/
	volatile uint8_t p_int_flag;
	/* Host pipe interrupt clear register*/
	volatile uint8_t p_int_clr;
	/* Host pipe interrupt set register*/
	volatile uint8_t p_int_set;
	volatile uint8_t rsvd_1[22u];
};

struct usbh_atsamx_reg {
	/*  USB HOST GENERAL REGISTERS  */
	/* Control A*/
	volatile uint8_t ctrl_a;
	volatile uint8_t rsvd_0;
	/* Synchronization Busy*/
	volatile uint8_t sync_busy;
	/* QOS control*/
	volatile uint8_t qos_ctrl;
	volatile uint32_t rsvd_1;
	/* Control B*/
	volatile uint16_t ctrl_b;
	/* Host Start-of-Frame control*/
	volatile uint8_t h_sof_c;
	volatile uint8_t rsvd_2;
	/* Status*/
	volatile uint8_t status;
	/* Finite state machine status*/
	volatile uint8_t fsm_status;
	volatile uint16_t rsvd_3;
	/* Host frame number*/
	volatile uint16_t frm_nbr;
	/* Host frame length*/
	volatile uint8_t frm_len_high;
	volatile uint8_t rsvd_4;
	/* Host interrupt enable register clear*/
	volatile uint16_t int_en_clr;
	volatile uint16_t rsvd_5;
	/* Host interrupt enable register set*/
	volatile uint16_t int_en_set;
	volatile uint16_t rsvd_6;
	/* Host interrupt flag status and clear*/
	volatile uint16_t int_flag;
	volatile uint16_t rsvd7;
	/* Pipe interrupt summary*/
	volatile uint16_t p_int_smry;
	volatile uint16_t rsvd_8;
	/* Descriptor address*/
	volatile uint32_t desc_addr;
	/* Pad calibration*/
	volatile uint16_t pad_cal;
	volatile uint8_t rsvd_9[214u];
	/* Host pipes*/
	struct usbh_atsamx_pipe_reg h_pipe[ATSAMX_MAX_NBR_PIPE];
};

struct usbh_atsamx_pinfo {
	/* Device addr | EP DIR | EP NBR.*/
	uint16_t ep_addr;
	/*
	 * To ensure the urb EP xfer size is not corrupted
	 * for multi-transaction transfer
	 */
	uint16_t appbuf_len;
	uint32_t next_xfer_len;
	struct usbh_urb *urb_ptr;
};

struct usbh_drv_data {
	struct usbh_atsamx_pipe_desc desc_tbl[ATSAMX_MAX_NBR_PIPE];
	struct usbh_atsamx_pinfo pipe_tbl[ATSAMX_MAX_NBR_PIPE];
	/* Bit array for BULK, ISOC, INTR, CTRL pipe mgmt.*/
	uint16_t pipe_used;
	/* RH desc content.*/
	uint8_t rh_desc[USBH_HUB_LEN_HUB_DESC];
	/* Root Hub Port status.*/
	uint16_t rh_port_stat;
	/* Root Hub Port status change.*/
	uint16_t rh_port_chng;
};

static struct k_thread htask;
static struct usbh_hc_drv *p_hc_drv_local;

K_MSGQ_DEFINE(atsamx_urb_proc_q, sizeof(struct usbh_urb), ATSAMX_URB_PROC_Q_MAX,
	      4);

K_THREAD_STACK_DEFINE(atsamx_urb_task_stk, ATSAMX_URB_PROC_TASK_STK_SIZE);

K_MEM_POOL_DEFINE(atsamx_drv_pool, DT_INST_PROP(0, buf_len),
		  DT_INST_PROP(0, buf_len),
		  (DT_INST_PROP(0, nbr_ep_bulk) + DT_INST_PROP(0, nbr_ep_intr) +
		   1),
		  sizeof(uint32_t));

/* --------------- DRIVER API FUNCTIONS --------------- */
static void usbh_atsamx_hcd_init(struct usbh_hc_drv *p_hc_drv, int *p_err);

static void usbh_atsamx_hcd_start(struct usbh_hc_drv *p_hc_drv, int *p_err);

static void usbh_atsamx_hcd_stop(struct usbh_hc_drv *p_hc_drv, int *p_err);

static enum usbh_device_speed
usbh_atsamx_hcd_spd_get(struct usbh_hc_drv *p_hc_drv, int *p_err);

static void usbh_atsamx_hcd_suspend(struct usbh_hc_drv *p_hc_drv, int *p_err);

static void usbh_atsamx_hcd_resume(struct usbh_hc_drv *p_hc_drv, int *p_err);

static uint32_t usbh_atsamx_hcd_frame_nbr_get(struct usbh_hc_drv *p_hc_drv,
					      int *p_err);

static void usbh_atsamx_hcd_ep_open(struct usbh_hc_drv *p_hc_drv,
				    struct usbh_ep *p_ep, int *p_err);

static void usbh_atsamx_hcd_ep_close(struct usbh_hc_drv *p_hc_drv,
				     struct usbh_ep *p_ep, int *p_err);

static void usbh_atsamx_hcd_ep_abort(struct usbh_hc_drv *p_hc_drv,
				     struct usbh_ep *p_ep, int *p_err);

static bool usbh_atsamx_hcd_ep_is_halt(struct usbh_hc_drv *p_hc_drv,
				       struct usbh_ep *p_ep, int *p_err);

static void usbh_atsamx_hcd_urb_submit(struct usbh_hc_drv *p_hc_drv,
				       struct usbh_urb *p_urb, int *p_err);

static void usbh_atsamx_hcd_urb_complete(struct usbh_hc_drv *p_hc_drv,
					 struct usbh_urb *p_urb, int *p_err);

static void usbh_atsamx_hcd_urb_abort(struct usbh_hc_drv *p_hc_drv,
				      struct usbh_urb *p_urb, int *p_err);

/*  ROOT HUB API FUNCTIONS  */
static bool
usbh_atsamx_rh_port_status_get(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr,
			       struct usbh_hub_port_status *p_port_status);

static bool usbh_atsamx_rh_hub_desc_get(struct usbh_hc_drv *p_hc_drv,
					void *p_buf, uint8_t buf_len);

static bool usbh_atsamx_rh_port_en_set(struct usbh_hc_drv *p_hc_drv,
				       uint8_t port_nbr);

static bool usbh_atsamx_rh_port_en_clr(struct usbh_hc_drv *p_hc_drv,
				       uint8_t port_nbr);

static bool usbh_atsamx_rh_port_en_chng_clr(struct usbh_hc_drv *p_hc_drv,
					    uint8_t port_nbr);

static bool usbh_atsamx_rh_port_pwr_set(struct usbh_hc_drv *p_hc_drv,
					uint8_t port_nbr);

static bool usbh_atsamx_rh_port_pwr_clr(struct usbh_hc_drv *p_hc_drv,
					uint8_t port_nbr);

static bool usbh_atsamx_hcd_port_reset_set(struct usbh_hc_drv *p_hc_drv,
					   uint8_t port_nbr);

static bool usbh_atsamx_hcd_port_reset_chng_clr(struct usbh_hc_drv *p_hc_drv,
						uint8_t port_nbr);

static bool usbh_atsamx_hcd_port_suspend_clr(struct usbh_hc_drv *p_hc_drv,
					     uint8_t port_nbr);

static bool usbh_atsamx_hcd_port_conn_chng_clr(struct usbh_hc_drv *p_hc_drv,
					       uint8_t port_nbr);

static bool usbh_atsamx_rh_int_en(struct usbh_hc_drv *p_hc_drv);

static bool usbh_atsamx_rh_int_dis(struct usbh_hc_drv *p_hc_drv);

static void usbh_atsamx_isr_callback(void *p_drv);

static void usbh_atsamx_process_urb(void *p_arg, void *p_arg2, void *p_arg3);

static uint8_t usbh_atsamx_get_free_pipe(struct usbh_drv_data *p_drv_data);

static uint8_t usbh_atsamx_get_pipe_nbr(struct usbh_drv_data *p_drv_data,
					struct usbh_ep *p_ep);

static void usbh_atsamx_pipe_cfg(struct usbh_urb *p_urb,
				 struct usbh_atsamx_pipe_reg *p_reg_hpipe,
				 struct usbh_atsamx_pinfo *p_pipe_info,
				 struct usbh_atsamx_desc_bank *p_desc_bank);

const struct usbh_hc_drv_api usbh_hcd_api = {
	.init = usbh_atsamx_hcd_init,
	.start = usbh_atsamx_hcd_start,
	.stop = usbh_atsamx_hcd_stop,
	.spd_get = usbh_atsamx_hcd_spd_get,
	.suspend = usbh_atsamx_hcd_suspend,
	.resume = usbh_atsamx_hcd_resume,
	.frm_nbr_get = usbh_atsamx_hcd_frame_nbr_get,

	.ep_open = usbh_atsamx_hcd_ep_open,
	.ep_close = usbh_atsamx_hcd_ep_close,
	.ep_abort = usbh_atsamx_hcd_ep_abort,
	.ep_halt = usbh_atsamx_hcd_ep_is_halt,

	.urb_submit = usbh_atsamx_hcd_urb_submit,
	.urb_complete = usbh_atsamx_hcd_urb_complete,
	.urb_abort = usbh_atsamx_hcd_urb_abort,
};

const struct usbh_hc_rh_api usbh_hcd_rh_api = {
	.status_get = usbh_atsamx_rh_port_status_get,
	.desc_get = usbh_atsamx_rh_hub_desc_get,

	.en_set = usbh_atsamx_rh_port_en_set,
	.en_clr = usbh_atsamx_rh_port_en_clr,
	.en_chng_clr = usbh_atsamx_rh_port_en_chng_clr,

	.pwr_set = usbh_atsamx_rh_port_pwr_set,
	.pwr_clr = usbh_atsamx_rh_port_pwr_clr,

	.rst_set = usbh_atsamx_hcd_port_reset_set,
	.rst_chng_clr = usbh_atsamx_hcd_port_reset_chng_clr,

	.suspend_clr = usbh_atsamx_hcd_port_suspend_clr,
	.conn_chng_clr = usbh_atsamx_hcd_port_conn_chng_clr,

	.int_en = usbh_atsamx_rh_int_en,
	.int_dis = usbh_atsamx_rh_int_dis
};

/*
 * Initialize host controller and allocate driver's internal memory/variables.
 */
static void usbh_atsamx_hcd_init(struct usbh_hc_drv *p_hc_drv, int *p_err)
{
	struct usbh_drv_data *p_drv_data;

	p_drv_data = k_malloc(sizeof(struct usbh_drv_data));
	if (p_drv_data == NULL) {
		*p_err = ENOMEM;
		return;
	}

	memset(p_drv_data, 0, sizeof(struct usbh_drv_data));
	p_hc_drv->data_ptr = (void *)p_drv_data;

	if ((DT_INST_PROP(0, buf_len) % USBH_MAX_EP_SIZE_TYPE_BULK_FS) != 0) {
		*p_err = ENOMEM;
		return;
	}

	k_thread_create(&htask, atsamx_urb_task_stk,
			K_THREAD_STACK_SIZEOF(atsamx_urb_task_stk),
			usbh_atsamx_process_urb, (void *)p_hc_drv, NULL, NULL,
			0, 0, K_NO_WAIT);

	*p_err = 0;
}

/*
 * Start Host Controller.
 */
static void usbh_atsamx_hcd_start(struct usbh_hc_drv *p_hc_drv, int *p_err)
{
	p_hc_drv_local = p_hc_drv;
	struct usbh_atsamx_reg *p_reg;
	struct usbh_drv_data *p_drv_data;
	uint32_t pad_transn;
	uint32_t pad_transp;
	uint32_t pad_trim;
	uint8_t reg_val;
	uint8_t i;
	int key;

	p_reg = (struct usbh_atsamx_reg *)DT_INST_REG_ADDR(0);
	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;

	/* maybe not enough for all atsam boards */
	PM->AHBMASK.bit.USB_ = 1;
	PM->APBBMASK.bit.USB_ = 1;

	/* Enable the GCLK */
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_USB | GCLK_CLKCTRL_GEN_GCLK0 |
			    GCLK_CLKCTRL_CLKEN;

	while (GCLK->STATUS.bit.SYNCBUSY) {
	}
	/* --- */

	if (!(p_reg->sync_busy & USBH_ATSAMX_SYNCBUSY_SWRST)) {
		/* Check if synchronization is completed*/
		while (p_reg->sync_busy & USBH_ATSAMX_SYNCBUSY_MSK)
			;
		reg_val = p_reg->ctrl_a;
		if (reg_val & USBH_ATSAMX_CTRLA_ENABLE) {
			reg_val &= ~USBH_ATSAMX_CTRLA_ENABLE;
			key = irq_lock();
			p_reg->ctrl_a = reg_val;
			while (p_reg->sync_busy & USBH_ATSAMX_SYNCBUSY_MSK)
				;
			/* Disable USB peripheral*/
			irq_unlock(key);
			while (p_reg->sync_busy & USBH_ATSAMX_SYNCBUSY_ENABLE) {
			}
		}
		/*
		 * Resets all regs in the USB to initial state,
		 * and the USB will be disabled
		 */
		key = irq_lock();
		p_reg->ctrl_a = USBH_ATSAMX_CTRLA_SWRST;
		while (p_reg->sync_busy & USBH_ATSAMX_SYNCBUSY_MSK) {
		}
		irq_unlock(key);
	}

	while (p_reg->sync_busy & USBH_ATSAMX_SYNCBUSY_SWRST) {
	}

	/* LOAD PAD CALIBRATION REG WITH PRODUCTION VALUES */
	pad_transn = ATSAMX_NVM_USB_TRANSN;
	pad_transp = ATSAMX_NVM_USB_TRANSP;
	pad_trim = ATSAMX_NVM_USB_TRIM;

	if ((pad_transn == 0) || (pad_transn == ATSAMX_NVM_USB_TRANSN_MSK)) {
		/* Default value*/
		pad_transn = 9u;
	}
	if ((pad_transp == 0) || (pad_transp == ATSAMX_NVM_USB_TRANSP_MSK)) {
		/* Default value*/
		pad_transp = 25u;
	}
	if ((pad_trim == 0) || (pad_trim == ATSAMX_NVM_USB_TRIM_MSK)) {
		/* Default value*/
		pad_trim = 6u;
	}

	p_reg->pad_cal =
		((pad_transp << 0) | (pad_transn << 6) | (pad_trim << 12u));

	/* Write quality of service RAM access*/
	p_reg->qos_ctrl =
		(USBH_ATSAMX_QOSCTRL_DQOS_HIGH | USBH_ATSAMX_QOSCTRL_CQOS_HIGH);
	/* Set Host mode & set USB clk to run in standby mode   */
	key = irq_lock();
	p_reg->ctrl_a =
		(USBH_ATSAMX_CTRLA_HOST_MODE | USBH_ATSAMX_CTRLA_RUNSTBY);
	while (p_reg->sync_busy & USBH_ATSAMX_SYNCBUSY_MSK)
		;
	irq_unlock(key);
	/* Set Pipe Descriptor address*/
	p_reg->desc_addr = (uint32_t)&p_drv_data->desc_tbl[0];
	/* Clear USB speed configuration*/
	p_reg->ctrl_b &= ~USBH_ATSAMX_CTRLB_SPDCONF_MSK;
	/* Set USB LS/FS speed configuration*/
	p_reg->ctrl_b |=
		(USBH_ATSAMX_CTRLB_SPDCONF_LSFS | USBH_ATSAMX_CTRLB_VBUSOK);

	IRQ_CONNECT(DT_INST_IRQN(0), 0, usbh_atsamx_isr_callback, 0, 0);
	irq_enable(DT_INST_IRQN(0));

	while (p_reg->sync_busy & USBH_ATSAMX_SYNCBUSY_MSK)
		;
	reg_val = p_reg->ctrl_a;
	if ((reg_val & USBH_ATSAMX_CTRLA_ENABLE) == 0) {
		key = irq_lock();
		p_reg->ctrl_a = (reg_val | USBH_ATSAMX_CTRLA_ENABLE);
		while (p_reg->sync_busy & USBH_ATSAMX_SYNCBUSY_MSK) {
		}
		irq_unlock(key);
		while (p_reg->sync_busy & USBH_ATSAMX_SYNCBUSY_ENABLE) {
		}
	}

	for (i = 0; i < ATSAMX_MAX_NBR_PIPE; i++) {
		/* Disable the pipe*/
		p_reg->h_pipe[i].p_cfg = 0;
		/* Set default pipe info fields.*/
		p_drv_data->pipe_tbl[i].appbuf_len = 0;
		p_drv_data->pipe_tbl[i].next_xfer_len = 0;
		p_drv_data->pipe_tbl[i].ep_addr = ATSAMX_DFLT_EP_ADDR;
	}
	/* Clear all interrupts*/
	p_reg->int_flag = USBH_ATSAMX_INT_MSK;
	/* Enable interrupts to detect connection*/
	p_reg->int_en_set = (USBH_ATSAMX_INT_DCONN | USBH_ATSAMX_INT_RST |
			     USBH_ATSAMX_INT_WAKEUP);
	*p_err = 0;
}

/*
 * Stop Host Controller.
 */
static void usbh_atsamx_hcd_stop(struct usbh_hc_drv *p_hc_drv, int *p_err)
{
	LOG_DBG("Stop");

	struct usbh_atsamx_reg *p_reg;

	p_reg = (struct usbh_atsamx_reg *)DT_INST_REG_ADDR(0);
	/* Disable all interrupts*/
	p_reg->int_en_clr = USBH_ATSAMX_INT_MSK;
	/* Clear all interrupts*/
	p_reg->int_flag = USBH_ATSAMX_INT_MSK;

	irq_disable(7);

	p_reg->ctrl_b &= ~USBH_ATSAMX_CTRLB_VBUSOK;

	*p_err = 0;
}

/*
 * Returns Host Controller speed.
 */
static enum usbh_device_speed
usbh_atsamx_hcd_spd_get(struct usbh_hc_drv *p_hc_drv, int *p_err)
{
	*p_err = 0;

	return USBH_FULL_SPEED;
}

/*
 * Suspend Host Controller.
 */
static void usbh_atsamx_hcd_suspend(struct usbh_hc_drv *p_hc_drv, int *p_err)
{
	LOG_DBG("Suspend");
	struct usbh_atsamx_reg *p_reg;
	struct usbh_drv_data *p_drv_data;
	uint8_t pipe_nbr;

	p_reg = (struct usbh_atsamx_reg *)DT_INST_REG_ADDR(0);
	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;

	for (pipe_nbr = 0; pipe_nbr < ATSAMX_MAX_NBR_PIPE; pipe_nbr++) {
		if (DEF_BIT_IS_SET(p_drv_data->pipe_used, BIT(pipe_nbr))) {
			/* Stop transfer*/
			p_reg->h_pipe[pipe_nbr].p_status_set =
				USBH_ATSAMX_PSTATUS_PFREEZE;
		}
	}
	/* wait at least 1 complete frame*/
	k_sleep(K_MSEC(1));
	/* Stop sending start of frames*/
	p_reg->ctrl_b &= ~USBH_ATSAMX_CTRLB_SOFE;

	*p_err = 0;
}

/*
 * Resume Host Controller.
 */
static void usbh_atsamx_hcd_resume(struct usbh_hc_drv *p_hc_drv, int *p_err)
{
	LOG_DBG("Resume");
	struct usbh_atsamx_reg *p_reg;
	struct usbh_drv_data *p_drv_data;
	uint8_t pipe_nbr;

	p_reg = (struct usbh_atsamx_reg *)DT_INST_REG_ADDR(0);
	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;
	/* Start sending start of frames*/
	p_reg->ctrl_b |= USBH_ATSAMX_CTRLB_SOFE;
	/* Do resume to downstream*/
	p_reg->ctrl_b |= USBH_ATSAMX_CTRLB_RESUME;
	/* Force a wakeup interrupt*/
	p_reg->int_en_set = USBH_ATSAMX_INT_WAKEUP;

	k_sleep(K_MSEC(20u));

	for (pipe_nbr = 0; pipe_nbr < ATSAMX_MAX_NBR_PIPE; pipe_nbr++) {
		if (DEF_BIT_IS_SET(p_drv_data->pipe_used, BIT(pipe_nbr))) {
			/* Start transfer*/
			p_reg->h_pipe[pipe_nbr].p_status_clr =
				USBH_ATSAMX_PSTATUS_PFREEZE;
		}
	}

	*p_err = 0;
}

/*
 * Retrieve current frame number.
 */
static uint32_t usbh_atsamx_hcd_frame_nbr_get(struct usbh_hc_drv *p_hc_drv,
					      int *p_err)
{
	struct usbh_atsamx_reg *p_reg;
	uint32_t frm_nbr;

	p_reg = (struct usbh_atsamx_reg *)DT_INST_REG_ADDR(0);
	frm_nbr =
		(p_reg->frm_nbr & USBH_ATSAMX_FNUM_MSK) >> USBH_ATSAMX_FNUM_POS;

	*p_err = 0;

	return frm_nbr;
}

/*
 * Allocate/open endpoint of given type.
 */
static void usbh_atsamx_hcd_ep_open(struct usbh_hc_drv *p_hc_drv,
				    struct usbh_ep *p_ep, int *p_err)
{
	struct usbh_atsamx_reg *p_reg;

	p_reg = (struct usbh_atsamx_reg *)DT_INST_REG_ADDR(0);
	/* Set PID to DATA0*/
	p_ep->data_pid = 0;
	p_ep->urb.err = 0;
	*p_err = 0;

	if (p_reg->status == 0) {
		/* Do not open Endpoint if device is disconnected*/
		LOG_ERR("device not connected");
		*p_err = EIO;
		return;
	}
}

/*
 * Close specified endpoint.
 */
static void usbh_atsamx_hcd_ep_close(struct usbh_hc_drv *p_hc_drv,
				     struct usbh_ep *p_ep, int *p_err)
{
	struct usbh_atsamx_reg *p_reg;
	struct usbh_drv_data *p_drv_data;
	uint8_t pipe_nbr;

	p_reg = (struct usbh_atsamx_reg *)DT_INST_REG_ADDR(0);
	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;
	pipe_nbr = usbh_atsamx_get_pipe_nbr(p_drv_data, p_ep);
	/* Set PID to DATA0*/
	p_ep->data_pid = 0;
	*p_err = 0;

	if (p_ep->urb.dma_buf_ptr != NULL) {
		k_free(p_ep->urb.dma_buf_ptr);
		p_ep->urb.dma_buf_ptr = NULL;
	}

	if (pipe_nbr != ATSAMX_INVALID_PIPE) {
		/* Disable all pipe interrupts*/
		p_reg->h_pipe[pipe_nbr].p_int_clr = USBH_ATSAMX_PINT_ALL;
		/* Clear all pipe interrupt flags*/
		p_reg->h_pipe[pipe_nbr].p_int_flag = USBH_ATSAMX_PINT_ALL;
		/* Disable the pipe*/
		p_reg->h_pipe[pipe_nbr].p_cfg = 0;

		p_drv_data->pipe_tbl[pipe_nbr].ep_addr = ATSAMX_DFLT_EP_ADDR;
		p_drv_data->pipe_tbl[pipe_nbr].appbuf_len = 0;
		p_drv_data->pipe_tbl[pipe_nbr].next_xfer_len = 0;
		WRITE_BIT(p_drv_data->pipe_used, pipe_nbr, 0);
	}
}

/*
 * Abort all pending URBs on endpoint.
 */
static void usbh_atsamx_hcd_ep_abort(struct usbh_hc_drv *p_hc_drv,
				     struct usbh_ep *p_ep, int *p_err)
{
	*p_err = 0;
}

/*
 * Retrieve endpoint halt state.
 */
static bool usbh_atsamx_hcd_ep_is_halt(struct usbh_hc_drv *p_hc_drv,
				       struct usbh_ep *p_ep, int *p_err)
{
	*p_err = 0;
	if (p_ep->urb.err == EIO) {
		return true;
	}

	return false;
}

/*
 * Submit specified urb.
 */
static void usbh_atsamx_hcd_urb_submit(struct usbh_hc_drv *p_hc_drv,
				       struct usbh_urb *p_urb, int *p_err)
{
	struct usbh_atsamx_reg *p_reg;
	struct usbh_drv_data *p_drv_data;
	uint8_t ep_type;
	uint8_t pipe_nbr;

	p_reg = (struct usbh_atsamx_reg *)DT_INST_REG_ADDR(0);
	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;
	ep_type = usbh_ep_type_get(p_urb->ep_ptr);

	if (p_urb->state == USBH_URB_STATE_ABORTED) {
		*p_err = EIO;
		return;
	}

	pipe_nbr = usbh_atsamx_get_free_pipe(p_drv_data);
	if (pipe_nbr == ATSAMX_INVALID_PIPE) {
		*p_err = EBUSY;
		return;
	}
	LOG_DBG("pipe %d set ep address %d", pipe_nbr,
		((p_urb->ep_ptr->dev_addr << 8) |
		 p_urb->ep_ptr->desc.b_endpoint_address));
	p_drv_data->pipe_tbl[pipe_nbr].ep_addr =
		((p_urb->ep_ptr->dev_addr << 8) |
		 p_urb->ep_ptr->desc.b_endpoint_address);
	p_drv_data->pipe_tbl[pipe_nbr].appbuf_len = 0;
	p_drv_data->pipe_tbl[pipe_nbr].next_xfer_len = 0;

	if (p_urb->dma_buf_ptr == NULL) {
		p_urb->dma_buf_ptr = k_mem_pool_malloc(
			&atsamx_drv_pool, DT_INST_PROP(0, buf_len));
		if (p_urb->dma_buf_ptr == NULL) {
			*p_err = ENOMEM;
			return;
		}
	}
	/* Set pipe type and single bank*/
	p_reg->h_pipe[pipe_nbr].p_cfg = USBH_ATSAMX_PCFG_PTYPE(ep_type);
	/* Disable all interrupts*/
	p_reg->h_pipe[pipe_nbr].p_int_clr = USBH_ATSAMX_PINT_ALL;
	/* Clear all interrupts*/
	p_reg->h_pipe[pipe_nbr].p_int_flag = USBH_ATSAMX_PINT_ALL;

	/*enable general error and stall interrupts*/
	p_reg->h_pipe[pipe_nbr].p_int_set =
		(USBH_ATSAMX_PINT_STALL | USBH_ATSAMX_PINT_PERR);

	if ((ep_type == USBH_EP_TYPE_BULK) && (p_urb->ep_ptr->interval < 1)) {
		p_reg->h_pipe[pipe_nbr].b_interval = 1;
	} else {
		p_reg->h_pipe[pipe_nbr].b_interval = p_urb->ep_ptr->interval;
	}
	/* Store transaction urb*/
	p_drv_data->pipe_tbl[pipe_nbr].urb_ptr = p_urb;

	switch (ep_type) {
	case USBH_EP_TYPE_CTRL:
		if (p_urb->token == USBH_TOKEN_SETUP) {
			/* Enable setup transfer interrupt*/
			p_reg->h_pipe[pipe_nbr].p_int_set =
				USBH_ATSAMX_PINT_TXSTP;
			/* Set PID to DATA0*/
			p_reg->h_pipe[pipe_nbr].p_status_clr =
				USBH_ATSAMX_PSTATUS_DTGL;
		} else {
			/* Enable transfer complete interrupt*/
			p_reg->h_pipe[pipe_nbr].p_int_set =
				USBH_ATSAMX_PINT_TRCPT;
			/* Set PID to DATA1*/
			p_reg->h_pipe[pipe_nbr].p_status_set =
				USBH_ATSAMX_PSTATUS_DTGL;
		}
		break;

	case USBH_EP_TYPE_INTR:
	case USBH_EP_TYPE_BULK:
		/* Enable transfer complete interrupt*/
		p_reg->h_pipe[pipe_nbr].p_int_set = USBH_ATSAMX_PINT_TRCPT;

		if (p_urb->ep_ptr->data_pid == 0) {
			/* Set PID to DATA0*/
			p_reg->h_pipe[pipe_nbr].p_status_clr =
				USBH_ATSAMX_PSTATUS_DTGL;
		} else {
			/* Set PID to DATA1*/
			p_reg->h_pipe[pipe_nbr].p_status_set =
				USBH_ATSAMX_PSTATUS_DTGL;
		}
		break;

	case USBH_EP_TYPE_ISOC:
	default:
		*p_err = ENOTSUP;
		return;
	}

	usbh_atsamx_pipe_cfg(p_urb, &p_reg->h_pipe[pipe_nbr],
			     &p_drv_data->pipe_tbl[pipe_nbr],
			     &p_drv_data->desc_tbl[pipe_nbr].desc_bank[0]);
	/* Start transfer*/
	p_reg->h_pipe[pipe_nbr].p_status_clr = USBH_ATSAMX_PSTATUS_PFREEZE;
	*p_err = 0;
}

/*
 * Complete specified urb.
 */
static void usbh_atsamx_hcd_urb_complete(struct usbh_hc_drv *p_hc_drv,
					 struct usbh_urb *p_urb, int *p_err)
{
	struct usbh_atsamx_reg *p_reg;
	struct usbh_drv_data *p_drv_data;
	uint8_t pipe_nbr;
	uint16_t xfer_len;
	int key;

	*p_err = 0;
	p_reg = (struct usbh_atsamx_reg *)DT_INST_REG_ADDR(0);
	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;
	pipe_nbr = usbh_atsamx_get_pipe_nbr(p_drv_data, p_urb->ep_ptr);

	key = irq_lock();
	xfer_len = USBH_ATSAMX_GET_BYTE_CNT(
		p_drv_data->desc_tbl[pipe_nbr].desc_bank[0].pck_size);

	if (p_urb->token == USBH_TOKEN_IN) {
		/*  HANDLE IN TRANSACTIONS  */
		memcpy((void *)((uint32_t)p_urb->userbuf_ptr + p_urb->xfer_len),
		       p_urb->dma_buf_ptr, xfer_len);

		/* Check if it rx'd more data than what was expected*/
		if (p_drv_data->pipe_tbl[pipe_nbr].appbuf_len >
		    p_urb->uberbuf_len) {
			p_urb->xfer_len += xfer_len;
			p_urb->err = EIO;
		} else {
			p_urb->xfer_len += xfer_len;
		}
	} else {
		/*  HANDLE SETUP/OUT TRANSACTIONS  */
		xfer_len =
			(p_drv_data->pipe_tbl[pipe_nbr].appbuf_len - xfer_len);
		if (xfer_len == 0) {
			p_urb->xfer_len +=
				p_drv_data->pipe_tbl[pipe_nbr].next_xfer_len;
		} else {
			p_urb->xfer_len +=
				(p_drv_data->pipe_tbl[pipe_nbr].next_xfer_len -
				 xfer_len);
		}
	}
	/* Disable all pipe interrupts*/
	p_reg->h_pipe[pipe_nbr].p_int_clr = USBH_ATSAMX_PINT_ALL;
	/* Clear all pipe interrupts*/
	p_reg->h_pipe[pipe_nbr].p_int_flag = USBH_ATSAMX_PINT_ALL;
	/* Disable the pipe*/
	p_reg->h_pipe[pipe_nbr].p_cfg = 0;
	irq_unlock(key);

	k_free(p_urb->dma_buf_ptr);
	p_urb->dma_buf_ptr = NULL;

	p_drv_data->pipe_tbl[pipe_nbr].ep_addr = ATSAMX_DFLT_EP_ADDR;
	p_drv_data->pipe_tbl[pipe_nbr].appbuf_len = 0;
	p_drv_data->pipe_tbl[pipe_nbr].next_xfer_len = 0;
	p_drv_data->pipe_tbl[pipe_nbr].urb_ptr = NULL;
	WRITE_BIT(p_drv_data->pipe_used, pipe_nbr, 0);
}

/*
 * Abort specified urb.
 */
static void usbh_atsamx_hcd_urb_abort(struct usbh_hc_drv *p_hc_drv,
				      struct usbh_urb *p_urb, int *p_err)
{
	p_urb->state = USBH_URB_STATE_ABORTED;
	p_urb->err = EAGAIN;

	*p_err = 0;
}

/*
 * Retrieve port status changes and port status.
 */
static bool
usbh_atsamx_rh_port_status_get(struct usbh_hc_drv *p_hc_drv, uint8_t port_nbr,
			       struct usbh_hub_port_status *p_port_status)
{
	struct usbh_atsamx_reg *p_reg;
	struct usbh_drv_data *p_drv_data;
	uint8_t reg_val;

	p_reg = (struct usbh_atsamx_reg *)DT_INST_REG_ADDR(0);
	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;

	if (port_nbr != 1) {
		p_port_status->w_port_status = 0;
		p_port_status->w_port_change = 0;
		return 0;
	}
	/* Bits not used by the stack. Maintain constant value. */
	p_drv_data->rh_port_stat &= ~USBH_HUB_STATUS_PORT_TEST;
	p_drv_data->rh_port_stat &= ~USBH_HUB_STATUS_PORT_INDICATOR;

	reg_val = (p_reg->status & USBH_ATSAMX_STATUS_SPEED_MSK) >>
		  USBH_ATSAMX_STATUS_SPEED_POS;
	if (reg_val == USBH_ATSAMX_STATUS_SPEED_FS) {
		/*  FULL-SPEED DEVICE ATTACHED  */
		/* PORT_LOW_SPEED  = 0 = FS dev attached.*/
		p_drv_data->rh_port_stat &= ~USBH_HUB_STATUS_PORT_LOW_SPD;
		/* PORT_HIGH_SPEED = 0 = FS dev attached.*/
		p_drv_data->rh_port_stat &= ~USBH_HUB_STATUS_PORT_HIGH_SPD;
	} else if (reg_val == USBH_ATSAMX_STATUS_SPEED_LS) {
		/*  LOW-SPEED DEVICE ATTACHED  */
		/* PORT_LOW_SPEED  = 1 = LS dev attached.*/
		p_drv_data->rh_port_stat |= USBH_HUB_STATUS_PORT_LOW_SPD;
		/* PORT_HIGH_SPEED = 0 = LS dev attached.*/
		p_drv_data->rh_port_stat &= ~USBH_HUB_STATUS_PORT_HIGH_SPD;
	}

	p_port_status->w_port_status = p_drv_data->rh_port_stat;
	p_port_status->w_port_change = p_drv_data->rh_port_chng;

	return 1;
}

/*
 * Retrieve root hub descriptor.
 */
static bool usbh_atsamx_rh_hub_desc_get(struct usbh_hc_drv *p_hc_drv,
					void *p_buf, uint8_t buf_len)
{
	struct usbh_drv_data *p_drv_data;
	struct usbh_hub_desc hub_desc;

	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;

	hub_desc.b_desc_length = USBH_HUB_LEN_HUB_DESC;
	hub_desc.b_desc_type = USBH_HUB_DESC_TYPE_HUB;
	hub_desc.b_nbr_ports = 1;
	hub_desc.w_hub_characteristics = 0;
	hub_desc.b_pwr_on_to_pwr_good = 100u;
	hub_desc.b_hub_contr_current = 0;
	/* Write the structure in USB format*/
	usbh_hub_fmt_hub_desc(&hub_desc, p_drv_data->rh_desc);
	/* Copy the formatted structure into the buffer*/
	buf_len = MIN(buf_len, sizeof(struct usbh_hub_desc));
	memcpy(p_buf, p_drv_data->rh_desc, buf_len);

	return 1;
}

/*
 * Enable given port.
 */
static bool usbh_atsamx_rh_port_en_set(struct usbh_hc_drv *p_hc_drv,
				       uint8_t port_nbr)
{
	return 1;
}

/*
 * Clear port enable status.
 */
static bool usbh_atsamx_rh_port_en_clr(struct usbh_hc_drv *p_hc_drv,
				       uint8_t port_nbr)
{
	struct usbh_drv_data *p_drv_data;

	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;
	/* Bit is clr by ClearPortFeature(PORT_ENABLE)*/
	p_drv_data->rh_port_stat &= ~USBH_HUB_STATUS_PORT_EN;

	return 1;
}

/*
 * Clear port enable status change.
 */
static bool usbh_atsamx_rh_port_en_chng_clr(struct usbh_hc_drv *p_hc_drv,
					    uint8_t port_nbr)
{
	struct usbh_drv_data *p_drv_data;

	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;
	/* Bit is clr by ClearPortFeature(C_PORT_ENABLE)*/
	p_drv_data->rh_port_chng &= ~USBH_HUB_STATUS_C_PORT_EN;

	return 1;
}

/*
 * Set port power based on port power mode.
 */
static bool usbh_atsamx_rh_port_pwr_set(struct usbh_hc_drv *p_hc_drv,
					uint8_t port_nbr)
{
	struct usbh_drv_data *p_drv_data;

	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;
	/* Bit is set by SetPortFeature(PORT_POWER) request*/
	p_drv_data->rh_port_stat |= USBH_HUB_STATUS_PORT_PWR;

	return 1;
}

/*
 * Clear port power.
 */
static bool usbh_atsamx_rh_port_pwr_clr(struct usbh_hc_drv *p_hc_drv,
					uint8_t port_nbr)
{
	return 1;
}

/*
 * Reset given port.
 */
static bool usbh_atsamx_hcd_port_reset_set(struct usbh_hc_drv *p_hc_drv,
					   uint8_t port_nbr)
{
	struct usbh_atsamx_reg *p_reg;
	struct usbh_drv_data *p_drv_data;

	p_reg = (struct usbh_atsamx_reg *)DT_INST_REG_ADDR(0);
	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;
	/* This bit is set while in the resetting state*/
	p_drv_data->rh_port_stat |= USBH_HUB_STATUS_PORT_RESET;
	/* Send USB reset signal.*/
	p_reg->ctrl_b |= USBH_ATSAMX_CTRLB_BUSRESET;

	return 1;
}

/*
 * Clear port reset status change.
 */
static bool usbh_atsamx_hcd_port_reset_chng_clr(struct usbh_hc_drv *p_hc_drv,
						uint8_t port_nbr)
{
	struct usbh_drv_data *p_drv_data;

	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;
	/* Bit is clr by ClearPortFeature(C_PORT_RESET) request */
	p_drv_data->rh_port_chng &= ~USBH_HUB_STATUS_C_PORT_RESET;

	return 1;
}

/*
 * Resume given port if port is suspended.
 */
static bool usbh_atsamx_hcd_port_suspend_clr(struct usbh_hc_drv *p_hc_drv,
					     uint8_t port_nbr)
{
	return 1;
}

/*
 * Clear port connect status change.
 */
static bool usbh_atsamx_hcd_port_conn_chng_clr(struct usbh_hc_drv *p_hc_drv,
					       uint8_t port_nbr)
{
	struct usbh_drv_data *p_drv_data;

	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;
	p_drv_data->rh_port_chng ^= USBH_HUB_STATUS_C_PORT_CONN;

	return 1;
}

/*
 * Enable root hub interrupt.
 */
static bool usbh_atsamx_rh_int_en(struct usbh_hc_drv *p_hc_drv)
{
	return 1;
}

/*
 * Disable root hub interrupt.
 */
static bool usbh_atsamx_rh_int_dis(struct usbh_hc_drv *p_hc_drv)
{
	return 1;
}

/*
 * ISR handler.
 */
static void usbh_atsamx_isr_callback(void *p_drv)
{
	struct usbh_atsamx_reg *p_reg;
	struct usbh_drv_data *p_drv_data;
	struct usbh_hc_drv *p_hc_drv;
	uint16_t int_stat;
	uint16_t pipe_stat;
	uint16_t pipe_nbr;
	uint16_t xfer_len;
	uint16_t max_pkt_size;
	struct usbh_urb *p_urb;

	p_hc_drv = (struct usbh_hc_drv *)p_hc_drv_local;
	p_reg = (struct usbh_atsamx_reg *)DT_INST_REG_ADDR(0);
	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;
	int_stat = p_reg->int_flag;
	int_stat &= p_reg->int_en_set;

	/*  HANDLE INTERRUPT FLAG status  */
	if (int_stat & USBH_ATSAMX_INT_RST) {
		LOG_DBG("bus reset");
		/* Clear BUS RESET interrupt flag*/
		p_reg->int_flag = USBH_ATSAMX_INT_RST;
		/* Bit may be set due to SetPortFeature(PORT_RESET)*/
		p_drv_data->rh_port_stat |= USBH_HUB_STATUS_PORT_EN;
		/* Transitioned from RESET state to ENABLED state*/
		p_drv_data->rh_port_chng |= USBH_HUB_STATUS_C_PORT_RESET;
		/* Notify the core layer.*/
		usbh_rh_event(p_hc_drv->rh_dev_ptr);
	} else if (int_stat & USBH_ATSAMX_INT_DDISC) {
		LOG_DBG("disconnect usb");

		/* Clear device disconnect/connect interrupt flags*/
		p_reg->int_flag =
			(USBH_ATSAMX_INT_DDISC | USBH_ATSAMX_INT_DCONN);
		/* Disable disconnect/wakeup interrupt*/
		p_reg->int_en_clr =
			(USBH_ATSAMX_INT_DDISC | USBH_ATSAMX_INT_WAKEUP);

		/* Clear device connect/wakeup interrupt flags*/
		p_reg->int_flag =
			(USBH_ATSAMX_INT_DCONN | USBH_ATSAMX_INT_WAKEUP);
		/* Enable connect/wakeup interrupt*/
		p_reg->int_en_set =
			(USBH_ATSAMX_INT_DCONN | USBH_ATSAMX_INT_WAKEUP);
		/* Clear Root hub Port Status bits*/
		p_drv_data->rh_port_stat = 0;
		/* Current connect status has changed*/
		p_drv_data->rh_port_chng |= USBH_HUB_STATUS_C_PORT_CONN;
		/* Notify the core layer.*/
		usbh_rh_event(p_hc_drv->rh_dev_ptr);
	} else if (int_stat & USBH_ATSAMX_INT_DCONN) {
		LOG_DBG("connect usb");
		/* Disable device connect interrupt*/
		p_reg->int_en_clr = USBH_ATSAMX_INT_DCONN;
		/* Bit reflects if a device is currently connected*/
		p_drv_data->rh_port_stat |= USBH_HUB_STATUS_PORT_CONN;
		/* Bit indicates a Port's current connect status change */
		p_drv_data->rh_port_chng |= USBH_HUB_STATUS_C_PORT_CONN;
		/* Clear  disconnection interrupt*/
		p_reg->int_flag = USBH_ATSAMX_INT_DDISC;
		/* Enable disconnection interrupt*/
		p_reg->int_en_set = USBH_ATSAMX_INT_DDISC;
		/* Notify the core layer.*/
		usbh_rh_event(p_hc_drv->rh_dev_ptr);
	}
	/*  WAKE UP TO POWER  */
	if (int_stat & (USBH_ATSAMX_INT_WAKEUP | USBH_ATSAMX_INT_DCONN)) {
		LOG_DBG("Wake up to power");
		/* Clear WAKEUP interrupt flag*/
		p_reg->int_flag = USBH_ATSAMX_INT_WAKEUP;
	}

	/*  RESUME  */
	if (int_stat & (USBH_ATSAMX_INT_WAKEUP | USBH_ATSAMX_INT_UPRSM |
			USBH_ATSAMX_INT_DNRSM)) {
		LOG_DBG("resume\n");
		/* Clear interrupt flag*/
		p_reg->int_flag =
			(USBH_ATSAMX_INT_WAKEUP | USBH_ATSAMX_INT_UPRSM |
			 USBH_ATSAMX_INT_DNRSM);
		/* Disable interrupts*/
		p_reg->int_en_clr =
			(USBH_ATSAMX_INT_WAKEUP | USBH_ATSAMX_INT_UPRSM |
			 USBH_ATSAMX_INT_DNRSM);
		p_reg->int_en_set =
			(USBH_ATSAMX_INT_RST | USBH_ATSAMX_INT_DDISC);
		/* enable start of frames */
		p_reg->ctrl_b |= USBH_ATSAMX_CTRLB_SOFE;
	}

	/*  HANDLE PIPE INTERRUPT status  */
	/* Read Pipe interrupt summary*/
	pipe_stat = p_reg->p_int_smry;
	while (pipe_stat != 0) {
		/* Check if there is a pipe to handle*/
		pipe_nbr = u32_count_trailing_zeros(pipe_stat);
		int_stat = p_reg->h_pipe[pipe_nbr].p_int_flag;
		int_stat &= p_reg->h_pipe[pipe_nbr].p_int_set;
		p_urb = p_drv_data->pipe_tbl[pipe_nbr].urb_ptr;
		xfer_len = USBH_ATSAMX_GET_BYTE_CNT(
			p_drv_data->desc_tbl[pipe_nbr].desc_bank[0].pck_size);

		if (int_stat & USBH_ATSAMX_PINT_STALL) {
			LOG_DBG("stall");
			/* Stop transfer*/
			p_reg->h_pipe[pipe_nbr].p_status_set =
				USBH_ATSAMX_PSTATUS_PFREEZE;
			/* Clear Stall interrupt flag*/
			p_reg->h_pipe[pipe_nbr].p_int_flag =
				USBH_ATSAMX_PINT_STALL;
			p_urb->err = EBUSY;
			/* Notify the Core layer about the urb completion*/
			usbh_urb_done(p_urb);
		}

		if (int_stat & USBH_ATSAMX_PINT_PERR) {
			LOG_DBG("pipe error interrupt");
			/* Stop transfer*/
			p_reg->h_pipe[pipe_nbr].p_status_set =
				USBH_ATSAMX_PSTATUS_PFREEZE;
			/* Clear Pipe error interrupt flag*/
			p_reg->h_pipe[pipe_nbr].p_int_flag =
				USBH_ATSAMX_PINT_PERR;
			p_urb->err = EIO;
			/* Notify the Core layer about the urb completion*/
			usbh_urb_done(p_urb);
		}

		if (int_stat & USBH_ATSAMX_PINT_TXSTP) {
			/*  HANDLE SETUP PACKETS  */
			LOG_DBG("handle setup packets");
			/* Disable Setup transfer interrupt*/
			p_reg->h_pipe[pipe_nbr].p_int_clr =
				USBH_ATSAMX_PINT_TXSTP;
			/* Clear   Setup transfer flag*/
			p_reg->h_pipe[pipe_nbr].p_int_flag =
				USBH_ATSAMX_PINT_TXSTP;

			xfer_len = p_drv_data->pipe_tbl[pipe_nbr].appbuf_len +
				   p_urb->xfer_len;
			p_urb->err = 0;
			/* Notify the Core layer about the urb completion*/
			usbh_urb_done(p_urb);
		}

		if (int_stat & USBH_ATSAMX_PINT_TRCPT) {
			/* Disable transfer complete interrupt*/
			p_reg->h_pipe[pipe_nbr].p_int_clr =
				USBH_ATSAMX_PINT_TRCPT;
			/* Clear   transfer complete flag*/
			p_reg->h_pipe[pipe_nbr].p_int_flag =
				USBH_ATSAMX_PINT_TRCPT;

			/* Keep track of PID DATA toggle*/
			p_urb->ep_ptr->data_pid = USBH_ATSAMX_GET_DPID(
				p_reg->h_pipe[pipe_nbr].p_status);

			if (p_urb->token == USBH_TOKEN_IN) {
				/* IN PACKETS HANDLER  */
				LOG_DBG("in packets handler");
				max_pkt_size =
					usbh_ep_max_pkt_size_get(p_urb->ep_ptr);
				p_drv_data->pipe_tbl[pipe_nbr].appbuf_len +=
					xfer_len;

				if ((p_drv_data->pipe_tbl[pipe_nbr].appbuf_len
				     == p_urb->uberbuf_len) ||
				    (xfer_len < max_pkt_size)) {
					p_urb->err = 0;
					/*
					 * Notify the Core layer about
					 * the urb completion
					 */
					usbh_urb_done(p_urb);
				} else {
					p_urb->err =
						k_msgq_put(&atsamx_urb_proc_q,
							   (void *)p_urb,
							   K_NO_WAIT);
					if (p_urb->err != 0) {
						/*
						 * Notify the Core layer about
						 * the urb completion
						 */
						usbh_urb_done(p_urb);
					}
				}
			} else {
				/*  OUT PACKETS HANDLER  */
				LOG_DBG("out packets handler");
				xfer_len = p_drv_data->pipe_tbl[pipe_nbr]
					   .appbuf_len +
					   p_urb->xfer_len;

				if (xfer_len == p_urb->uberbuf_len) {
					p_urb->err = 0;
					/*
					 * Notify the Core layer about
					 * the urb completion
					 */
					usbh_urb_done(p_urb);
				} else {
					p_urb->err =
						k_msgq_put(&atsamx_urb_proc_q,
							   (void *)p_urb,
							   K_NO_WAIT);
					if (p_urb->err != 0) {
						/*
						 * Notify the Core layer about
						 * the urb completion
						 */
						usbh_urb_done(p_urb);
					}
				}
			}
		}

		WRITE_BIT(pipe_stat, pipe_nbr, 0);
	}
}

/*
 * The task handles additional EP IN/OUT transactions when needed.
 */
static void usbh_atsamx_process_urb(void *p_arg, void *p_arg2, void *p_arg3)
{
	struct usbh_hc_drv *p_hc_drv;
	struct usbh_drv_data *p_drv_data;
	struct usbh_atsamx_reg *p_reg;
	struct usbh_urb *p_urb = NULL;
	uint32_t xfer_len;
	uint8_t pipe_nbr;
	int p_err;
	int key;

	p_hc_drv = (struct usbh_hc_drv *)p_arg;
	p_drv_data = (struct usbh_drv_data *)p_hc_drv->data_ptr;
	p_reg = (struct usbh_atsamx_reg *)DT_INST_REG_ADDR(0);

	while (true) {
		p_err = k_msgq_get(&atsamx_urb_proc_q, (void *)p_urb,
				   K_FOREVER);
		if (p_err != 0) {
			LOG_ERR("Cannot get USB urb");
		}

		pipe_nbr = usbh_atsamx_get_pipe_nbr(p_drv_data, p_urb->ep_ptr);

		if (pipe_nbr != ATSAMX_INVALID_PIPE) {
			key = irq_lock();
			xfer_len = USBH_ATSAMX_GET_BYTE_CNT(
				p_drv_data->desc_tbl[pipe_nbr]
				.desc_bank[0]
				.pck_size);

			if (p_urb->token == USBH_TOKEN_IN) {
				/*  HANDLE IN TRANSACTIONS  */
				memcpy((void *)((uint32_t)p_urb->userbuf_ptr +
						p_urb->xfer_len),
				       p_urb->dma_buf_ptr, xfer_len);
				/*
				 * Check if it rx'd more
				 * data than what was expected
				 */
				if (xfer_len > p_drv_data->pipe_tbl[pipe_nbr]
				    .next_xfer_len) {
					/*
					 * Rx'd more data than
					 * what was expected
					 */
					p_urb->xfer_len +=
						p_drv_data->pipe_tbl[pipe_nbr]
						.next_xfer_len;
					p_urb->err = EIO;

					LOG_ERR("Rx'd more data than \
						was expected.");
					/*
					 * Notify the Core layer
					 * about the urb completion
					 */
					usbh_urb_done(p_urb);
				} else {
					p_urb->xfer_len += xfer_len;
				}
			} else {
				/*  HANDLE OUT TRANSACTIONS  */
				xfer_len = (p_drv_data->pipe_tbl[pipe_nbr]
					    .appbuf_len -
					    xfer_len);
				if (xfer_len == 0) {
					p_urb->xfer_len +=
						p_drv_data->pipe_tbl[pipe_nbr]
						.next_xfer_len;
				} else {
					p_urb->xfer_len +=
						(p_drv_data->pipe_tbl[pipe_nbr]
						 .next_xfer_len -
						 xfer_len);
				}
			}

			if (p_urb->err == 0) {
				usbh_atsamx_pipe_cfg(
					p_urb, &p_reg->h_pipe[pipe_nbr],
					&p_drv_data->pipe_tbl[pipe_nbr],
					&p_drv_data->desc_tbl[pipe_nbr]
					.desc_bank[0]);
				/* Enable transfer complete interrupt */
				p_reg->h_pipe[pipe_nbr].p_int_set =
					USBH_ATSAMX_PINT_TRCPT;
				/* Start transfer*/
				p_reg->h_pipe[pipe_nbr].p_status_clr =
					USBH_ATSAMX_PSTATUS_PFREEZE;
			}
			irq_unlock(key);
		}
	}
}

/*
 * initialize pipe configurations based on the endpoint
 *  direction and characteristics.
 */
static void usbh_atsamx_pipe_cfg(struct usbh_urb *p_urb,
				 struct usbh_atsamx_pipe_reg *p_reg_hpipe,
				 struct usbh_atsamx_pinfo *p_pipe_info,
				 struct usbh_atsamx_desc_bank *p_desc_bank)
{
	uint8_t ep_nbr;
	uint8_t reg_val;
	uint16_t max_pkt_size;
	int key;

	max_pkt_size = usbh_ep_max_pkt_size_get(p_urb->ep_ptr);
	ep_nbr = usbh_ep_log_nbr_get(p_urb->ep_ptr);
	p_pipe_info->next_xfer_len = p_urb->uberbuf_len - p_urb->xfer_len;
	p_pipe_info->next_xfer_len =
		MIN(p_pipe_info->next_xfer_len, DT_INST_PROP(0, buf_len));

	memset(p_urb->dma_buf_ptr, 0, p_pipe_info->next_xfer_len);
	if (p_urb->token != USBH_TOKEN_IN) {
		/*  SETUP/OUT PACKETS  */
		p_pipe_info->appbuf_len = p_pipe_info->next_xfer_len;

		memcpy(p_urb->dma_buf_ptr,
		       (void *)((uint32_t)p_urb->userbuf_ptr + p_urb->xfer_len),
		       p_pipe_info->next_xfer_len);

		p_desc_bank->pck_size = p_pipe_info->appbuf_len;
	} else {
		/*  IN PACKETS  */
		p_desc_bank->pck_size = (p_pipe_info->next_xfer_len << 14u);
	}

	p_desc_bank->addr = (uint32_t)p_urb->dma_buf_ptr;
	p_desc_bank->pck_size |=
		(u32_count_trailing_zeros(max_pkt_size >> 3u) << 28u);
	p_desc_bank->ctrl_pipe = (p_urb->ep_ptr->dev_addr | (ep_nbr << 8));

	if (p_urb->token != USBH_TOKEN_IN) {
		/*  SETUP/OUT PACKETS  */
		LOG_DBG("setup out packets");
		key = irq_lock();
		reg_val = p_reg_hpipe->p_cfg;
		reg_val &= ~USBH_ATSAMX_PCFG_PTOKEN_MSK;
		reg_val |= (p_urb->token ? USBH_ATSAMX_PCFG_PTOKEN_OUT :
			    USBH_ATSAMX_PCFG_PTOKEN_SETUP);
		p_reg_hpipe->p_cfg = reg_val;
		irq_unlock(key);
		/* Set Bank0 ready : Pipe can send data to device*/
		p_reg_hpipe->p_status_set = USBH_ATSAMX_PSTATUS_BK0RDY;
	} else {
		/*  IN PACKETS  */
		LOG_DBG("setup in packets");
		key = irq_lock();
		reg_val = p_reg_hpipe->p_cfg;
		reg_val &= ~USBH_ATSAMX_PCFG_PTOKEN_MSK;
		reg_val |= USBH_ATSAMX_PCFG_PTOKEN_IN;
		p_reg_hpipe->p_cfg = reg_val;
		irq_unlock(key);
		/* Clear Bank0 ready: Pipe can receive data from device */
		p_reg_hpipe->p_status_clr = USBH_ATSAMX_PSTATUS_BK0RDY;
	}
}

/*
 * Allocate a free host pipe number for the newly opened pipe.
 */

static uint8_t usbh_atsamx_get_free_pipe(struct usbh_drv_data *p_drv_data)
{
	uint8_t pipe_nbr;

	for (pipe_nbr = 0; pipe_nbr < ATSAMX_MAX_NBR_PIPE; pipe_nbr++) {
		if (DEF_BIT_IS_CLR(p_drv_data->pipe_used, BIT(pipe_nbr))) {
			WRITE_BIT(p_drv_data->pipe_used, pipe_nbr, 1);
			return pipe_nbr;
		}
	}
	return ATSAMX_INVALID_PIPE;
}

/*
 * Get the host pipe number corresponding to
 * a given device address, endpoint number and direction.
 */
static uint8_t usbh_atsamx_get_pipe_nbr(struct usbh_drv_data *p_drv_data,
					struct usbh_ep *p_ep)
{
	uint8_t pipe_nbr;
	uint16_t ep_addr;

	ep_addr = ((p_ep->dev_addr << 8) | p_ep->desc.b_endpoint_address);

	for (pipe_nbr = 0; pipe_nbr < ATSAMX_MAX_NBR_PIPE; pipe_nbr++) {
		if (p_drv_data->pipe_tbl[pipe_nbr].ep_addr == ep_addr) {
			return pipe_nbr;
		}
	}
	return ATSAMX_INVALID_PIPE;
}
