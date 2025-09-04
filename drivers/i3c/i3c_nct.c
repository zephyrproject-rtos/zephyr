/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct_i3c

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/pinctrl.h>
#include <stdlib.h>
#include "i3c_nct.h"

#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nct_i3c, CONFIG_I3C_LOG_LEVEL);

/* I3C properties */
#define I3C_CHK_TIMEOUT_US       10000 /* Timeout for checking register status */
#define I3C_CLK_FREQ_48_MHZ      MHZ(48)
#define I3C_CLK_FREQ_96_MHZ      MHZ(96)
#define I3C_SCL_PP_FREQ_MAX_MHZ 12500000
#define I3C_SCL_OD_FREQ_MAX_MHZ 4170000
#define I3C_BUS_TLOW_PP_MIN_NS  24  /* T_LOW period in push-pull mode */
#define I3C_BUS_TLOW_OD_MIN_NS  200 /* T_LOW period in open-drain mode */
#define I3C_TGT_WR_REQ_WAIT_US   10  /* I3C target write request PDMA completion after stop */
#define I3C_FIFO_SIZE            16
#define PPBAUD_DIV_MAX           0xF
#define I2CBAUD_DIV_MAX          0xF
#define DAA_TGT_INFO_SZ 0x8 /* 8 bytes = PID(6) + BCR(1) + DCR(1) */
#define I3C_TRANS_TIMEOUT_MS     K_MSEC(1000) /*  Default maximum allow time for an I3C transfer */
#define I3C_IBI_MAX_PAYLOAD_SIZE 32
#define I3C_STATUS_CLR_MASK                                                 \
	(BIT(NCT_I3C_MSTATUS_TGTSTART) | BIT(NCT_I3C_MSTATUS_MCTRLDONE) | BIT(NCT_I3C_MSTATUS_COMPLETE) |      \
	 BIT(NCT_I3C_MSTATUS_IBIWON) | BIT(NCT_I3C_MSTATUS_NOWCNTLR))

#define I3C_TGT_INTSET_MASK                                                                        \
	(BIT(NCT_I3C_INTSET_START) | BIT(NCT_I3C_INTSET_MATCHED) | BIT(NCT_I3C_INTSET_STOP) |                  \
	 BIT(NCT_I3C_INTSET_DACHG) | BIT(NCT_I3C_INTSET_CCC) | BIT(NCT_I3C_INTSET_ERRWARN) |                   \
	 BIT(NCT_I3C_INTSET_DDRMATCHED) | BIT(NCT_I3C_INTSET_CHANDLED) | BIT(NCT_I3C_INTSET_EVENT))

/* Register access helper */

/* Driver convenience defines */
#define HAL_INSTANCE(dev) ((struct i3c_reg *)((const struct nct_i3c_config *)(dev)->config)->base)
#define NCT_PCC_NODE     DT_NODELABEL(pcc)

/* I3C hardware index parsing */
#define I3C_NCT_HW_IDX(n) ((n & 0xFFF) >> 9)

/* I3C target PID parsing */
#define GET_PID_VENDOR_ID(pid) (((uint64_t)pid >> 33) & 0x7fff) /* PID[47:33] */
#define GET_PID_ID_TYP(pid)    (((uint64_t)pid >> 32) & 0x1)    /* PID[32] */
#define GET_PID_PARTNO(pid)    (pid & 0xffffffff)               /* PID[31:0] */
#ifdef CONFIG_I3C_NCT_DMA
/* PDMA mux ID parsing */
#define I3C_NCT_PDMA_MUX_ID(n, rnw)                                                               \
	(rnw ? ((((n & 0xFFF) >> 9) * 2) + 5) : ((((n & 0xFFF) >> 9) * 2) + 6))
#endif

/* PDMA channel parsing */
#define NCT_PDMA_BASE(n)         (n & 0xFFFFFF00)
#define NCT_PDMA_DSCT_IDX(n)     ((n - (n & 0xFFFFFF00)) >> 4)
#define NCT_PDMA_CHANNEL_PER_REQ 0x4

/* Supported I3C clock frequency */
enum nct_i3c_clk_speed {
	I3C_CLK_FREQ_48MHZ,
	I3C_CLK_FREQ_96MHZ,
};

static const struct nct_i3c_timing_cfg nct_def_speed_cfg[] = {
		/*
	 * Recommended I3C timing values are based on I3C frequency 48 or 96 MHz
	 * PP = 12.5 mhz, OD = 4.17 Mhz, i2c = 1.0 Mhz
		 */
	[I3C_CLK_FREQ_48MHZ] = {.ppbaud = 1, .pplow = 0, .odhpp = 1, .odbaud = 4, .i2c_baud = 3},
	[I3C_CLK_FREQ_96MHZ] = {.ppbaud = 3, .pplow = 0, .odhpp = 1, .odbaud = 4, .i2c_baud = 3},
};

//============================================================
void init_i3c_slave_rx_payload(const struct device *dev)
{
	struct nct_i3c_data *data = dev->data;
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(data->pdma_rx_buf); i++) {
		data->slave_rx_payload[i].buf = &data->pdma_rx_buf[i][0];
		data->slave_rx_payload[i].size = sizeof(data->pdma_rx_buf[i]);
	}

	data->rx_payload_curr = &data->slave_rx_payload[0];
	data->rx_payload_in = 0;
	data->rx_payload_out = 0;
}

struct i3c_slave_payload *alloc_i3c_slave_rx_payload(const struct device *dev)
{
	struct nct_i3c_data *data = dev->data;

	return data->rx_payload_curr;
}

void update_i3c_slave_rx_payload(const struct device *dev)
{
	struct nct_i3c_data *data = dev->data;

	data->rx_payload_in = (data->rx_payload_in + 1) % ARRAY_SIZE(data->pdma_rx_buf);
	data->rx_payload_curr = &data->slave_rx_payload[data->rx_payload_in];

	/* if queue full, skip the oldest un-read message */
	if (data->rx_payload_in == data->rx_payload_out) {
		data->rx_payload_out = (data->rx_payload_out + 1) % ARRAY_SIZE(data->pdma_rx_buf);
	}
}

//==========================================================
static K_SEM_DEFINE(tx_fifo_empty_sem, 0, 1);

// used by ap layer to wait for tx fifo empty
int target_wait_for_tx_fifo_empty(k_timeout_t timeout) {
    if (k_sem_take(&tx_fifo_empty_sem, timeout) == 0) {
        return 0;
    } else {
        return -ETIMEDOUT;
    }
}

// used in i3c target isr to release semaphore
static void tx_fifo_empty_handler(void) {
    k_sem_give(&tx_fifo_empty_sem);
}

typedef void (*tx_fifo_empty_cb_t)(void);
static tx_fifo_empty_cb_t tx_fifo_empty_cb = NULL;

void target_register_tx_fifo_empty_cb(tx_fifo_empty_cb_t cb) {
    tx_fifo_empty_cb = cb;
}
//============================================================

/* I3C functions */
static void nct_i3c_mutex_lock(const struct device *dev)
{
	struct nct_i3c_data *const data = dev->data;

	k_mutex_lock(&data->lock_mutex, K_FOREVER);
}

static void nct_i3c_mutex_unlock(const struct device *dev)
{
	struct nct_i3c_data *const data = dev->data;

	k_mutex_unlock(&data->lock_mutex);
}

static void nct_i3c_reset_module(const struct device *dev)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct pmc_reg *pmc = (struct pmc_reg *)DT_REG_ADDR_BY_NAME(NCT_PCC_NODE, pmc);
	uint8_t index;

	index = I3C_NCT_HW_IDX((uint32_t)i3c_inst);

	/* Reset i3c module, write 1 to the bit, then write 0 */
	pmc->SW_RST1 |= BIT(index);
	/* Require one NOP instruction cycle time */
	arch_nop();
	pmc->SW_RST1 &= ~BIT(index);
}

/*
 * brief:  Wait for status bit done and clear the status
 *
 * param[in] i3c_inst  Pointer to I3C register.
 *
 * return  0, success
 *         -ETIMEDOUT: check status timeout.
 */
static inline int nct_i3c_status_wait_clear(struct i3c_reg *i3c_inst, uint32_t bit_val)
{
	if (WAIT_FOR(i3c_inst->MSTATUS & bit_val, I3C_CHK_TIMEOUT_US, NULL) == false) {
		return -ETIMEDOUT;
	}

	i3c_inst->MSTATUS = bit_val; /* W1C */

	return 0;
}

static inline uint32_t nct_i3c_state_get(struct i3c_reg *i3c_inst)
{
	return GET_FIELD(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_STATE);
}

static inline void nct_i3c_interrupt_all_disable(struct i3c_reg *i3c_inst)
{
	uint32_t intmask = i3c_inst->MINTSET;

	i3c_inst->MINTCLR = intmask;
}

static inline void nct_i3c_interrupt_enable(struct i3c_reg *i3c_inst, uint32_t mask)
{
	i3c_inst->MINTSET = mask;
}

static void nct_i3c_enable_target_interrupt(const struct device *dev, bool enable)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);

	/* Disable the target interrupt events */
	i3c_inst->INTCLR = i3c_inst->INTSET;

	/* Clear the target interrupt status */
	i3c_inst->STATUS = i3c_inst->STATUS;

	/* Enable the target interrupt events */
	if (enable) {
		i3c_inst->INTSET = I3C_TGT_INTSET_MASK;
		i3c_inst->MINTSET |= BIT(NCT_I3C_MINTSET_NOWMASTER); /* I3C target is now controller */

#ifndef CONFIG_I3C_NCT_DMA
		/* Receive buffer pending (FIFO mode) */
		i3c_inst->INTSET |= BIT(NCT_I3C_INTSET_RXPEND);
#endif
	}
}

static inline void nct_i3c_target_rx_fifo_flush(struct i3c_reg *i3c_inst)
{
	i3c_inst->DATACTRL |= BIT(NCT_I3C_DATACTRL_FLUSHFB);
}

static inline void nct_i3c_target_tx_fifo_flush(struct i3c_reg *i3c_inst)
{
	i3c_inst->DATACTRL |= BIT(NCT_I3C_DATACTRL_FLUSHTB);
}

static bool nct_i3c_has_error(struct i3c_reg *i3c_inst)
{
	if (IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_ERRWARN)) {
		if (i3c_inst->MERRWARN == BIT(NCT_I3C_MERRWARN_TIMEOUT)) {
			LOG_DBG("Timeout error, MERRWARN 0x%08x", i3c_inst->MERRWARN);
			i3c_inst->MERRWARN = BIT(NCT_I3C_MERRWARN_TIMEOUT);
			return false;
		}

		LOG_WRN("ERROR: MSTATUS 0x%08x MERRWARN 0x%08x", i3c_inst->MSTATUS,
							i3c_inst->MERRWARN);
		return true;
	}

	return false;
}

static inline void nct_i3c_status_clear_all(struct i3c_reg *i3c_inst)
{
	uint32_t mask = I3C_STATUS_CLR_MASK;

	// don't clear SLVSTART
	if (IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_TGTSTART)) {
		mask &= ~BIT(NCT_I3C_MSTATUS_TGTSTART);		
	}

	i3c_inst->MSTATUS = mask;
}

static inline void nct_i3c_errwarn_clear_all(struct i3c_reg *i3c_inst)
{
	if (i3c_inst->MERRWARN) {
	i3c_inst->MERRWARN = i3c_inst->MERRWARN;
}
}

static inline void nct_i3c_controller_fifo_flush(struct i3c_reg *i3c_inst)
{
	i3c_inst->MDATACTRL |= (BIT(NCT_I3C_MDATACTRL_FLUSHTB) | BIT(NCT_I3C_MDATACTRL_FLUSHFB));
}

/*
 * brief:  Send request and check the request is valid
 *
 * param[in] i3c_inst  Pointer to I3C register.
 *
 * return  0, success
 *         -ETIMEDOUT check MCTRLDONE timeout.
 *         -ENOSYS    invalid use of request.
 */
static inline int nct_i3c_send_request(struct i3c_reg *i3c_inst, uint32_t mctrl_val)
{
	i3c_inst->MCTRL = mctrl_val;

	if (nct_i3c_status_wait_clear(i3c_inst, BIT(NCT_I3C_MSTATUS_MCTRLDONE)) != 0) {
		return -ETIMEDOUT;
	}

	/* Check invalid use of request */
	if (IS_BIT_SET(i3c_inst->MERRWARN, NCT_I3C_MERRWARN_INVREQ)) {
		LOG_ERR("Invalid request, merrwarn: %#x", i3c_inst->MERRWARN);
		return -ENOSYS;
	}

	return 0;
}

/* Start DAA procedure and continue the DAA with a Repeated START */
static inline int nct_i3c_request_daa(struct i3c_reg *i3c_inst)
{
	uint32_t val = 0;
	int ret;

	/* Set IBI response NACK while processing DAA */
	SET_FIELD(val, NCT_I3C_MCTRL_IBIRESP, MCTRL_IBIRESP_NACK);

	/* Send DAA request */
	SET_FIELD(val, NCT_I3C_MCTRL_REQUEST, MCTRL_REQUEST_PROCESSDAA);

	ret = nct_i3c_send_request(i3c_inst, val);
	if (ret != 0) {
		LOG_ERR("Request DAA error, %d", ret);
		return ret;
	}

	return 0;
}

/* Tell controller to start auto IBI */
static inline int nct_i3c_request_auto_ibi(struct i3c_reg *i3c_inst)
{
	uint32_t val = 0;
	int ret;

	SET_FIELD(val, NCT_I3C_MCTRL_IBIRESP, MCTRL_IBIRESP_ACK);
	SET_FIELD(val, NCT_I3C_MCTRL_REQUEST, MCTRL_REQUEST_AUTOIBI);

	ret = nct_i3c_send_request(i3c_inst, val);
	if (ret != 0) {
		LOG_ERR("Request auto ibi error, %d", ret);
		return ret;
	}

	return 0;
}

/*
 * brief:  Controller emit start and send address
 *
 * param[in] i3c_inst     Pointer to I3C register.
 * param[in] addr     Dyamic address for xfer or 0x7E for CCC command.
 * param[in] op_type  Request type.
 * param[in] is_rx    true=rx or false=tx operation.
 * param[in] read_sz  Read size.
 *
 * return  0, success
 *         else, error
 */
static int nct_i3c_request_emit_start(struct i3c_reg *i3c_inst, uint8_t addr,
				       enum nct_i3c_mctrl_type op_type, bool is_rx, size_t read_sz)
{
	uint32_t mctrl = 0;
	int ret;

	/* Set request and target address*/
	SET_FIELD(mctrl, NCT_I3C_MCTRL_REQUEST, MCTRL_REQUEST_EMITSTARTADDR);

	/* Set operation type */
	SET_FIELD(mctrl, NCT_I3C_MCTRL_TYPE, op_type);

	/* Set IBI response NACK in emit start */
	SET_FIELD(mctrl, NCT_I3C_MCTRL_IBIRESP, MCTRL_IBIRESP_NACK);

	/* Set dynamic address */
	SET_FIELD(mctrl, NCT_I3C_MCTRL_ADDR, addr);

	/* Set rx or tx */
	if (is_rx) {
		mctrl |= BIT(NCT_I3C_MCTRL_DIR);
		if (read_sz <= 255) {
		SET_FIELD(mctrl, NCT_I3C_MCTRL_RDTERM, read_sz); /* Set read length */
		}
	} else {
		mctrl &= ~BIT(NCT_I3C_MCTRL_DIR);
	}

	ret = nct_i3c_send_request(i3c_inst, mctrl);
	if (ret != 0) {
		LOG_ERR("Request start error, %d", ret);
		return ret;
	}

	/* Check NACK after MCTRLDONE is get */
	if (IS_BIT_SET(i3c_inst->MERRWARN, NCT_I3C_MERRWARN_NACK)) {
		LOG_DBG("NACK");
		return -ENODEV;
	}

	return 0;
}

/*
 * brief:  Controller emit STOP.
 *
 * This emits STOP when controller is in NORMACT state.
 *
 * param[in] i3c_inst  Pointer to I3C register.
 *
 * return  0 success
 *         -ECANCELED i3c state not as expected.
 *         -ETIMEDOUT check MCTRLDONE timeout.
 *         -ENOSYS    invalid use of request.
 */
static inline int nct_i3c_request_emit_stop(struct i3c_reg *i3c_inst)
{
	uint32_t val = 0;
	int ret;
	uint32_t i3c_state = nct_i3c_state_get(i3c_inst);

	/* Make sure we are in a state where we can emit STOP */
	if (i3c_state == MSTATUS_STATE_IDLE ||
	    i3c_state == MSTATUS_STATE_TGTREQ) {
		LOG_ERR("Request stop state error, state= %#x", i3c_state);
		return -ECANCELED;
	}

	SET_FIELD(val, NCT_I3C_MCTRL_REQUEST, MCTRL_REQUEST_EMITSTOP);

	ret = nct_i3c_send_request(i3c_inst, val);
	if (ret != 0) {
		LOG_ERR("Request stop error, %d", ret);
		return ret;
	}

	return 0;
}

static inline int nct_i3c_ibi_respond_nack(struct i3c_reg *i3c_inst)
{
	uint32_t val = 0;
	int ret;

	SET_FIELD(val, NCT_I3C_MCTRL_IBIRESP, MCTRL_IBIRESP_NACK);
	SET_FIELD(val, NCT_I3C_MCTRL_REQUEST, MCTRL_REQUEST_IBIACKNACK);

	ret = nct_i3c_send_request(i3c_inst, val);
	if (ret != 0) {
		LOG_ERR("Request ibi_rsp nack error, %d", ret);
		return ret;
	}

	return 0;
}

static inline int nct_i3c_ibi_respond_ack(struct i3c_reg *i3c_inst)
{
	uint32_t val = 0;
	int ret;

	SET_FIELD(val, NCT_I3C_MCTRL_IBIRESP, MCTRL_IBIRESP_ACK);
	SET_FIELD(val, NCT_I3C_MCTRL_REQUEST, MCTRL_REQUEST_IBIACKNACK);

	ret = nct_i3c_send_request(i3c_inst, val);
	if (ret != 0) {
		LOG_ERR("Request ibi_rsp ack error %d", ret);
		return ret;
	}

	return 0;
}

/*
 * brief:  Find a registered I3C target device.
 *
 * This returns the I3C device descriptor of the I3C device
 * matching the incoming id.
 *
 * param[in] dev  Pointer to controller device driver instance.
 * param[in] id   Pointer to I3C device ID.
 *
 * return  see i3c_device_find.
 */
static inline struct i3c_device_desc *nct_i3c_device_find(const struct device *dev,
							   const struct i3c_device_id *id)
{
	const struct nct_i3c_config *config = dev->config;

	return i3c_dev_list_find(&config->common.dev_list, id);
}

/*
 * brief:  Perform bus recovery.
 *
 * param[in] dev  Pointer to controller device driver instance.
 *
 * return 0 success, otherwise error
 */
static int nct_i3c_recover_bus(const struct device *dev)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);

	/*
	 * If the controller is in NORMACT state, tells it to emit STOP
	 * so it can return to IDLE, or is ready to clear any pending
	 * target initiated IBIs.
	 */
	if (nct_i3c_state_get(i3c_inst) == MSTATUS_STATE_NORMACT) {
		nct_i3c_request_emit_stop(i3c_inst);
	};

	/* Exhaust all target initiated IBI */
	while (IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_TGTSTART)) {
		/* Tell the controller to perform auto IBI. */
		nct_i3c_request_auto_ibi(i3c_inst);

		if (WAIT_FOR(IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_COMPLETE), I3C_CHK_TIMEOUT_US,
			     NULL) == false) {
			break;
		}

		/* Once auto IBI is done, discard bytes in FIFO. */
		while (IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_RXPEND)) {
			/* Flush FIFO as long as RXPEND is set. */
			nct_i3c_controller_fifo_flush(i3c_inst);
		}

		/* Emit stop */
		nct_i3c_request_emit_stop(i3c_inst);

		/*
		 * There might be other IBIs waiting.
		 * So pause a bit to let other targets initiates
		 * their IBIs.
		 */
		k_busy_wait(100);
	}

	/* Check IDLE state */
	if (WAIT_FOR((nct_i3c_state_get(i3c_inst) == MSTATUS_STATE_IDLE),
		     I3C_CHK_TIMEOUT_US, NULL) == false) {
		return -EBUSY;
	}

	return 0;
}

static inline void nct_i3c_xfer_reset(struct i3c_reg *i3c_inst)
{
	nct_i3c_status_clear_all(i3c_inst);
	nct_i3c_errwarn_clear_all(i3c_inst);
	nct_i3c_controller_fifo_flush(i3c_inst);
}

/*
 * brief:  Perform one write transaction.
 *
 * This writes all data in buf to TX FIFO or time out
 * waiting for FIFO spaces.
 *
 * param[in] i3c_inst   Pointer to controller registers.
 * param[in] buf        Buffer containing data to be sent.
 * param[in] buf_sz     Number of bytes in buf to send.
 * param[in] no_ending  True if not to signal end of write message.
 *
 * return  Number of bytes written, or negative if error.
 *
 */
static int nct_i3c_xfer_write_fifo(struct i3c_reg *i3c_inst, uint8_t *buf, uint8_t buf_sz,
				    bool no_ending)
{
	int offset = 0;
	int remaining = buf_sz;

	while (remaining > 0) {
		/* Check tx fifo not full */
		if (WAIT_FOR(!(IS_BIT_SET(i3c_inst->MDATACTRL, NCT_I3C_MDATACTRL_TXFULL)), 
				I3C_CHK_TIMEOUT_US, NULL) == false) {
			LOG_DBG("Check tx fifo not full timed out");
			return -ETIMEDOUT;
		}

		if ((remaining > 1) || no_ending) {
			i3c_inst->MWDATAB = (uint32_t)buf[offset];
		} else {
			i3c_inst->MWDATABE = (uint32_t)buf[offset]; /* Set last byte */
		}

		offset += 1;
		remaining -= 1;
	}

	return offset;
}

/*
 * brief:  Perform read transaction.
 *
 * This reads from RX FIFO until COMPLETE bit is set in MSTATUS
 * or time out.
 *
 * param[in] i3c_inst  Pointer to controller registers.
 * param[in] buf       Buffer to store data.
 * param[in] buf_sz    Number of bytes to read.
 *
 * return  Number of bytes read, or negative if error.
 *
 */
static int nct_i3c_xfer_read_fifo(struct i3c_reg *i3c_inst, uint8_t *buf, uint8_t rd_sz)
{
	bool is_done = false;
	int offset = 0;

	while (is_done == false) {
		/* Check message is terminated */
		if (IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_COMPLETE)) {
			is_done = true;
		}

		/* Check I3C bus error */
		if (nct_i3c_has_error(i3c_inst)) {
			/* Check timeout*/
			if (IS_BIT_SET(i3c_inst->MERRWARN, NCT_I3C_MERRWARN_TIMEOUT)) {
				LOG_WRN("ERR: timeout");
			}

			i3c_inst->MERRWARN = i3c_inst->MERRWARN;

			return -EIO;
		}

		/* Check rx not empty */
		if (IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_RXPEND)) {

			/* Receive all the data in this round.
			 * Read in a tight loop to reduce chance of losing
			 * FIFO data when the i3c speed is high.
			 */
			while (offset < rd_sz) {
				if (GET_FIELD(i3c_inst->MDATACTRL,
						       NCT_I3C_MDATACTRL_RXCOUNT) == 0) {
					break;
				}

				buf[offset++] = (uint8_t)i3c_inst->MRDATAB;
			}
		}
	}

	return offset;
}

static enum nct_i3c_oper_state get_oper_state(const struct device *dev)
{
	struct nct_i3c_data *const data = dev->data;

	return data->oper_state;
}

static void set_oper_state(const struct device *dev, enum nct_i3c_oper_state state)
{
	struct nct_i3c_data *const data = dev->data;

	data->oper_state = state;
}

#ifdef CONFIG_I3C_NCT_DMA
static int nct_i3c_pdma_dsct(const struct device *dev, bool is_rx,
			     struct pdma_dsct_reg **dsct_inst)
{
	const struct nct_i3c_config *config = dev->config;

	if (is_rx) {
		*dsct_inst = (struct pdma_dsct_reg *)config->pdma_rx;
	} else {
		*dsct_inst = (struct pdma_dsct_reg *)config->pdma_tx;
	}

	return NCT_PDMA_DSCT_IDX((uint32_t)*dsct_inst);
}

static void nct_i3c_ctrl_notify(const struct device *dev)
{
	struct nct_i3c_data *const data = dev->data;

	k_sem_give(&data->sync_sem);
}

static int nct_i3c_ctrl_wait_completion(const struct device *dev)
{
	struct nct_i3c_data *const data = dev->data;

	return k_sem_take(&data->sync_sem, I3C_TRANS_TIMEOUT_MS);
}

static int nct_i3c_pdma_wait_completion(const struct device *dev, bool is_rx)
{
	struct pdma_reg *pdma_inst;
	struct pdma_dsct_reg *dsct_inst = NULL;
	uint8_t dsct_idx;

	dsct_idx = nct_i3c_pdma_dsct(dev, is_rx, &dsct_inst);
	if (dsct_inst == NULL) {
		LOG_ERR("dsct(%d) not exist", dsct_idx);
		return -EINVAL;
	}

	pdma_inst = (struct pdma_reg *) NCT_PDMA_BASE((uint32_t)dsct_inst);
	if (pdma_inst == NULL) {
		LOG_ERR("pdma base address not exist.");
		return -EINVAL;
	}

	/* Check dma transfer done */
	if (WAIT_FOR(IS_BIT_SET(pdma_inst->PDMA_TDSTS, dsct_idx), I3C_CHK_TIMEOUT_US, NULL) ==
	    false) {
		LOG_ERR("Check dma transfer done timed out");
		return -ETIMEDOUT;
	}

	return 0;
}

static int nct_i3c_pdma_remain_count(const struct device *dev, bool is_rx)
{
	struct pdma_reg *pdma_inst;
	struct pdma_dsct_reg *dsct_inst = NULL;
	uint8_t dsct_idx;

	dsct_idx = nct_i3c_pdma_dsct(dev, is_rx, &dsct_inst);
	if (dsct_inst == NULL) {
		LOG_ERR("dsct(%d) not exist", is_rx);
		return -EINVAL;
	}

	pdma_inst = (struct pdma_reg *) NCT_PDMA_BASE((uint32_t)dsct_inst);
	if (pdma_inst == NULL) {
		LOG_ERR("pdma base address not exist.");
		return -EINVAL;
	}

	if (!IS_BIT_SET(pdma_inst->PDMA_TDSTS, dsct_idx)) {
		return GET_FIELD(dsct_inst->CTL, NCT_PDMA_DSCT_CTL_TXCNT) + 1;
	} else {
		return 0;
	}
}

static int nct_i3c_pdma_stop(const struct device *dev, bool is_rx)
{
	struct nct_i3c_data *const data = dev->data;
	struct pdma_dsct_reg *dsct_inst = NULL;
	struct pdma_reg *pdma_inst;
	uint8_t dsct_idx;
	uint32_t key;

	dsct_idx = nct_i3c_pdma_dsct(dev, is_rx, &dsct_inst);
	if (dsct_inst == NULL) {
		LOG_ERR("dsct(%d) not exist", is_rx);
		return -EINVAL;
	}

	/* Get pdma base address */
	pdma_inst = (struct pdma_reg *) NCT_PDMA_BASE((uint32_t)dsct_inst);
	if (pdma_inst == NULL) {
		LOG_ERR("pdma base address not exist.");
		return -EINVAL;
	}

	key = irq_lock();

	/* Clear transfer done flag */
	if (pdma_inst->PDMA_TDSTS & BIT(dsct_idx)) {
//		pdma_inst->PDMA_TDSTS |= BIT(dsct_idx);
	}

	pdma_inst->PDMA_CHCTL &= ~BIT(dsct_idx);

	/* Clear DMA triggered flag */
	data->dma_triggered &= ~BIT(dsct_idx);

	irq_unlock(key);

	return 0;
}

static int nct_i3c_pdma_start(const struct device *dev, bool is_rx)
{
	struct nct_i3c_data *const data = dev->data;
	struct pdma_dsct_reg *dsct_inst = NULL;
	struct pdma_reg *pdma_inst;
	uint8_t dsct_idx;
	uint32_t key;

	dsct_idx = nct_i3c_pdma_dsct(dev, is_rx, &dsct_inst);
	if (dsct_inst == NULL) {
		LOG_ERR("dsct(%d) not exist", is_rx);
		return -EINVAL;
	}

	/* Get pdma base address */
	pdma_inst = (struct pdma_reg *) NCT_PDMA_BASE((uint32_t)dsct_inst);
	if (pdma_inst == NULL) {
		LOG_ERR("pdma base address not exist.");
                return -EINVAL;
	}

	key = irq_lock();

// disable pdma interrupt
pdma_inst->PDMA_INTEN &= ~BIT(dsct_idx);

	/* Clear transfer done flag */
	if (pdma_inst->PDMA_TDSTS & BIT(dsct_idx)) {
		pdma_inst->PDMA_TDSTS |= BIT(dsct_idx);
	}

	/* Start PDMA */
	pdma_inst->PDMA_CHCTL |= BIT(dsct_idx);

	/* Set DMA triggered flag */
	data->dma_triggered |= BIT(dsct_idx);

	irq_unlock(key);

	return 0;
}

/*
 * brief:  Configure DMA transaction.
 *
 * For DMA read, use one descriptor table to received data from target/controller.
 * For DMA write, use two descriptor tables to transfer data to target/controller.
 *
 * param[in] dev           Pointer to controller device driver instance.
 * param[in] type          i3c config type
 * param[in] is_rx         true=rx or false=tx operation.
 * param[in] buf           Buffer to store data.
 * param[in] buf_sz        Number of bytes to read.
 * param[in] no_ending     True if not to signal end of message.
 *
 * return  Successful return 0, or negative if error.
 *
 */
static int nct_i3c_pdma_configure(const struct device *dev, enum i3c_config_type type, bool is_rx,
				   uint8_t *buf, uint16_t buf_sz, bool no_ending)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *const data = dev->data;
	struct pdma_reg *pdma_inst;
	struct pdma_dsct_reg *dsct_inst = NULL;
	uint8_t dsct_idx;
	uint8_t i3c_mux_id;
	uint32_t src_addr;
	uint32_t dst_addr;
	uint32_t ctrl;
	uint32_t key;

	/* No data to be transferred or wrong type */
	if ((buf == NULL) || (buf_sz == 0) || (type == I3C_CONFIG_CUSTOM)) {
		return 0;
	}

	dsct_idx = nct_i3c_pdma_dsct(dev, is_rx, &dsct_inst);
	if (dsct_inst == NULL) {
		LOG_ERR("dsct(%d) not exist", is_rx);
		return -EINVAL;
	}

	/* Get pdma base address */
	pdma_inst = (struct pdma_reg *) NCT_PDMA_BASE((uint32_t)dsct_inst);
	if (pdma_inst == NULL) {
		LOG_ERR("pdma base address not exist.");
		return -EINVAL;
	}

	i3c_mux_id = I3C_NCT_PDMA_MUX_ID((uint32_t)i3c_inst, is_rx);

	key = irq_lock();

	/* Setup channel request selection */
	SET_FIELD(pdma_inst->PDMA_REQSEL[dsct_idx / NCT_PDMA_CHANNEL_PER_REQ],
			NCT_PDMA_REQSEL_CHANNEL(dsct_idx % NCT_PDMA_CHANNEL_PER_REQ),
			i3c_mux_id);

	/*
	 * PDMA support scatter-gather and basic mode, we use scatter-gather mode
	 * as default mode.
	 */
	ctrl = 0;

	/* Initial top descriptor table */
	dsct_inst->CTL = NCT_PDMA_DSCT_CTL_OPMODE_SGM;
	dsct_inst->SA = 0x0;
	dsct_inst->DA = 0x0;
	dsct_inst->NEXT = (is_rx) ? ((uint32_t)&data->dsct_sg[0]) & 0xFFFF
				  : ((uint32_t)&data->dsct_sg[2]) & 0xFFFF;

	/* Configure scatter-gather table base MSB address. */
	pdma_inst->PDMA_SCATBA = (is_rx) ? ((uint32_t)&data->dsct_sg[0]) & 0xFFFF0000
					 : ((uint32_t)&data->dsct_sg[2]) & 0xFFFF0000;

	/* Set 8 bits transfer width */
	SET_FIELD(ctrl, NCT_PDMA_DSCT_CTL_TXWIDTH, NCT_PDMA_DSCT_CTL_TX_WIDTH_8);

	/* Set DMA single request type */
	ctrl |= BIT(NCT_PDMA_DSCT_CTL_TXTYPE_SINGLE);

	/* Set mode as basic means the last descriptor */
	SET_FIELD(ctrl, NCT_PDMA_DSCT_CTL_OPMODE, NCT_PDMA_DSCT_CTL_OPMODE_BASIC);

	/*
	 * For read DMA, fixed src address.
	 * For write DMA, fixed dst address.
	 */
	if (is_rx) {
		/* Set transfer size, TXCNT + 1 */
		SET_FIELD(ctrl, NCT_PDMA_DSCT_CTL_TXCNT, (buf_sz - 1));
		SET_FIELD(ctrl, NCT_PDMA_DSCT_CTL_SAINC, NCT_PDMA_DSCT_CTL_SAINC_FIX);
		SET_FIELD(ctrl, NCT_PDMA_DSCT_CTL_DAINC, 0x0);

		/* Set source address */
		src_addr = (type == I3C_CONFIG_CONTROLLER) ? (uint32_t)&i3c_inst->MRDATAB
							   : (uint32_t)&i3c_inst->RDATAB;

		/* Fixed destination address */
		dst_addr = (uint32_t)&buf[0];
	} else {
		SET_FIELD(ctrl, NCT_PDMA_DSCT_CTL_DAINC, NCT_PDMA_DSCT_CTL_DAINC_FIX);
		SET_FIELD(ctrl, NCT_PDMA_DSCT_CTL_SAINC, 0x0);

		/* Fixed source address */
		src_addr = (uint32_t)&buf[0];

		/* Set transfer size, TXCNT + 1 */
		SET_FIELD(ctrl, NCT_PDMA_DSCT_CTL_TXCNT, (buf_sz - 1));

		/* Set destination address */
		if (no_ending) {
			dst_addr = (type == I3C_CONFIG_CONTROLLER) ? (uint32_t)&i3c_inst->MWDATAB1
								   : (uint32_t)&i3c_inst->WDATAB1;
		} else if (buf_sz > 1) {
			/*
			 * In this case need use second descriptor table, re-configure
			 * first descriptor SGM and (tx_length - 2), the last byte use
			 * second decriptor table.
			 */
			SET_FIELD(ctrl, NCT_PDMA_DSCT_CTL_OPMODE,
					NCT_PDMA_DSCT_CTL_OPMODE_SGM);
			SET_FIELD(ctrl, NCT_PDMA_DSCT_CTL_TXCNT, (buf_sz - 2));

			dst_addr = (type == I3C_CONFIG_CONTROLLER) ? (uint32_t)&i3c_inst->MWDATAB1
								   : (uint32_t)&i3c_inst->WDATAB1;
		} else {
			dst_addr = (type == I3C_CONFIG_CONTROLLER) ? (uint32_t)&i3c_inst->MWDATABE
								   : (uint32_t)&i3c_inst->WDATABE;
		}
	}

	/* Set next descriptor */
	if (is_rx) {
	data->dsct_sg[0].CTL = ctrl;
	data->dsct_sg[0].SA = src_addr;
	data->dsct_sg[0].DA = dst_addr;
	data->dsct_sg[0].NEXT = 0x0;
	} else {
		data->dsct_sg[2].CTL = ctrl;
		data->dsct_sg[2].SA = src_addr;
		data->dsct_sg[2].DA = dst_addr;
		data->dsct_sg[2].NEXT = 0x0;

		/* If first descriptor use scatter-gather mode */
		if (GET_FIELD(data->dsct_sg[2].CTL, NCT_PDMA_DSCT_CTL_OPMODE) ==
				NCT_PDMA_DSCT_CTL_OPMODE_SGM) {
			/* Configure next descriptor. */
			data->dsct_sg[2].NEXT = (uint32_t)&data->dsct_sg[3];

			/* Set basic mode for last descriptor */
			SET_FIELD(ctrl, NCT_PDMA_DSCT_CTL_OPMODE,
					NCT_PDMA_DSCT_CTL_OPMODE_BASIC);
			SET_FIELD(ctrl, NCT_PDMA_DSCT_CTL_TXCNT, 0x0);

			data->dsct_sg[3].CTL = ctrl;
			data->dsct_sg[3].SA = (uint32_t)&buf[buf_sz - 1];
			data->dsct_sg[3].DA = (type == I3C_CONFIG_CONTROLLER)
						      ? (uint32_t)&i3c_inst->MWDATABE
						      : (uint32_t)&i3c_inst->WDATABE;
			data->dsct_sg[3].NEXT = 0x0;
		}
	}

	irq_unlock(key);

	return 0;
}

static uint8_t nct_i3c_pdma_get_index(const struct device *dev, bool is_rx)
{
	struct pdma_dsct_reg *dsct_inst = NULL;
	uint8_t dsct_idx;

	dsct_idx = nct_i3c_pdma_dsct(dev, is_rx, &dsct_inst);
	if (dsct_inst == NULL) {
		LOG_ERR("dsct(%d) not exist", dsct_idx);
		return -EINVAL;
	}

	return dsct_idx;
}

static int nct_i3c_controller_dma_on(const struct device *dev, bool is_rx)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	int ret = 0;

	/* Enable PDMA */
	ret = nct_i3c_pdma_start(dev, is_rx);
	if (ret != 0) {
		return ret;
	}

	/* Enable DMA */
	if (is_rx) {
		/* Receive */
		SET_FIELD(i3c_inst->MDMACTRL, NCT_I3C_MDMACTRL_DMAFB,
				   MDMA_DMAFB_EN_MANUAL);
	} else {
		/* Transmit */
		SET_FIELD(i3c_inst->MDMACTRL, NCT_I3C_MDMACTRL_DMATB,
				   MDMA_DMATB_EN_MANUAL);
	}

	return 0;
}

static int nct_i3c_controller_dma_off(const struct device *dev, bool is_rx)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *const data = dev->data;
	uint8_t dsct_idx = nct_i3c_pdma_get_index(dev, is_rx);
	int ret = 0;

	/* Only disable set DMA */
	if (!(data->dma_triggered & BIT(dsct_idx))) {
		return ret;
	}

	/* Disable DMA */
	if (is_rx) {
		if (GET_FIELD(i3c_inst->MDMACTRL, NCT_I3C_MDMACTRL_DMAFB) !=
		    MDMA_DMAFB_DISABLE) {
			SET_FIELD(i3c_inst->MDMACTRL, NCT_I3C_MDMACTRL_DMAFB,
					   MDMA_DMAFB_DISABLE);
		}
	} else {
		if (GET_FIELD(i3c_inst->MDMACTRL, NCT_I3C_MDMACTRL_DMATB) !=
		    MDMA_DMATB_DISABLE) {
			SET_FIELD(i3c_inst->MDMACTRL, NCT_I3C_MDMACTRL_DMATB,
					   MDMA_DMATB_DISABLE);
		}
	}

	/* Stop PDMA */
	ret = nct_i3c_pdma_stop(dev, is_rx);

	/* Flush FIFO */
	nct_i3c_controller_fifo_flush(i3c_inst);

	return ret;
}

static int nct_i3c_target_dma_on(const struct device *dev, bool is_rx)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	int ret = 0;

	ret = nct_i3c_pdma_start(dev, is_rx);
	if (ret < 0) {
		return ret;
	}

	if (is_rx) {
		SET_FIELD(i3c_inst->DMACTRL, NCT_I3C_DMACTRL_DMAFB,
				   DMA_DMAFB_EN_MANUAL);
	} else {
		SET_FIELD(i3c_inst->DMACTRL, NCT_I3C_DMACTRL_DMATB,
				   DMA_DMATB_EN_MANUAL);
	}

	return 0;
}

static int nct_i3c_target_dma_off(const struct device *dev, bool is_rx)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *const data = dev->data;
	uint8_t dsct_idx = nct_i3c_pdma_get_index(dev, is_rx);
	int ret = 0;

	/* Only disable set DMA */
	if (!(data->dma_triggered & BIT(dsct_idx))) {
		return ret;
	}

	/* Disable DMA */
	if (is_rx) {
		/* Receive */
		if (GET_FIELD(i3c_inst->DMACTRL, NCT_I3C_DMACTRL_DMAFB) !=
		    DMA_DMAFB_DISABLE) {
			SET_FIELD(i3c_inst->DMACTRL, NCT_I3C_DMACTRL_DMAFB,
					   DMA_DMAFB_DISABLE);
		}
	} else {
		/* Transmit */
		if (GET_FIELD(i3c_inst->DMACTRL, NCT_I3C_DMACTRL_DMATB) !=
		    DMA_DMATB_DISABLE) {
			SET_FIELD(i3c_inst->DMACTRL, NCT_I3C_DMACTRL_DMATB,
					   DMA_DMATB_DISABLE);
		}
	}

	/* Stop PDMA */
	ret = nct_i3c_pdma_stop(dev, is_rx);

	/* Flush FIFO */
	if (is_rx) {
		nct_i3c_target_rx_fifo_flush(i3c_inst);
	} else {
		nct_i3c_target_tx_fifo_flush(i3c_inst);
	}

	return ret;
}

/*
 * brief:  Perform I3C target DMA transaction.
 *
 * This function performs I3C target DMA transaction for read or write.
 *
 * param[in] dev       Pointer to controller device driver instance.
 * param[in] is_rx     true=rx or false=tx operation.
 * param[in] buf       Buffer containing data to be sent or received.
 *                     The caller must ensure that the buffer is valid until the
 *                     transaction is completed.
 * param[in] buf_sz    Number of bytes in buf to send or receive.
 * param[in] no_ending True if not to signal end of message.
 *
 * return  Number of bytes transferred, or negative if error.
 *
 */
static int nct_i3c_target_do_request_dma(const struct device *dev, bool is_rx, uint8_t *buf,
					  size_t buf_sz, bool no_ending)
{
	int ret;

	/* Stop previous PDMA */
	nct_i3c_target_dma_off(dev, is_rx);

	/* Configure PDMA */
	ret = nct_i3c_pdma_configure(dev, I3C_CONFIG_TARGET, is_rx, buf, buf_sz, no_ending);
	if (ret != 0) {
		goto err_out;
	}

	/* Enable PDMA */
	if (nct_i3c_target_dma_on(dev, is_rx) < 0) {
		ret = -EIO;
		goto err_out;
	}

	/* Check remian data count */
	ret = nct_i3c_pdma_remain_count(dev, is_rx);
	if (ret >= 0) {
		return (buf_sz - ret);
	}

err_out:
	nct_i3c_target_dma_off(dev, is_rx);

	return ret;
}

static int nct_i3c_ctlr_xfer_read_fifo_dma(const struct device *dev, uint8_t addr,
					    enum nct_i3c_mctrl_type op_type, uint8_t *buf,
					    size_t buf_sz, bool is_rx, bool emit_start,
					    bool emit_stop, bool no_ending)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	int ret;

	/* Stop PDMA */
	nct_i3c_target_dma_off(dev, true);

	ret = nct_i3c_pdma_configure(dev, I3C_CONFIG_CONTROLLER, true, buf, buf_sz, no_ending);
	if (ret) {
		return ret;
	}

	/* Enable DMA until DMA is disabled by setting DMAFB to 00 */
	ret = nct_i3c_controller_dma_on(dev, true);
	if (ret) {
		goto out_read_fifo_dma;
	}

	/* Emit START if needed */
	if (emit_start) {
		ret = nct_i3c_request_emit_start(i3c_inst, addr, op_type, is_rx, buf_sz);
		if (ret != 0) {
			goto out_read_fifo_dma;
		}
	}

	if (no_ending) {
		ret = nct_i3c_pdma_wait_completion(dev, true);
		if (ret) {
			LOG_ERR("i3c wait dma completion timeout");
		}
	} else {
		/* Enable COMPLETE interrupt */
		i3c_inst->MINTSET |= BIT(NCT_I3C_MINTSET_COMPLETE);

		ret = nct_i3c_ctrl_wait_completion(dev);
		if (ret) {
			i3c_inst->MINTCLR = NCT_I3C_MINTCLR_COMPLETE;
			LOG_ERR("i3c wait completion timeout 1");
		}
	}

out_read_fifo_dma:
	if (nct_i3c_controller_dma_off(dev, true) < 0) {
		ret = -EIO;
	}

	if (ret == 0 && (buf && buf_sz)) {
		ret = nct_i3c_pdma_remain_count(dev, true);
		if (ret >= 0) {
			ret = buf_sz - ret;
		}
	}

	/* Emit STOP if needed */
	if (emit_stop) {
		nct_i3c_request_emit_stop(i3c_inst);
	}

	return ret;
}

static int nct_i3c_ctlr_xfer_write_fifo_dma(const struct device *dev, uint8_t addr,
					     enum nct_i3c_mctrl_type op_type, uint8_t *buf,
					     size_t buf_sz, bool is_rx, bool emit_start,
					     bool emit_stop, bool no_ending)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	uint32_t key;
	int ret;

	ret = nct_i3c_pdma_configure(dev, I3C_CONFIG_CONTROLLER, false, buf, buf_sz, no_ending);
	if (ret) {
		return ret;
	}

	/*
	 * For write operation, we enable dma after emit start. Disable all interrupts
	 * to avoid i3c stall timeout.
	 */
	key = irq_lock();

	/* Emit START if needed */
	if (emit_start) {
		ret = nct_i3c_request_emit_start(i3c_inst, addr, op_type, is_rx, buf_sz);
		if (ret != 0) {
			irq_unlock(key);
			return ret;
		}
	}

	/* Enable PDMA after emit start */
	ret = nct_i3c_controller_dma_on(dev, false);
	if (ret) {
		irq_unlock(key);
		goto out_write_fifo_dma;
	}

	/* Enable interrupts */
	irq_unlock(key);

	if (no_ending) {
		ret = nct_i3c_pdma_wait_completion(dev, false);
		if (ret) {
			LOG_ERR("i3c wait dma completion timeout");
		}
	} else {
		/* Enable COMPLETE interrupt */
		i3c_inst->MINTSET |= BIT(NCT_I3C_MINTSET_COMPLETE);
		ret = nct_i3c_ctrl_wait_completion(dev);
		if (ret) {
			i3c_inst->MINTCLR = BIT(NCT_I3C_MINTCLR_COMPLETE);
			LOG_ERR("i3c wait completion timeout 2");
		}
	}

out_write_fifo_dma:
	if (nct_i3c_controller_dma_off(dev, false) < 0) {
		ret = -EIO;
	}

	if (ret == 0 && (buf && buf_sz)) {
		ret = nct_i3c_pdma_remain_count(dev, false);
		if (ret >= 0) {
			ret = buf_sz - ret;
		}
	}

	/* Emit STOP if needed */
	if (emit_stop) {
		nct_i3c_request_emit_stop(i3c_inst);
	}

	return ret;
}

/*
 * brief:  Perform one transfer transaction by DMA.
 *
 * param[in] dev         Pointer to device driver instance.
 * param[in] addr        Target address.
 * param[in] op_type     Request type.
 * param[in] buf         Buffer for data to be sent or received.
 * param[in] buf_sz      Buffer size in bytes.
 * param[in] is_rx    true=rx or false=tx operation.
 * param[in] emit_start  True if START is needed before read/write.
 * param[in] emit_stop   True if STOP is needed after read/write.
 * param[in] no_ending   True if not to signal end of message.
 *
 * return  Number of bytes read/written, or negative if error.
 */
static int nct_i3c_do_one_xfer_dma(const struct device *dev, uint8_t addr,
				    enum nct_i3c_mctrl_type op_type, uint8_t *buf, size_t buf_sz,
				    bool is_rx, bool emit_start, bool emit_stop, bool no_ending)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	int ret = 0;

	nct_i3c_status_clear_all(i3c_inst);
	nct_i3c_errwarn_clear_all(i3c_inst);

	if (is_rx) {
		ret = nct_i3c_ctlr_xfer_read_fifo_dma(dev, addr, op_type, buf, buf_sz, is_rx,
						       emit_start, emit_stop, no_ending);
	} else {
		ret = nct_i3c_ctlr_xfer_write_fifo_dma(dev, addr, op_type, buf, buf_sz, is_rx,
							emit_start, emit_stop, no_ending);
	}

	if (ret < 0) {
		LOG_ERR("%s fifo fail", is_rx ? "read" : "write");
		return ret;
	}

	/* Check I3C bus error */
	if (nct_i3c_has_error(i3c_inst)) {
		LOG_ERR("I3C bus error, 0x%08x", i3c_inst->MERRWARN);
		return -EIO;
	}

	if (no_ending) {
		/* Flush fifo data */
		nct_i3c_controller_fifo_flush(i3c_inst);
	}

	return ret;
}

/* brief:  Handle the end of transfer (read request or write request).
 *         The ending signal might be either STOP or Sr.
 * return: -EINVAL: operation not read or write request.
 *         -ETIMEDOUT: in write request, wait rx fifo empty as pdma done.
 *          0: success
 */
static int nct_i3c_target_xfer_end_handle_dma(const struct device *dev,
					       enum nct_i3c_oper_state oper_state)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *const data = dev->data;

#ifdef CONFIG_I3C_TARGET_BUFFER_MODE	
	const struct i3c_target_callbacks *target_cb =
		(data->target_config != NULL) ? data->target_config->callbacks : NULL;
#endif
	uint8_t len = 0;
	uint8_t rx_fifo_count;
	bool is_rx;
	int ret = 0;
	int i;

	if (oper_state == I3C_OP_STATE_RD) {
		is_rx = false;

		/* After STOP, the data in tx fifo is invalid. */
		data->tx_valid = false;

		goto out_pdma_end;
	} else if (oper_state == I3C_OP_STATE_WR) {
		is_rx = true;

		/* Wait until rx fifo is stable */
#define RX_FIFO_EMPTY_TIMEOUT 100
		len = GET_FIELD(i3c_inst->DATACTRL, NCT_I3C_DATACTRL_RXCOUNT);
		for (i = 0; i < RX_FIFO_EMPTY_TIMEOUT; i++) {
			/* For 12.5MHz, [data] + [T] = 0.75us */
			k_busy_wait(10);
			rx_fifo_count =
				GET_FIELD(i3c_inst->DATACTRL, NCT_I3C_DATACTRL_RXCOUNT);

			if (len == rx_fifo_count) {
				break;
			} else {
				len = rx_fifo_count;
			}
		}

		if (len) {
			ret = nct_i3c_target_do_request_dma(dev, is_rx, data->dma_rx_buf, len,
							     false);
			if (ret < 0) {
				LOG_ERR("DMA write request failed");
				goto out_pdma_end;
			}

			ret = nct_i3c_pdma_wait_completion(dev, is_rx);
			if (ret) {
				LOG_ERR("i3c wait dma completion timeout");
			}

#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
			if (target_cb && target_cb->buf_write_received_cb) {
				target_cb->buf_write_received_cb(data->target_config,
								 data->dma_rx_buf, len);
			}

			// For V2.6 mctp
			if (data->slave_data.callbacks != NULL) {
				if (data->slave_data.callbacks->write_requested != NULL) {
					struct i3c_config_target *config_tgt = &data->config_target;

					data->rx_payload = data->slave_data.callbacks->write_requested(
						data->slave_data.dev);

					data->rx_payload->size = config_tgt->max_read_len;
				}

				memcpy(data->rx_payload->buf, data->dma_rx_buf, len);
				data->rx_payload->size = len;

				if (data->slave_data.callbacks->write_done != NULL) {
					data->slave_data.callbacks->write_done(data->slave_data.dev);
				}
			}
#endif
		} else {
			LOG_ERR("rx fifo empty");
		}
	} else {
		LOG_ERR("oper_state error :%d", oper_state);
		return -EINVAL;
	}

out_pdma_end:
	nct_i3c_target_dma_off(dev, is_rx);

	return ret;
}

static int nct_i3c_pdma_stop_v2(const struct device *dev, bool is_rx)
{
	struct nct_i3c_data *const data = dev->data;
	struct i3c_config_target *config_tgt = &data->config_target;
	struct pdma_dsct_reg *dsct_inst = NULL;
	struct pdma_reg *pdma_inst;
	uint8_t dsct_idx;
	uint32_t key;

	dsct_idx = nct_i3c_pdma_dsct(dev, is_rx, &dsct_inst);
	if (dsct_inst == NULL) {
		LOG_ERR("dsct(%d) not exist", is_rx);
		return -EINVAL;
	}

	/* Get pdma base address */
	pdma_inst = (struct pdma_reg *)NCT_PDMA_BASE((uint32_t)dsct_inst);
	if (pdma_inst == NULL) {
		LOG_ERR("pdma base address not exist.");
		return -EINVAL;
	}

	key = irq_lock();

	/* Clear transfer done flag */
	if (pdma_inst->PDMA_TDSTS & BIT(dsct_idx)) {
		pdma_inst->PDMA_TDSTS |= BIT(dsct_idx);

		// update slave rcv data length before reset TDSTS
		if (config_tgt->enable && is_rx) {
			data->slave_rx_payload[data->rx_payload_out].size = config_tgt->max_read_len;
		}
	}
	else {
		if (config_tgt->enable && is_rx) {
			data->slave_rx_payload[data->rx_payload_out].size = config_tgt->max_read_len - 
				(GET_FIELD(dsct_inst->CTL, NCT_PDMA_DSCT_CTL_TXCNT) + 1);
		}
	}

	pdma_inst->PDMA_CHCTL &= ~BIT(dsct_idx);

	/* Clear DMA triggered flag */
	data->dma_triggered &= ~BIT(dsct_idx);

	irq_unlock(key);

	return 0;
}


static int nct_i3c_target_dma_off_v2(const struct device *dev, bool is_rx)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *const data = dev->data;
	uint8_t dsct_idx = nct_i3c_pdma_get_index(dev, is_rx);
	int ret = 0;

	/* Only disable set DMA */
	if (!(data->dma_triggered & BIT(dsct_idx))) {
		return ret;
	}

	/* Disable DMA */
	if (is_rx) {
		/* Receive */
		if (GET_FIELD(i3c_inst->DMACTRL, NCT_I3C_DMACTRL_DMAFB) !=
		    DMA_DMAFB_DISABLE) {
			SET_FIELD(i3c_inst->DMACTRL, NCT_I3C_DMACTRL_DMAFB,
					   DMA_DMAFB_DISABLE);
		}
	} else {
		/* Transmit */
		if (GET_FIELD(i3c_inst->DMACTRL, NCT_I3C_DMACTRL_DMATB) !=
		    DMA_DMATB_DISABLE) {
			SET_FIELD(i3c_inst->DMACTRL, NCT_I3C_DMACTRL_DMATB,
					   DMA_DMATB_DISABLE);
		}
	}

	/* Stop PDMA */
	ret = nct_i3c_pdma_stop_v2(dev, is_rx);

	/* Flush FIFO */
	if (is_rx) {
		nct_i3c_target_rx_fifo_flush(i3c_inst);
	} else {
		nct_i3c_target_tx_fifo_flush(i3c_inst);
	}

	return ret;
}

static int nct_i3c_target_do_request_dma_v2(const struct device *dev, bool is_rx, uint8_t *buf,
					  size_t buf_sz, bool no_ending)
{
	int ret;

	/* Stop the previous PDMA */
	nct_i3c_target_dma_off_v2(dev, is_rx);

	/* Configure PDMA */
	ret = nct_i3c_pdma_configure(dev, I3C_CONFIG_TARGET, is_rx, buf, buf_sz, no_ending);
	if (ret != 0) {
		goto err_out;
	}

	/* Enable PDMA */
	if (nct_i3c_target_dma_on(dev, is_rx) < 0) {
		ret = -EIO;
		goto err_out;
	}

	/* Check remian data count */
	ret = nct_i3c_pdma_remain_count(dev, is_rx);
	if (ret >= 0) {
		return (buf_sz - ret);
	}

err_out:
	nct_i3c_target_dma_off(dev, is_rx);

	return ret;
}

static int nct_i3c_target_xfer_end_handle_dma_v2(const struct device *dev,
					       enum nct_i3c_oper_state oper_state)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *const data = dev->data;
	struct i3c_config_target *config_tgt = &data->config_target;
#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
	const struct i3c_target_callbacks *target_cb =
		(data->target_config != NULL) ? data->target_config->callbacks : NULL;
#endif
	uint16_t len = 0;
	uint16_t rx_fifo_count;
	bool is_rx;
	int ret = 0;
	int i;

	if (oper_state == I3C_OP_STATE_RD) {
		is_rx = false;

		len = GET_FIELD(i3c_inst->DATACTRL, NCT_I3C_DATACTRL_TXCOUNT);
		if (len == 0) {
        	if (tx_fifo_empty_cb) {
        		tx_fifo_empty_cb();
        	}
		}

		/* After STOP, the data in tx fifo is invalid. */
		data->tx_valid = false;

		goto out_pdma_end;
	} else if (oper_state == I3C_OP_STATE_WR) {
		is_rx = true;

		/* Wait until no data insert into rx fifo */
#define RX_FIFO_EMPTY_TIMEOUT 100
		len = GET_FIELD(i3c_inst->DATACTRL, NCT_I3C_DATACTRL_RXCOUNT);
		for (i = 0; i < RX_FIFO_EMPTY_TIMEOUT; i++) {
			/* For 12.5MHz, [data] + [T] = 0.75us */
			k_busy_wait(10);
			rx_fifo_count =
				GET_FIELD(i3c_inst->DATACTRL, NCT_I3C_DATACTRL_RXCOUNT);

			if (len == rx_fifo_count) {
				break;
			} else {
				len = rx_fifo_count;
			}
		}

		update_i3c_slave_rx_payload(dev);		

		struct i3c_slave_payload *new_payload = alloc_i3c_slave_rx_payload(dev);
		new_payload->size = config_tgt->max_read_len;

		is_rx = true;
		bool no_ending = false;

		ret = nct_i3c_target_do_request_dma_v2(dev, is_rx, new_payload->buf, config_tgt->max_read_len, no_ending);
		if (ret < 0) {
			LOG_ERR("do xfer fail");
		}

		// process the received data
		len = data->slave_rx_payload[data->rx_payload_out].size;

	#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
		if (target_cb && target_cb->buf_write_received_cb) {
			target_cb->buf_write_received_cb(data->target_config, 
				data->slave_rx_payload[data->rx_payload_out].buf, len);
		}
	#endif
		// For V2.6 mctp
		if (data->slave_data.callbacks != NULL) {
			if (data->slave_data.callbacks->write_requested != NULL) {
				data->rx_payload = data->slave_data.callbacks->write_requested(
					data->slave_data.dev);

				data->rx_payload->size = config_tgt->max_read_len;
			}

			memcpy(data->rx_payload->buf, data->slave_rx_payload[data->rx_payload_out].buf, len);
			data->rx_payload->size = len;

			if (data->slave_data.callbacks->write_done != NULL) {
				data->slave_data.callbacks->write_done(data->slave_data.dev);
			}
		}

		data->rx_payload_out = (data->rx_payload_out + 1) % ARRAY_SIZE(data->pdma_rx_buf);
		return 0;
	} else if (oper_state == I3C_OP_STATE_CCC) {
		is_rx = true;

		/* Wait until no data insert into rx fifo */
#define RX_FIFO_EMPTY_TIMEOUT 100
		len = GET_FIELD(i3c_inst->DATACTRL, NCT_I3C_DATACTRL_RXCOUNT);
		for (i = 0; i < RX_FIFO_EMPTY_TIMEOUT; i++) {
			/* For 12.5MHz, [data] + [T] = 0.75us */
			k_busy_wait(10);
			rx_fifo_count =
				GET_FIELD(i3c_inst->DATACTRL, NCT_I3C_DATACTRL_RXCOUNT);

			if (len == rx_fifo_count) {
				break;
			} else {
				len = rx_fifo_count;
			}
		}	

		update_i3c_slave_rx_payload(dev);

		struct i3c_slave_payload *new_payload = alloc_i3c_slave_rx_payload(dev);
		new_payload->size = config_tgt->max_read_len;

		is_rx = true;
		bool no_ending = false;

		ret = nct_i3c_target_do_request_dma_v2(dev, is_rx, new_payload->buf, config_tgt->max_read_len, no_ending);
		if (ret < 0) {
			LOG_ERR("do xfer fail");
		}

		// process the received data
		len = data->slave_rx_payload[data->rx_payload_out].size;

		// call CCC handler
		uint8_t buf[10];
		uint8_t rcv_cnt;

		rcv_cnt = data->slave_rx_payload[data->rx_payload_out].size;
		memcpy(buf, data->slave_rx_payload[data->rx_payload_out].buf, rcv_cnt);

		if (buf[0] == I3C_CCC_RSTACT(true)) {
			LOG_DBG("CCC RSTACT received");
		}

		data->rx_payload_out = (data->rx_payload_out + 1) % ARRAY_SIZE(data->pdma_rx_buf);
		return 0;
	} else {
		LOG_ERR("oper_state error :%d", oper_state);
		return -EINVAL;
	}

out_pdma_end:
	nct_i3c_target_dma_off(dev, is_rx);

	return ret;
}
#else
static bool nct_i3c_target_has_error(struct i3c_reg *i3c_inst)
{
	if (i3c_inst->STATUS & NCT_I3C_STATUS_ERRWARN) {
		LOG_WRN("ERROR: STATUS 0x%08x ERRWARN 0x%08x", i3c_inst->STATUS, i3c_inst->ERRWARN);

		return true;
	}

	return false;
}

/* brief:  Handle the end of transfer (read request or write request).
 *         The ending signal might be either STOP or Sr.
 * return: -EINVAL: operation not read or write request.
 *         -ETIMEDOUT: in write request, wait rx fifo empty as pdma done.
 *          0: success
 */
static int nct_i3c_target_xfer_end_handle(const struct device *dev,
					   enum nct_i3c_oper_state oper_state)

{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *const data = dev->data;
#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
	const struct i3c_target_callbacks *target_cb =
		(data->target_config != NULL) ? data->target_config->callbacks : NULL;
#endif
	int ret = 0;

	if (oper_state == I3C_OP_STATE_RD) {
		/* Set buffer invalid */
		data->tx_len = 0;

		nct_i3c_target_tx_fifo_flush(i3c_inst);
	} else if (oper_state == I3C_OP_STATE_WR) {
#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
		if (target_cb && target_cb->buf_write_received_cb) {
			target_cb->buf_write_received_cb(data->target_config, data->rx_buf,
							 data->rx_len);
		}

		nct_i3c_target_rx_fifo_flush(i3c_inst);
#endif
	}

	return ret;
}

/*
 * brief:  Perform one write transaction.
 *
 * This writes all data in buf to TX FIFO or time out
 * waiting for FIFO spaces.
 *
 * param[in] i3c_inst   Pointer to controller registers.
 * param[in] buf        Buffer containing data to be sent.
 * param[in] buf_sz     Number of bytes in buf to send.
 * param[in] no_ending  True if not to signal end of message.
 *
 * return  Number of bytes written, or negative if error.
 *
 */
static int nct_i3c_xfer_target_write_fifo(struct i3c_reg *i3c_inst, uint8_t *buf, uint8_t buf_sz,
					   bool no_ending)
{
	int remaining;
	int offset;
	int tx_remain;
	int i;

	/* Set buf_sz - 1 bytes */
	for (offset = 0, remaining = buf_sz - 1; remaining > 0;
	     offset += tx_remain, remaining -= tx_remain) {
		if (WAIT_FOR(GET_FIELD(i3c_inst->DATACTRL, NCT_I3C_DATACTRL_TXCOUNT) <
				     I3C_FIFO_SIZE,
			     I3C_CHK_TIMEOUT_US, NULL) == false) {
			return -ETIMEDOUT;
		}

		tx_remain = I3C_FIFO_SIZE -
			    GET_FIELD(i3c_inst->DATACTRL, NCT_I3C_DATACTRL_TXCOUNT);
		tx_remain = (tx_remain > remaining) ? remaining : tx_remain;

		for (i = 0; i < tx_remain; i++) {
			i3c_inst->WDATAB = (uint32_t)buf[offset + i];
		}
	}

	/* Set last byte */
	if (no_ending) {
		i3c_inst->WDATAB = (uint32_t)buf[offset++];
	} else {
		i3c_inst->WDATABE = (uint32_t)buf[offset++];
	}

	return offset;
}

/*
 * brief:  Perform read transaction.
 *
 * This reads from RX FIFO until COMPLETE bit is set in MSTATUS
 * or time out.
 *
 * param[in] i3c_inst  Pointer to controller registers.
 * param[in] buf       Buffer to store data.
 *
 * return  Number of bytes read, or negative if error.
 *
 */
static int nct_i3c_xfer_target_read_fifo(struct i3c_reg *i3c_inst, uint8_t *buf)
{
	bool is_done = false;
	int offset = 0;

	while (is_done == false) {
		/* Check tarnsaction is done */
		if ((i3c_inst->STATUS & NCT_I3C_STATUS_STOP) ||
		    (i3c_inst->STATUS & NCT_I3C_STATUS_START)) {
			is_done = true;
		}

		/* Check message is canceled */
		if (i3c_inst->STATUS & NCT_I3C_STATUS_CHANDLED) {
			is_done = true;
		}

		/* Check I3C bus error */
		if (nct_i3c_target_has_error(i3c_inst)) {
			/* Check timeout */
			if (i3c_inst->ERRWARN & NCT_I3C_ERRWARN_TERM) {
				LOG_WRN("ERR: terminated");
			}

			i3c_inst->ERRWARN = i3c_inst->ERRWARN;

			return -EIO;
		}

		/* Check rx not empty */
		if (i3c_inst->STATUS & NCT_I3C_STATUS_RXPEND) {
			/*
			 * Receive all the data in this round.
			 * Read in a tight loop to reduce chance of losing
			 * FIFO data when the i3c speed is high.
			 */
			while (GET_FIELD(i3c_inst->DATACTRL, NCT_I3C_DATACTRL_RXCOUNT)) {
				buf[offset++] = (uint8_t)i3c_inst->RDATAB;
			}
		}
	}

	return offset;
}
#endif /* End of CONFIG_I3C_NCT_DMA */

// setup rx pdma for target to receive request from controller
static void nct_i3c_target_rx_read(const struct device *dev)
{
	struct nct_i3c_data *data = dev->data;
	struct i3c_config_target *config_tgt = &data->config_target;

#ifdef CONFIG_I3C_NCT_DMA
	bool is_rx = true;
	bool no_ending = false;

	struct i3c_slave_payload *new_payload = alloc_i3c_slave_rx_payload(dev);
	new_payload->size = config_tgt->max_read_len;

	int ret = nct_i3c_target_do_request_dma_v2(dev, is_rx, new_payload->buf, config_tgt->max_read_len, no_ending);
	if (ret < 0) {
		LOG_ERR("do xfer fail");
	}
#endif	
}

/*
 * brief:  Perform one transfer transaction.
 *
 * param[in] dev         Pointer to device driver instance
 * param[in] addr        Target address.
 * param[in] op_type     Request type.
 * param[in] buf         Buffer for data to be sent or received.
 * param[in] buf_sz      Buffer size in bytes.
 * param[in] is_rx       true=rx or false=tx operation.
 * param[in] emit_start  True if START is needed before read/write.
 * param[in] emit_stop   True if STOP is needed after read/write.
 * param[in] no_ending   True if not to signal end of write message.
 *
 * return  Number of bytes read/written, or negative if error.
 */
static int nct_i3c_do_one_xfer(const struct device *dev, uint8_t addr,
				enum nct_i3c_mctrl_type op_type, uint8_t *buf, size_t buf_sz,
				bool is_rx, bool emit_start, bool emit_stop, bool no_ending)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	int ret = 0;

	nct_i3c_status_clear_all(i3c_inst);
	nct_i3c_errwarn_clear_all(i3c_inst);

	/* Emit START if needed */
	if (emit_start) {
		ret = nct_i3c_request_emit_start(i3c_inst, addr, op_type, is_rx, buf_sz);
		if (ret != 0) {
			goto out_do_one_xfer;
		}
	}

	/* No data to be transferred */
	if ((buf == NULL) || (buf_sz == 0)) {
		goto out_do_one_xfer;
	}

	/* Select read or write operation */
	if (is_rx) {
		ret = nct_i3c_xfer_read_fifo(i3c_inst, buf, buf_sz);
	} else {
		ret = nct_i3c_xfer_write_fifo(i3c_inst, buf, buf_sz, no_ending);
	}

	if (ret < 0) {
		LOG_ERR("%s fifo fail", is_rx ? "read" : "write");
		goto out_do_one_xfer;
	}

	/* Check message complete if is a read transaction or
	 * ending byte of a write transaction.
	 */
	if (is_rx || !no_ending) {
		/* Wait message transfer complete */
		if (WAIT_FOR(IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_COMPLETE), I3C_CHK_TIMEOUT_US,
			     NULL) == false) {
			LOG_ERR("timed out addr 0x%02x, buf_sz %u", addr, buf_sz);

			ret = -ETIMEDOUT;
			emit_stop = true;

			goto out_do_one_xfer;
		}

		i3c_inst->MSTATUS = BIT(NCT_I3C_MSTATUS_COMPLETE); /* W1C */
	}

	/* Check I3C bus error */
	if (nct_i3c_has_error(i3c_inst)) {
		ret = -EIO;
		LOG_ERR("I3C bus error");
	}

out_do_one_xfer:
	/* Emit STOP if needed */
	if (emit_stop) {
		nct_i3c_request_emit_stop(i3c_inst);
	}

	return ret;
}

/*
 * brief:  Transfer messages in I3C mode.
 *
 * see i3c_transfer
 *
 * param[in] dev       Pointer to device driver instance.
 * param[in] target    Pointer to target device descriptor.
 * param[in] msgs      Pointer to I3C messages.
 * param[in] num_msgs  Number of messages to transfers.
 *
 * return  see i3c_transfer
 */
static int nct_i3c_transfer(const struct device *dev, struct i3c_device_desc *target,
			     struct i3c_msg *msgs, uint8_t num_msgs)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	uint32_t intmask;
	int xfered_len, ret = 0;
	bool send_broadcast = true;
	bool is_xfer_done = true;

	if (msgs == NULL) {
		return -EINVAL;
	}

	if (target->dynamic_addr == 0U) {
		return -EINVAL;
	}

	nct_i3c_mutex_lock(dev);

	/* Check bus in idle state */
	if (WAIT_FOR((nct_i3c_state_get(i3c_inst) == MSTATUS_STATE_IDLE),
		     I3C_CHK_TIMEOUT_US, NULL) == false) {
		LOG_ERR("xfer state error: %d", nct_i3c_state_get(i3c_inst));
		nct_i3c_mutex_unlock(dev);
		return -ETIMEDOUT;
	}

	/* Disable interrupt */
	intmask = i3c_inst->MINTSET;
	nct_i3c_interrupt_all_disable(i3c_inst);

	nct_i3c_xfer_reset(i3c_inst);

	/* Iterate over all the messages */
	for (int i = 0; i < num_msgs; i++) {
		bool is_rx = (msgs[i].flags & I3C_MSG_RW_MASK) == I3C_MSG_READ;
		bool no_ending = false;

		/*
		 * Emit start if this is the first message or that
		 * the RESTART flag is set in message.
		 */
		bool emit_start =
			(i == 0) || ((msgs[i].flags & I3C_MSG_RESTART) == I3C_MSG_RESTART);

		bool emit_stop = (msgs[i].flags & I3C_MSG_STOP) == I3C_MSG_STOP;

		/*
		 * The controller requires special treatment of last byte of
		 * a write message. Since the API permits having a bunch of
		 * write messages without RESTART in between, this is just some
		 * logic to determine whether to treat the last byte of this
		 * message to be the last byte of a series of write mssages.
		 * If not, tell the write function not to treat it that way.
		 */
		if (!is_rx && !emit_stop && ((i + 1) != num_msgs)) {
			bool next_is_write = (msgs[i + 1].flags & I3C_MSG_RW_MASK) == I3C_MSG_WRITE;
			bool next_is_restart =
				((msgs[i + 1].flags & I3C_MSG_RESTART) == I3C_MSG_RESTART);

			/* Check next msg is still write operation and not including Sr */
			if (next_is_write && !next_is_restart) {
				no_ending = true;
			}
		}

		/*
		 * Two ways to do read/write transfer .
		 * 1. [S] + [0x7E]    + [address] + [data] + [Sr or P]
		 * 2. [S] + [address] + [data]    + [Sr or P]
		 *
		 * Send broadcast header(0x7E) on first transfer or after a STOP,
		 * unless flag is set not to.
		 */
		if (!(msgs[i].flags & I3C_MSG_NBCH) && (send_broadcast)) {
			ret = nct_i3c_request_emit_start(i3c_inst, I3C_BROADCAST_ADDR,
							  NCT_I3C_MCTRL_TYPE_I3C, false, 0);
			if (ret < 0) {
				LOG_ERR("emit start of broadcast addr failed, error (%d)", ret);
				break;
			}
			send_broadcast = false;
		}

#ifdef CONFIG_I3C_NCT_DMA
		/* Do transfer with target device */
		xfered_len = nct_i3c_do_one_xfer_dma(dev, target->dynamic_addr, NCT_I3C_MCTRL_TYPE_I3C,
						      msgs[i].buf, msgs[i].len, is_rx, emit_start,
						      emit_stop, no_ending);
#else
		xfered_len = nct_i3c_do_one_xfer(dev, target->dynamic_addr, NCT_I3C_MCTRL_TYPE_I3C,
						  msgs[i].buf, msgs[i].len, is_rx, emit_start,
						  emit_stop, no_ending);
#endif
		if (xfered_len < 0) {
			LOG_ERR("do xfer fail");
			ret = xfered_len; /* Set error code to ret */
			break;
		}

		/* Write back the total number of bytes transferred */
		msgs[i].num_xfer = xfered_len;

		if (emit_stop) {
			/* After a STOP, send broadcast header before next msg */
			send_broadcast = true;
		}

		/* Check emit stop flag including in the final msg */
		if ((i == num_msgs - 1) && (emit_stop == false)) {
			is_xfer_done = false;
		}
	}

	/* Emit stop if error occurs or stop flag not in the msg */
	if ((ret != 0) || (is_xfer_done == false)) {
		nct_i3c_request_emit_stop(i3c_inst);
	}

	nct_i3c_errwarn_clear_all(i3c_inst);
	nct_i3c_status_clear_all(i3c_inst);

	nct_i3c_interrupt_enable(i3c_inst, intmask);

	nct_i3c_mutex_unlock(dev);
	return ret;
}

/*
 * brief:  Perform Dynamic Address Assignment.
 *
 * param[in] dev  Pointer to controller device driver instance.
 *
 * return  0 If successful.
 *         -EBUSY Bus is busy.
 *         -EIO General input / output error.
 *         -ENODEV If a provisioned ID does not match to any target devices
 *                 in the registered device list.
 *         -ENOSPC No more free addresses can be assigned to target.
 *         -ENOSYS Dynamic address assignment is not supported by
 *                 the controller driver.
 */
static int nct_i3c_do_daa(const struct device *dev)
{
	const struct nct_i3c_config *config = dev->config;
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *data = dev->data;
	int ret = 0;
	uint8_t rx_buf[8];
	size_t rx_count;
	uint32_t intmask;

	nct_i3c_mutex_lock(dev);

	memset(rx_buf, 0xff, sizeof(rx_buf));

	/* Check bus in idle state */
	if (WAIT_FOR((nct_i3c_state_get(i3c_inst) == MSTATUS_STATE_IDLE),
		     I3C_CHK_TIMEOUT_US, NULL) == false) {
		LOG_ERR("DAA state error: %d", nct_i3c_state_get(i3c_inst));
		nct_i3c_mutex_unlock(dev);
		return -ETIMEDOUT;
	}

	LOG_DBG("DAA: ENTDAA");

	/* Disable interrupt */
	intmask = i3c_inst->MINTSET;
	nct_i3c_interrupt_all_disable(i3c_inst);

	nct_i3c_xfer_reset(i3c_inst);

	/* Emit process DAA */
	if (nct_i3c_request_daa(i3c_inst) != 0) {
		ret = -ETIMEDOUT;
		LOG_ERR("Emit process DAA error");
		goto out_do_daa;
	}

	/* Loop until no more responses from devices */
	do {
		/* Check ERRWARN bit set */
		if (nct_i3c_has_error(i3c_inst)) {
			ret = -EIO;
			LOG_ERR("DAA recv error");
			break;
		}

		/* Receive Provisioned ID, BCR and DCR (total 8 bytes) */
		rx_count = GET_FIELD(i3c_inst->MDATACTRL, NCT_I3C_MDATACTRL_RXCOUNT);

		if (rx_count == DAA_TGT_INFO_SZ) {
			for (int i = 0; i < rx_count; i++) {
				rx_buf[i] = (uint8_t)i3c_inst->MRDATAB;
			}
		} else {
			/* Data count not as expected, exit DAA */
			ret = -EBADMSG;
			LOG_DBG("Rx count not as expected %d, abort DAA", rx_count);
			break;
		}

		/* Start assign dynamic address */
		if ((nct_i3c_state_get(i3c_inst) == MSTATUS_STATE_DAA) &&
		    (IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_BETWEEN))) {
			struct i3c_device_desc *target;
			uint16_t vendor_id;
			uint32_t part_no;
			uint64_t pid;
			uint8_t dyn_addr = 0;

			/* PID[47:33] = manufacturer ID */
			vendor_id = (((uint16_t)rx_buf[0] << 8U) | (uint16_t)rx_buf[1]) & 0xFFFEU;

			/* PID[31:0] = vendor fixed falue or random value */
			part_no = (uint32_t)rx_buf[2] << 24U | (uint32_t)rx_buf[3] << 16U |
				  (uint32_t)rx_buf[4] << 8U | (uint32_t)rx_buf[5];

			/* Combine into one Provisioned ID */
			pid = (uint64_t)vendor_id << 32U | (uint64_t)part_no;

			LOG_DBG("DAA: Rcvd PID 0x%04x%08x", vendor_id, part_no);

				/* Find a usable address during ENTDAA */
			ret = i3c_dev_list_daa_addr_helper(&data->common.attached_dev.addr_slots,
							   &config->common.dev_list, pid, false,
							   true, &target, &dyn_addr);
			if (ret != 0) {
				LOG_ERR("Assign new DA error");
				break;
			}

			if (target == NULL) {
				LOG_INF("%s: PID 0x%04x%08x is not in registered device "
					"list, given dynamic address 0x%02x",
					dev->name, vendor_id, part_no, dyn_addr);
			} else {
				/* Update target descriptor */
				target->dynamic_addr = dyn_addr;
				target->bcr = rx_buf[6];
				target->dcr = rx_buf[7];
			}

			/* Mark the address as I3C device */
			i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots, dyn_addr);

			/*
			 * If the device has static address, after address assignment,
			 * the device will not respond to the static address anymore.
			 * So free the static one from address slots if different from
			 * newly assigned one.
			 */
			if ((target != NULL) && (target->static_addr != 0U) &&
			    (dyn_addr != target->static_addr)) {

				LOG_DBG("Free static address 0x%02x", target->static_addr);

				i3c_addr_slots_mark_free(&data->common.attached_dev.addr_slots,
							 dyn_addr);
			}

			/* Emit process DAA again to send the address to the device */
			i3c_inst->MWDATAB = dyn_addr;
			ret = nct_i3c_request_daa(i3c_inst);
			if (ret != 0) {
				LOG_ERR("Assign DA timeout");
				break;
			}

			LOG_DBG("PID 0x%04x%08x assigned dynamic address 0x%02x", vendor_id,
				part_no, dyn_addr);

			/* Target did not accept the assigned DA, exit DAA */
			if (i3c_inst->MSTATUS & NCT_I3C_MSTATUS_NACKED) {
				ret = -EFAULT;
				LOG_DBG("TGT NACK assigned DA %#x", dyn_addr);

				/* Free the reserved DA */
				i3c_addr_slots_mark_free(&data->common.attached_dev.addr_slots,
							 dyn_addr);

				/* 0 if address has not been assigned */
				if (target != NULL) {
					target->dynamic_addr = 0;
				}

				break;
			}
		}

		/* Check all targets have been assigned DA and DAA complete */
	} while ((!(IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_COMPLETE))) &&
		 nct_i3c_state_get(i3c_inst) != MSTATUS_STATE_IDLE);

out_do_daa:
	/* Exit DAA mode when error occurs */
	if (ret != 0) {
		nct_i3c_request_emit_stop(i3c_inst);
	}

	/* Clear all flags. */
	nct_i3c_errwarn_clear_all(i3c_inst);
	nct_i3c_status_clear_all(i3c_inst);

	/* Re-Enable I3C IRQ sources. */
	nct_i3c_interrupt_enable(i3c_inst, intmask);

	nct_i3c_controller_fifo_flush(i3c_inst);
	nct_i3c_mutex_unlock(dev);

	return ret;
}

/*
 * brief:  Send Common Command Code (CCC).
 *
 * param[in] dev      Pointer to controller device driver instance.
 * param[in] payload  Pointer to CCC payload.
 *
 * return:  The same as i3c_do_ccc()
 *          0 If successful.
 *          -EBUSY Bus is busy.
 *          -EIO General Input / output error.
 *          -EINVAL Invalid valid set in the payload structure.
 *          -ENOSYS Not implemented.
 */
static int nct_i3c_do_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	uint32_t intmask;
	int xfered_len;
	int ret;

	if (dev == NULL || payload == NULL) {
		return -EINVAL;
	}

	nct_i3c_mutex_lock(dev);

	/* Disable interrupt */
	intmask = i3c_inst->MINTSET;
	nct_i3c_interrupt_all_disable(i3c_inst);

	/* Clear status and flush fifo */
	nct_i3c_xfer_reset(i3c_inst);

	LOG_DBG("CCC[0x%02x]", payload->ccc.id);

	/* Write emit START and broadcast address (0x7E) */
	ret = nct_i3c_request_emit_start(i3c_inst, I3C_BROADCAST_ADDR, NCT_I3C_MCTRL_TYPE_I3C, false,
					  0);
	if (ret < 0) {
		LOG_ERR("CCC[0x%02x] %s START error (%d)", payload->ccc.id,
			i3c_ccc_is_payload_broadcast(payload) ? "broadcast" : "direct", ret);

		goto out_do_ccc;
	}

	/* Write CCC command */
	nct_i3c_status_clear_all(i3c_inst);
	nct_i3c_errwarn_clear_all(i3c_inst);
	xfered_len =
		nct_i3c_xfer_write_fifo(i3c_inst, &payload->ccc.id, 1, payload->ccc.data_len > 0);
	if (xfered_len < 0) {
		LOG_ERR("CCC[0x%02x] %s command error (%d)", payload->ccc.id,
			i3c_ccc_is_payload_broadcast(payload) ? "broadcast" : "direct", ret);
		ret = xfered_len;

		goto out_do_ccc;
	}

	/* Write data (defining byte or data bytes) for CCC if needed */
	if (payload->ccc.data_len > 0) {
		nct_i3c_status_clear_all(i3c_inst);
		nct_i3c_errwarn_clear_all(i3c_inst);
		xfered_len = nct_i3c_xfer_write_fifo(i3c_inst, payload->ccc.data,
						      payload->ccc.data_len, false);
		if (xfered_len < 0) {
			LOG_ERR("CCC[0x%02x] %s command payload error (%d)", payload->ccc.id,
				i3c_ccc_is_payload_broadcast(payload) ? "broadcast" : "direct",
				ret);
			ret = xfered_len;

			goto out_do_ccc;
		}

		/* Write back the transferred bytes */
		payload->ccc.num_xfer = xfered_len;
	}

	/* Wait message transfer complete */
	if (WAIT_FOR(IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_COMPLETE), I3C_CHK_TIMEOUT_US, NULL) ==
	    false) {
		ret = -ETIMEDOUT;
		LOG_DBG("Check complete timeout");
		goto out_do_ccc;
	}

	i3c_inst->MSTATUS = BIT(NCT_I3C_MSTATUS_COMPLETE); /* W1C */

	/* For direct CCC */
	if (!i3c_ccc_is_payload_broadcast(payload)) {
		/*
		 * If there are payload(s) for each target,
		 * RESTART and then send payload for each target.
		 */
		for (int idx = 0; idx < payload->targets.num_targets; idx++) {
			struct i3c_ccc_target_payload *tgt_payload =
				&payload->targets.payloads[idx];

			bool is_rx = (tgt_payload->rnw == 1U);

			xfered_len = nct_i3c_do_one_xfer(
				dev, tgt_payload->addr, NCT_I3C_MCTRL_TYPE_I3C, tgt_payload->data,
				tgt_payload->data_len, is_rx, true, false, false);
			if (xfered_len < 0) {
				LOG_ERR("CCC[0x%02x] target payload error (%d)", payload->ccc.id,
					ret);
				ret = xfered_len;

				goto out_do_ccc;
			}

			/* Write back the total number of bytes transferred */
			tgt_payload->num_xfer = xfered_len;
		}
	}

out_do_ccc:
	nct_i3c_request_emit_stop(i3c_inst);

	nct_i3c_interrupt_enable(i3c_inst, intmask);

	nct_i3c_mutex_unlock(dev);

	return ret;
}

#ifdef CONFIG_I3C_USE_IBI
/*
 * brief  Callback to service target initiated IBIs in workqueue.
 *
 * param[in] work  Pointer to k_work item.
 */
static void nct_i3c_ibi_work(struct k_work *work)
{
	uint8_t payload[CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE];
	size_t payload_sz = 0;

	struct i3c_ibi_work *i3c_ibi_work = CONTAINER_OF(work, struct i3c_ibi_work, work);
	const struct device *dev = i3c_ibi_work->controller;
	struct nct_i3c_data *data = dev->data;
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct i3c_device_desc *target = NULL;
	uint32_t ibitype, ibiaddr;
	int ret;

	k_sem_take(&data->ibi_lock_sem, K_FOREVER);

	if (nct_i3c_state_get(i3c_inst) != MSTATUS_STATE_TGTREQ) {
		LOG_DBG("IBI work %p running not because of IBI", work);
		LOG_ERR("MSTATUS 0x%08x MERRWARN 0x%08x", i3c_inst->MSTATUS, i3c_inst->MERRWARN);

		nct_i3c_request_emit_stop(i3c_inst);

		goto out_ibi_work;
	};

	/* Use auto IBI to service the IBI */
	nct_i3c_request_auto_ibi(i3c_inst);

	/* Wait for target to win address arbitration (ibitype and ibiaddr) */
	if (WAIT_FOR(IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_IBIWON), I3C_CHK_TIMEOUT_US, NULL) ==
	    false) {
		LOG_ERR("IBI work, IBIWON timeout");

		goto out_ibi_work;
	}

	ibitype = GET_FIELD(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_IBITYPE);
	ibiaddr = GET_FIELD(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_IBIADDR);

	switch (ibitype) {
	case MSTATUS_IBITYPE_IBI:
		target = i3c_dev_list_i3c_addr_find(dev, (uint8_t)ibiaddr);
		if (target != NULL) {
			ret = nct_i3c_xfer_read_fifo(i3c_inst, &payload[0], sizeof(payload));
			if (ret >= 0) {
				payload_sz = (size_t)ret;
if (payload_sz != 1) {
	LOG_ERR("IBI payload size = %u", payload_sz);
}

if (payload[0] != 0xAE) {
	LOG_ERR("IBI payload = %02X", payload[0]);
}

			} else {
				LOG_ERR("Error reading IBI payload");

				nct_i3c_request_emit_stop(i3c_inst);

				goto out_ibi_work;
			}
		} else {
			/* NACK IBI coming from unknown device */
			nct_i3c_ibi_respond_nack(i3c_inst);
		}
		break;

	case MSTATUS_IBITYPE_HJ:
		nct_i3c_ibi_respond_ack(i3c_inst);
		nct_i3c_request_emit_stop(i3c_inst);
		break;

	case MSTATUS_IBITYPE_CR:
		LOG_DBG("Controller role handoff not supported");
		nct_i3c_ibi_respond_nack(i3c_inst);
		nct_i3c_request_emit_stop(i3c_inst);
		break;

	default:
		/* Intentionally left empty */
		break;
	}

	if (nct_i3c_has_error(i3c_inst)) {
		/*
		 * If the controller detects any errors, simply
		 * emit a STOP to abort the IBI. The target will
		 * raise IBI again if so desired.
		 */
		nct_i3c_request_emit_stop(i3c_inst);

		goto out_ibi_work;
	}

	switch (ibitype) {
	case MSTATUS_IBITYPE_IBI:
		if (target != NULL) {
			if (i3c_ibi_work_enqueue_target_irq(target, &payload[0], payload_sz) != 0) {
				LOG_ERR("Error enqueue IBI IRQ work");
			}
		}

		/* Finishing the IBI transaction */
		nct_i3c_request_emit_stop(i3c_inst);
		break;

	case MSTATUS_IBITYPE_HJ:
		if (i3c_ibi_work_enqueue_hotjoin(dev) != 0) {
			LOG_ERR("Error enqueue IBI HJ work");
		}
		break;

	case MSTATUS_IBITYPE_CR:
		/* Not supported, for future use. */
		break;

	default:
		break;
	}

out_ibi_work:
	nct_i3c_xfer_reset(i3c_inst);

	k_sem_give(&data->ibi_lock_sem);

	/* Re-enable target initiated IBI interrupt. */
	i3c_inst->MINTSET = BIT(NCT_I3C_MINTSET_TGTSTART);
}

/* Set local IBI information to IBIRULES register */
#define NCT_I3C_IBIRULES_ADDR_MSK   0x3F
#define NCT_I3C_IBIRULES_ADDR_SHIFT 0x6

static void nct_i3c_ibi_rules_setup(struct nct_i3c_data *data, struct i3c_reg *i3c_inst)
{
	uint32_t ibi_rules;
	int idx;

	ibi_rules = 0;

	for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
		uint32_t addr_6bit;

		/* Extract the lower 6-bit of target address */
		addr_6bit = (uint32_t)data->ibi.addr[idx] & NCT_I3C_IBIRULES_ADDR_MSK;

		/* Shift into correct place */
		addr_6bit <<= idx * NCT_I3C_IBIRULES_ADDR_SHIFT;

		/* Put into the temporary IBI Rules register */
		ibi_rules |= addr_6bit;
	}

	/* Enable i3c address arbitration optimization strategy. */
	if (!data->ibi.msb) {
		/* The MSB0 field is 1 if MSB is 0 */
		ibi_rules |= BIT(NCT_I3C_IBIRULES_MSB0);
	} else {
		ibi_rules &= ~BIT(NCT_I3C_IBIRULES_MSB0);
	}

	if (!data->ibi.has_mandatory_byte) {
		/* The NOBYTE field is 1 if there is no mandatory byte */
		ibi_rules |= BIT(NCT_I3C_IBIRULES_NOBYTE);
	}

	/* Update the register */
	i3c_inst->IBIRULES = ibi_rules;
}

static int nct_i3c_ibi_enable(const struct device *dev, struct i3c_device_desc *target)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *data = dev->data;
	struct i3c_ccc_events i3c_events;
	uint8_t idx;
	bool msb, has_mandatory_byte;
	int ret;

	/* Check target IBI request capable */
	if (!i3c_device_is_ibi_capable(target)) {
		LOG_ERR("device is not ibi capable");
		return -EINVAL;
	}

	if (data->ibi.num_addr >= ARRAY_SIZE(data->ibi.addr)) {
		/* No more free entries in the IBI Rules table */
		LOG_ERR("no more free space in the IBI rules table");
		return -ENOMEM;
	}

	/* Check whether the selected target is already in the list */
	for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
		if (data->ibi.addr[idx] == target->dynamic_addr) {
			LOG_ERR("selected target is already in the list");
			return -EINVAL;
		}
	}

	/* Disable controller interrupt while we configure IBI rules. */
	i3c_inst->MINTCLR = BIT(NCT_I3C_MINTCLR_TGTSTART);

	/* Check addess(7-bit) MSB enable */
	msb = (target->dynamic_addr & BIT(6)) == BIT(6);
	has_mandatory_byte = i3c_ibi_has_payload(target);

	/*
	 * If there are already addresses in the table, we must
	 * check if the incoming entry is compatible with
	 * the existing ones.
	 *
	 * All targets in the list should follow the same IBI rules.
	 */
	if (data->ibi.num_addr > 0) {
		/*
		 * 1. All devices in the table must all use mandatory
		 *    bytes, or do not.
		 *
		 * 2. Each address in entry only captures the lowest 6-bit.
		 *    The MSB (7th bit) is captured separated in another bit
		 *    in the register. So all addresses must have the same MSB.
		 */
		if ((has_mandatory_byte != data->ibi.has_mandatory_byte) ||
		    (msb != data->ibi.msb)) {
			ret = -EINVAL;
			LOG_ERR("New IBI does not have same mandatory byte or msb"
				" as previous IBI");
			goto out_ibi_enable;
		}

		/* Find an empty address slot */
		for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
			if (data->ibi.addr[idx] == 0U) {
				break;
			}
		}

		if (idx >= ARRAY_SIZE(data->ibi.addr)) {
			ret = -ENOTSUP;
			LOG_ERR("Cannot support more IBIs");
			goto out_ibi_enable;
		}
	} else {
		/*
		 * If the incoming address is the first in the table,
		 * it dictates future compatibilities.
		 */
		data->ibi.has_mandatory_byte = has_mandatory_byte;
		data->ibi.msb = msb;

		idx = 0;
	}

	data->ibi.addr[idx] = target->dynamic_addr;
	data->ibi.num_addr += 1U;

	nct_i3c_ibi_rules_setup(data, i3c_inst);

	/* Enable target IBI event by ENEC command */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, true, &i3c_events);
	if (ret != 0) {
		LOG_ERR("Error sending IBI ENEC for 0x%02x (%d)", target->dynamic_addr, ret);
	}

out_ibi_enable:
	if (data->ibi.num_addr > 0U) {
		/*
		 * If there is more than 1 target in the list,
		 * enable controller to raise interrupt when a target
		 * initiates IBI.
		 */
		i3c_inst->MINTSET = BIT(NCT_I3C_MINTSET_TGTSTART);
	}

	return ret;
}

static int nct_i3c_ibi_disable(const struct device *dev, struct i3c_device_desc *target)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *data = dev->data;
	struct i3c_ccc_events i3c_events;
	int ret;
	int idx;

	if (!i3c_device_is_ibi_capable(target)) {
		LOG_ERR("device is not ibi capable");
		return -EINVAL;
	}

	for (idx = 0; idx < ARRAY_SIZE(data->ibi.addr); idx++) {
		if (target->dynamic_addr == data->ibi.addr[idx]) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(data->ibi.addr)) {
		LOG_ERR("target is not in list of registered addresses");
		return -ENODEV;
	}

	/* Disable controller interrupt while we configure IBI rules. */
	i3c_inst->MINTCLR = BIT(NCT_I3C_MINTCLR_TGTSTART);

	/* Clear the ibi rule data */
	data->ibi.addr[idx] = 0U;
	data->ibi.num_addr -= 1U;

	/* Disable disable target IBI */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, false, &i3c_events);
	if (ret != 0) {
		LOG_ERR("Error sending IBI DISEC for 0x%02x (%d)", target->dynamic_addr, ret);
	}

	nct_i3c_ibi_rules_setup(data, i3c_inst);

	if (data->ibi.num_addr > 0U) {
		/*
		 * Enable controller to raise interrupt when a target
		 * initiates IBI.
		 */
		i3c_inst->MINTSET = BIT(NCT_I3C_MINTSET_TGTSTART);
	}

	return ret;
}

static int nct_i3c_target_ibi_raise(const struct device *dev, struct i3c_ibi *request)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *data = dev->data;

	/* The request or the payload were not specific */
	if ((request == NULL) || ((request->payload_len) && (request->payload == NULL))) {
		return -EINVAL;
	}

	/* The I3C was not in target mode or the bus is in HDR mode now */
	if (!(IS_BIT_SET(i3c_inst->CONFIG, NCT_I3C_CONFIG_TGTENA)) ||
	    (IS_BIT_SET(i3c_inst->STATUS, NCT_I3C_STATUS_STHDR))) {
		return -EINVAL;
	}

	switch (request->ibi_type) {
	case I3C_IBI_TARGET_INTR:
		if (IS_BIT_SET(i3c_inst->STATUS, NCT_I3C_STATUS_IBIDIS)) {
			return -ENOTSUP;
		}

		if (request->payload_len == 0) {
			LOG_ERR("IBI invalid payload_len, len: %#x", request->payload_len);
			return -EINVAL;
		}

		/* The payload length is too long */
		if (request->payload_len > I3C_IBI_MAX_PAYLOAD_SIZE) {
			LOG_ERR("IBI payload too long, use dma instead");
			return -EINVAL;
		}

		k_sem_take(&data->target_event_lock_sem, K_FOREVER);
		set_oper_state(dev, I3C_OP_STATE_IBI);

		/* Mandatory data byte */
		SET_FIELD(i3c_inst->CTRL, NCT_I3C_CTRL_IBIDATA, request->payload[0]);

		/* Extended data */
		if (request->payload_len > 1) {
#ifdef CONFIG_I3C_NCT_DMA
			int ret;

			ret = nct_i3c_target_do_request_dma(dev, false, &request->payload[1],
							     request->payload_len - 1, false);
			if (ret < 0) {
				LOG_ERR("DMA write request failed");
				return -EIO;
			}
#else
			int index;

			/* For transaction > 16 bytes, use dma to avoid bus underrun. */
			for (index = 1; index < (request->payload_len - 1); index++) {
				i3c_inst->WDATAB = request->payload[index];
			}

			i3c_inst->WDATABE = request->payload[index];
#endif
			SET_FIELD(i3c_inst->IBIEXT1, NCT_I3C_IBIEXT1_CNT, 0);
			i3c_inst->CTRL |= NCT_I3C_CTRL_EXTDATA;
		}

		SET_FIELD(i3c_inst->CTRL, NCT_I3C_CTRL_EVENT, CTRL_EVENT_IBI);
		break;

	case I3C_IBI_CONTROLLER_ROLE_REQUEST:
		if (IS_BIT_SET(i3c_inst->STATUS, NCT_I3C_STATUS_MRDIS)) {
			return -ENOTSUP;
		}

		/*
		 * The bus controller request was generate only a target with controller mode
		 * capabilities mode
		 */
		if (GET_FIELD(i3c_inst->MCONFIG, NCT_I3C_MCONFIG_CTRENA) !=
		    MCONFIG_CTRENA_CAPABLE) {
			return -ENOTSUP;
		}

		k_sem_take(&data->target_event_lock_sem, K_FOREVER);
		set_oper_state(dev, I3C_OP_STATE_IBI);

		SET_FIELD(i3c_inst->CTRL, NCT_I3C_CTRL_EVENT,
				   CTRL_EVENT_CNTLR_REQ);
		break;

	case I3C_IBI_HOTJOIN:
		if (IS_BIT_SET(i3c_inst->STATUS, NCT_I3C_STATUS_HJDIS)) {
			return -ENOTSUP;
		}

		k_sem_take(&data->target_event_lock_sem, K_FOREVER);
		set_oper_state(dev, I3C_OP_STATE_IBI);

		i3c_inst->CONFIG &= ~BIT(NCT_I3C_CONFIG_TGTENA);
		SET_FIELD(i3c_inst->CTRL, NCT_I3C_CTRL_EVENT, CTRL_EVENT_HJ);
		i3c_inst->CONFIG |= BIT(NCT_I3C_CONFIG_TGTENA);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_I3C_USE_IBI */

static inline int nct_i3c_target_MATCHED_handler(const struct device *dev)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *data = dev->data;
	const struct i3c_target_callbacks *target_cb =
		(data->target_config != NULL) ? data->target_config->callbacks : NULL;
	enum nct_i3c_oper_state oper_state = get_oper_state(dev);
	int ret = 0;
	uint32_t int_status = i3c_inst->STATUS;

#ifdef CONFIG_I3C_NCT_DMA
	if (oper_state != I3C_OP_STATE_IBI) {
		/* The current bus request is an SDR mode read or write */
		if (IS_BIT_SET(int_status, NCT_I3C_STATUS_STREQRD)) {
			/* SDR read request */
			set_oper_state(dev, I3C_OP_STATE_RD);
			ret = 1;

			/*
			 * It will be too late to enable pdma here, use target_tx_write() to
			 * write tx data into fifo before controller send read request.
			 */
#if CONFIG_I3C_TARGET_BUFFER_MODE
			/* Emit read request callback */
			if ((target_cb != NULL) && (target_cb->buf_read_requested_cb != NULL)) {
				target_cb->buf_read_requested_cb(data->target_config, NULL, NULL,
								 NULL);
			}
#endif
		} else if (IS_BIT_SET(int_status, NCT_I3C_STATUS_STREQWR)) {
			/* SDR write request */
			set_oper_state(dev, I3C_OP_STATE_WR);
			ret = 1;

			/* Emit write request callback */
			if ((target_cb != NULL) && (target_cb->write_requested_cb != NULL)) {
				target_cb->write_requested_cb(data->target_config);
			}
		}
	}
#else
	if (oper_state != I3C_OP_STATE_IBI) {
		if (IS_BIT_SET(int_status, NCT_I3C_STATUS_STREQRD)) {
			/* SDR read request */
			set_oper_state(dev, I3C_OP_STATE_RD);
			ret = 1;

			/*
			 * It will be too late to fill in buffer and write to fifo here, use write
			 * request to notify target to prepare tx data with target_tx_write()
			 * before controller send read request.
			 */
#ifndef CONFIG_I3C_TARGET_BUFFER_MODE
			/* Emit read request callback. */
			if ((target_cb != NULL) && (target_cb->read_requested_cb != NULL)) {
				target_cb->read_requested_cb(data->target_config, data->tx_buf);
			}
#endif

			/* Fill tx FIFO */
			if (data->tx_len) {
				ret = nct_i3c_xfer_target_write_fifo(i3c_inst, data->tx_buf,
								      data->tx_len, false);
				if (ret < 0) {
					LOG_ERR("Write tx FIFO failed");
				}
			} else {
				LOG_ERR("No tx data");
				ret = -EINVAL;
			}

#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
			/* Emit buffer read request callback */
			if ((target_cb != NULL) && (target_cb->buf_read_requested_cb != NULL)) {
				target_cb->buf_read_requested_cb(data->target_config, NULL, NULL,
								 NULL);
			}
#endif
		} else if (IS_BIT_SET(int_status, NCT_I3C_STATUS_STREQWR)) {
			/* SDR write request */
			set_oper_state(dev, I3C_OP_STATE_WR);
			ret = 1;

			/* Emit write request callback */
			if ((target_cb != NULL) && (target_cb->write_requested_cb != NULL)) {
				target_cb->write_requested_cb(data->target_config);
			}

			/* Fill write data to rx buffer */
			ret = nct_i3c_xfer_target_read_fifo(i3c_inst, data->rx_buf);
			if (ret < 0) {
				LOG_ERR("Read rx FIFO failed");
			} else {
				data->rx_len = ret;
			}
		}
	}
#endif

	/*
	 * If CONFIG.MATCHSS=1, MATCHED bit must remain 1 to detect next start
	 * or stop.
	 *
	 * Clear the status bit in STOP or START handler.
	 */
	if (IS_BIT_SET(i3c_inst->CONFIG, NCT_I3C_CONFIG_MATCHSS)) {
		i3c_inst->INTCLR = BIT(NCT_I3C_INTCLR_MATCHED);
	} else {
		i3c_inst->STATUS = BIT(NCT_I3C_STATUS_MATCHED);
	}

	return ret;
}

static inline void nct_i3c_target_STOP_handler(const struct device *dev)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *data = dev->data;
	const struct i3c_target_callbacks *target_cb =
		(data->target_config != NULL) ? data->target_config->callbacks : NULL;
	enum nct_i3c_oper_state oper_state = get_oper_state(dev);

	if (IS_BIT_SET(i3c_inst->INTMASKED, NCT_I3C_INTMASKED_START)) {
		/* Clear the status bit */
		i3c_inst->STATUS = BIT(NCT_I3C_STATUS_START);
	}

	/*
	 * The end of xfer is a STOP.
	 * For write request: check whether rx fifo count is 0.
	 * For read request: disable the pdma operation.
	 */
#ifdef CONFIG_I3C_NCT_DMA
	if ((oper_state == I3C_OP_STATE_WR) || (oper_state == I3C_OP_STATE_RD)) {
		if (nct_i3c_target_xfer_end_handle_dma_v2(dev, oper_state) != 0) {
			LOG_ERR("xfer end handle failed after stop, op state=%d", oper_state);
		}
	} else if (oper_state == I3C_OP_STATE_IBI) {
		if (GET_FIELD(i3c_inst->DATACTRL, NCT_I3C_DATACTRL_TXCOUNT) == 0) {
			nct_i3c_target_dma_off(dev, false);
		}
	} else if (oper_state == I3C_OP_STATE_CCC) {
		if (nct_i3c_target_xfer_end_handle_dma_v2(dev, oper_state) != 0) {
			LOG_ERR("xfer end handle failed after stop, op state=%d", oper_state);
		}

		i3c_inst->STATUS = BIT(NCT_I3C_STATUS_CCC);
		i3c_inst->INTSET = BIT(NCT_I3C_INTSET_CCC);
	} else {
		/* Check RXPEND */
		if (IS_BIT_SET(i3c_inst->STATUS, NCT_I3C_STATUS_RXPEND) && !(data->tx_valid)) {
			nct_i3c_target_rx_fifo_flush(i3c_inst);
		}
	}
#else
	if ((oper_state == I3C_OP_STATE_WR) || (oper_state == I3C_OP_STATE_RD)) {
		if (nct_i3c_target_xfer_end_handle(dev, oper_state) != 0) {
			LOG_ERR("xfer end handle failed after stop, op state=%d", oper_state);
		}
	}
#endif

	/* Clear the status bit */
	i3c_inst->STATUS = BIT(NCT_I3C_STATUS_STOP);

	/* Notify upper layer a STOP condition received */
	if ((target_cb != NULL) && (target_cb->stop_cb != NULL)) {
		target_cb->stop_cb(data->target_config);
	}

	set_oper_state(dev, I3C_OP_STATE_IDLE);
}

static inline void nct_i3c_target_START_handler(const struct device *dev)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	enum nct_i3c_oper_state oper_state = get_oper_state(dev); /* Entry operation state */

	/* The end of xfer is a Sr */
	if ((oper_state == I3C_OP_STATE_WR) || (oper_state == I3C_OP_STATE_RD)) {
		/* Use entry operation state to handle the xfer end */
#ifdef CONFIG_I3C_NCT_DMA
		if (nct_i3c_target_xfer_end_handle_dma(dev, oper_state) == -ETIMEDOUT) {
			LOG_ERR("xfer end handle failed after start, op state=%d", oper_state);

			set_oper_state(dev, I3C_OP_STATE_IDLE);
		}
#else
		if (nct_i3c_target_xfer_end_handle(dev, oper_state) != 0) {
			LOG_ERR("xfer end handle failed after stop, op state=%d", oper_state);
		}
#endif
	}

	/* Clear the status bit */
	i3c_inst->STATUS = BIT(NCT_I3C_STATUS_START);
}

static void nct_i3c_target_isr(const struct device *dev)
{
	struct nct_i3c_data *data = dev->data;
	struct i3c_config_target *config_target = &data->config_target;
	struct i3c_target_config *target_config = data->target_config;
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	uint32_t intmask = i3c_inst->INTMASKED;

	while (intmask != 0) {
#ifndef CONFIG_I3C_NCT_DMA
		if (IS_BIT_SET(intmask, NCT_I3C_INTMASKED_RXPEND)) {
			/* Flush rx and tx FIFO */
			nct_i3c_target_rx_fifo_flush(i3c_inst);
			nct_i3c_target_tx_fifo_flush(i3c_inst);
		}
#endif

		/* Check STOP detected */
		if (IS_BIT_SET(intmask, NCT_I3C_INTMASKED_STOP)) {
			nct_i3c_target_STOP_handler(dev);
		}

		/* Check START or Sr detected */
		if (IS_BIT_SET(intmask, NCT_I3C_INTMASKED_START)) {
			nct_i3c_target_START_handler(dev);
		}

		/* Check incoming header matched target dynamic address */
		if (IS_BIT_SET(intmask, NCT_I3C_INTMASKED_MATCHED)) {
			nct_i3c_target_MATCHED_handler(dev);
		}

		/* Check error or warning has occurred */
		if (IS_BIT_SET(intmask, NCT_I3C_INTMASKED_ERRWARN)) {
			if (i3c_inst->ERRWARN == 0x100) {
				LOG_DBG("ERRWARN %x", i3c_inst->ERRWARN);
			}
			else {
				LOG_ERR("ERRWARN %x", i3c_inst->ERRWARN);
			}

			i3c_inst->ERRWARN = i3c_inst->ERRWARN;
		}

		/* Check dynamic address changed */
		if (IS_BIT_SET(intmask, NCT_I3C_INTMASKED_DACHG)) {
			i3c_inst->STATUS = BIT(NCT_I3C_STATUS_DACHG);
			if (IS_BIT_SET(i3c_inst->DYNADDR, NCT_I3C_DYNADDR_DAVALID)) {
				if (target_config != NULL) {
					config_target->dynamic_addr = GET_FIELD(
						i3c_inst->DYNADDR, NCT_I3C_DYNADDR_DADDR);
				}
			}
		}

		/* CCC 'not' automatically handled was received */
		if (IS_BIT_SET(intmask, NCT_I3C_INTMASKED_CCC)) {
			set_oper_state(dev, I3C_OP_STATE_CCC);
			i3c_inst->INTCLR = BIT(NCT_I3C_INTCLR_CCC); /* W1C */
		}

		/* HDR command, address match */
		if (IS_BIT_SET(intmask, NCT_I3C_INTMASKED_DDRMATCHED)) {
			i3c_inst->STATUS = BIT(NCT_I3C_STATUS_DDRMATCH);
		}

		/* CCC handled (handled by IP) */
		if (IS_BIT_SET(intmask, NCT_I3C_INTMASKED_CHANDLED)) {
			i3c_inst->STATUS = BIT(NCT_I3C_STATUS_CHANDLED);
		}

		/* Event requested. IBI, hot-join, bus control */
		if (IS_BIT_SET(intmask, NCT_I3C_INTMASKED_EVENT)) {
			i3c_inst->STATUS = BIT(NCT_I3C_STATUS_EVENT);
			if (GET_FIELD(i3c_inst->STATUS, NCT_I3C_STATUS_EVDET) ==
			    STATUS_EVDET_REQ_SENT_ACKED) {
				k_sem_give(&data->target_event_lock_sem);
			}
		}

		/* Check mask again, flags may be set when handling */
		intmask = i3c_inst->INTMASKED;
	}

	/*
	 * Secondary controller (Controller register).
	 * Check I3C now bus controller.
	 * Disable target mode if target switch to controller mode success.
	 */
	if (IS_BIT_SET(i3c_inst->MINTMASKED, NCT_I3C_MINTMASKED_NOWMASTER)) {
		i3c_inst->MSTATUS = BIT(NCT_I3C_MSTATUS_NOWCNTLR); /* W1C */
		i3c_inst->CONFIG &= ~BIT(NCT_I3C_CONFIG_TGTENA);   /* Disable target mode */
	}
}

static void nct_i3c_isr(const struct device *dev)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);

	if (IS_BIT_SET(i3c_inst->CONFIG, NCT_I3C_CONFIG_TGTENA)) {
		/* Target mode */
		nct_i3c_target_isr(dev);
	} else if (GET_FIELD(i3c_inst->MCONFIG, NCT_I3C_MCONFIG_CTRENA) ==
		   MCONFIG_CTRENA_ON) {
		/* Controller mode */
#ifdef CONFIG_I3C_NCT_DMA
		if (IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_COMPLETE)) {
			/* Clear COMPLETE status */
			i3c_inst->MSTATUS = BIT(NCT_I3C_MSTATUS_COMPLETE); /* W1C */

			/* Disable COMPLETE interrupt */
			i3c_inst->MINTCLR = BIT(NCT_I3C_MINTCLR_COMPLETE);

			nct_i3c_ctrl_notify(dev);
			return;
		}
#endif /* CONFIG_I3C_NCT_DMA */

#ifdef CONFIG_I3C_USE_IBI
		int ret;

		/* Target start detected */
		if (IS_BIT_SET(i3c_inst->MSTATUS, NCT_I3C_MSTATUS_TGTSTART)) {
			/* Disable further target initiated IBI interrupt */
			i3c_inst->MINTCLR = BIT(NCT_I3C_MINTCLR_TGTSTART);

			/* Clear SLVSTART interrupt */
			i3c_inst->MSTATUS = BIT(NCT_I3C_MSTATUS_TGTSTART);

			/* Handle IBI in workqueue */

			ret = i3c_ibi_work_enqueue_cb(dev, nct_i3c_ibi_work);
			if (ret < 0) {
				LOG_ERR("Enqueuing ibi work fail, ret %d", ret);
				i3c_inst->MINTSET = BIT(NCT_I3C_MINTSET_TGTSTART);
			}
		}
#endif /* CONFIG_I3C_USE_IBI */
	} else {
		LOG_ERR("Unknown mode");
	}
}

static int nct_i3c_get_scl_config(struct nct_i3c_timing_cfg *cfg, uint32_t i3c_src_clk,
				   uint32_t pp_baudrate_hz, uint32_t od_baudrate_hz,
				   uint32_t i2c_baudrate_hz)
{
	uint32_t div;
	uint32_t i3c_ppbaud, i3c_odbaud;
	uint32_t i2c_baud_ns, i2c_baud;
	uint32_t i3c_odlow_ns;
	uint32_t i3c_pphigh_ns, i3c_pplow_ns;
	uint32_t src_clk_ns;

	if (cfg == NULL) {
		LOG_ERR("Freq config NULL");
		return -EINVAL;
	}

	if ((pp_baudrate_hz == 0) || (pp_baudrate_hz > I3C_SCL_PP_FREQ_MAX_MHZ) ||
	    (od_baudrate_hz == 0) || (od_baudrate_hz > I3C_SCL_OD_FREQ_MAX_MHZ)) {
		LOG_ERR("I3C PP_SCL should within 12.5 Mhz, input: %d", pp_baudrate_hz);
		LOG_ERR("I3C OD_SCL should within 4.17 Mhz, input: %d", od_baudrate_hz);
		return -EINVAL;
	}

	/*
	 * PPBAUD(pp-high) = number of i3c source clock period in one I3C_SCL high
	 * period for I3C push-pull operation.
	 * For example, 48Mhz = 20.8 ns, 96Mhz = 10.4 ns
	 */

	/* Source clock period */
	src_clk_ns = (uint32_t)(NSEC_PER_SEC / i3c_src_clk);

	/* Fixed PPLOW(pp-low) = 0, 50% duty cycle for push-pull */
	i3c_pphigh_ns = (uint32_t)(NSEC_PER_SEC / pp_baudrate_hz) / 2UL;

	div = i3c_pphigh_ns / src_clk_ns;
	div = (div == 0UL) ? 1UL : div;
	if (i3c_pphigh_ns % src_clk_ns != 0) {
		div++;
	}

	if (div > PPBAUD_DIV_MAX) {
		LOG_ERR("PPBAUD(%d) out of range", div);
		return -EINVAL;
	}

	/*
	 * 0x0 = one source clock period for pp-high
	 * 0x1 = two source clock period for pp-high
	 * 0x2 = three source clock period for pp-high
	 * ...
	 */
	i3c_ppbaud = div - 1UL;

	/* Record calculation result , 50% duty-cycle */
	i3c_pphigh_ns = src_clk_ns * div;
	i3c_pplow_ns = i3c_pphigh_ns;

	/* Check PP low period in spec (should be the same as PPHIGH) */
	if (i3c_pplow_ns < I3C_BUS_TLOW_PP_MIN_NS) {
		LOG_ERR("PPLOW(%d) ns out of spec", i3c_pplow_ns);
		return -EINVAL;
	}

	/*
	 * odbaud = Number of PPBAUD periods (minus 1) in one I3C_SCL low period
	 * for I3C open-drain operation.
	 */

	/* Fixed ODHPP(od-high) = 1, calculate odlow_ns value */
	i3c_odlow_ns = (uint32_t)(NSEC_PER_SEC / od_baudrate_hz) - i3c_pphigh_ns;

	/* pphigh_ns = PPBAUD periods */
	div = i3c_odlow_ns / i3c_pphigh_ns;
	div = (div == 0UL) ? 1UL : div;
	if (i3c_odlow_ns % i3c_pphigh_ns != 0) {
		div++;
	}

	/* 0x0 = one PPBAUD period
	 * 0x1 = two PPBAUD period
	 * 0x2 = three PPBAUD period
	 * ...
	 */
	i3c_odbaud = div - 1UL;

	/* Record calculation result, odhpp = pphpp */
	i3c_odlow_ns = i3c_pphigh_ns * div;

	/* Check OD low period in spec */
	if (i3c_odlow_ns < I3C_BUS_TLOW_OD_MIN_NS) {
		LOG_ERR("ODBAUD(%d) ns out of spec", i3c_odlow_ns);
		return -EINVAL;
	}

	if (i2c_baudrate_hz != 0) {
		/* Calculate i2c baudrate periods */
		i2c_baud_ns = (uint32_t)(NSEC_PER_SEC / i2c_baudrate_hz);

		/* 50% duty-cycle */
		div = i2c_baud_ns / i3c_odlow_ns;
		if (i2c_baud_ns % i3c_odlow_ns != 0) {
			div++;
		}

		/*
		 * I2CBAUD = scl-high + scl-low
		 * (I2CBAUD >> 1) + 1 ==> scl-high
		 * (I2CBAUD >> 1) + 1 + lsb bit ==> scl-low
		 */
		i2c_baud = div - (1 << 1);

		if (div > PPBAUD_DIV_MAX) {
			LOG_ERR("I2C out of range");
			return -EINVAL;
		}
	} else {
		i2c_baud = 0;
	}

	cfg->pplow = 0;
	cfg->odhpp = 1;
	cfg->ppbaud = i3c_ppbaud;
	cfg->odbaud = i3c_odbaud;
	cfg->i2c_baud = i2c_baud;

	return 0;
}

static int nct_i3c_freq_init(const struct device *dev)
{
	const struct nct_i3c_config *config = dev->config;
	struct nct_i3c_data *data = dev->data;
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	const struct device *const clk_dev = DEVICE_DT_GET(NCT_PCC_NODE);
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;

	uint32_t scl_pp = ctrl_config->scl.i3c;
	uint32_t scl_od = config->clocks.i3c_od_scl_hz;
	uint32_t scl_i2c = ctrl_config->scl.i2c;
	struct nct_i3c_timing_cfg timing_cfg;
	uint32_t i3c_freq_rate;
	int ret;

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t *)config->clk_cfg,
				     &i3c_freq_rate);
	if (ret != 0x0) {
		LOG_ERR("Get I3C source clock fail %d", ret);
		return -EINVAL;
	}

	LOG_DBG("SCL_PP_FREQ MAX: %d", I3C_SCL_PP_FREQ_MAX_MHZ);
	LOG_DBG("SCL_OD_FREQ MAX: %d", I3C_SCL_OD_FREQ_MAX_MHZ);
	LOG_DBG("i3c_clk_freq: %d", i3c_freq_rate);
	LOG_DBG("scl_pp: %d", scl_pp);
	LOG_DBG("scl_od: %d", scl_od);
	LOG_DBG("scl_i2c: %d", scl_i2c);
	LOG_DBG("hdr: %d", ctrl_config->supported_hdr);

	if (i3c_freq_rate == I3C_CLK_FREQ_48_MHZ) {
		timing_cfg = nct_def_speed_cfg[I3C_CLK_FREQ_48MHZ];
	} else if (i3c_freq_rate == I3C_CLK_FREQ_96_MHZ) {
		timing_cfg = nct_def_speed_cfg[I3C_CLK_FREQ_96MHZ];
	} else {
		LOG_ERR("Unsupported i3c freq for %s. freq rate: %d", dev->name, i3c_freq_rate);
		return -EINVAL;
	}

	ret = nct_i3c_get_scl_config(&timing_cfg, i3c_freq_rate, scl_pp, scl_od, scl_i2c);
	if (ret != 0x0) {
		LOG_ERR("Adjust I3C frequency fail");
		return -EINVAL;
	}

	/* Apply SCL_PP and SCL_OD */
	SET_FIELD(i3c_inst->MCONFIG, NCT_I3C_MCONFIG_PPBAUD, timing_cfg.ppbaud);
	SET_FIELD(i3c_inst->MCONFIG, NCT_I3C_MCONFIG_PPLOW, timing_cfg.pplow);
	SET_FIELD(i3c_inst->MCONFIG, NCT_I3C_MCONFIG_ODBAUD, timing_cfg.odbaud);
	SET_FIELD(i3c_inst->MCONFIG, NCT_I3C_MCONFIG_I2CBAUD, timing_cfg.i2c_baud);

	if (timing_cfg.odhpp != 0) {
		i3c_inst->MCONFIG |= NCT_I3C_MCONFIG_ODHPP;
	} else {
		i3c_inst->MCONFIG &= ~NCT_I3C_MCONFIG_ODHPP;
	}

	LOG_DBG("ppbaud: %d", (int)GET_FIELD(i3c_inst->MCONFIG, NCT_I3C_MCONFIG_PPBAUD));
	LOG_DBG("odbaud: %d", (int)GET_FIELD(i3c_inst->MCONFIG, NCT_I3C_MCONFIG_ODBAUD));
	LOG_DBG("pplow: %d", (int)GET_FIELD(i3c_inst->MCONFIG, NCT_I3C_MCONFIG_PPLOW));
	LOG_DBG("odhpp: %d", !!(i3c_inst->MCONFIG & NCT_I3C_MCONFIG_ODHPP));
	LOG_DBG("i2c_baud: %d",
		(int)GET_FIELD(i3c_inst->MCONFIG, NCT_I3C_MCONFIG_I2CBAUD));

	return 0;
}

static int nct_i3c_controller_init(const struct device *dev, uint8_t mode)
{
	const struct nct_i3c_config *config = dev->config;
	struct nct_i3c_data *data = dev->data;
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	const struct device *const clk_dev = DEVICE_DT_GET(NCT_PCC_NODE);
	uint32_t i3c_freq_rate;
	uint8_t bamatch;
	int ret;

	/* Enable Controller */
	SET_FIELD(i3c_inst->MCONFIG, NCT_I3C_MCONFIG_CTRENA, mode);
	/* No need to set the MCONFIG register in off mode */
	if (mode == MCONFIG_CTRENA_OFF) {
		return 0;
	}

	/* Disable all interrupts */
	nct_i3c_interrupt_all_disable(i3c_inst);

	/* Initial baudrate. */
	if (nct_i3c_freq_init(dev) != 0x0) {
		return -EINVAL;
	}

	/* Enable open-drain stop */
	i3c_inst->MCONFIG |= BIT(NCT_I3C_MCONFIG_ODSTOP);
	/* Enable timeout */
	i3c_inst->MCONFIG &= ~BIT(NCT_I3C_MCONFIG_DISTO);
	/* Flush tx and tx FIFO buffer */
	nct_i3c_controller_fifo_flush(i3c_inst);

	/* Set bus available match value in target register */
	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t *)config->clk_cfg,
				     &i3c_freq_rate);
	LOG_DBG("I3C_CLK_FREQ: %d", i3c_freq_rate);

	if (ret != 0x0) {
		LOG_ERR("Get I3C source clock fail %d", ret);
		return -EINVAL;
	}

	bamatch = DIV_ROUND_UP(i3c_freq_rate, MHZ(1));
	SET_FIELD(i3c_inst->CONFIG, NCT_I3C_CONFIG_BAMATCH, bamatch);

	if (mode == MCONFIG_CTRENA_ON) {
		struct i3c_config_target *config_target = &data->config_target;
		config_target->enable = false;
	}

	return 0;
}

#define NCT_I3C_CONFIG_HDRCMD_RD_FROM_FIFO   0x0
#define NCT_I3C_CONFIG_HDRCMD_RD_FROM_HDRCMD 0x1
static int nct_i3c_target_init(const struct device *dev)
{
	const struct nct_i3c_config *config = dev->config;
	struct nct_i3c_data *data = dev->data;
	struct i3c_config_target *config_target = &data->config_target;
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	const struct device *const clk_dev = DEVICE_DT_GET(NCT_PCC_NODE);
	uint32_t i3c_freq_rate;
	uint8_t bamatch;
	int ret;
	uint64_t pid;

	/* Make sure Slave Enable is not set when setting up target */
	i3c_inst->CONFIG &= ~BIT(NCT_I3C_CONFIG_TGTENA);

	/* Set bus available match value in target register */
	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t *)config->clk_cfg,
				     &i3c_freq_rate);
	LOG_DBG("I3C_CLK_FREQ: %d", i3c_freq_rate);

	if (ret != 0x0) {
		LOG_ERR("Get I3C source clock fail %d", ret);
		return -EINVAL;
	}

	bamatch = DIV_ROUND_UP(i3c_freq_rate, MHZ(1));
	SET_FIELD(i3c_inst->CONFIG, NCT_I3C_CONFIG_BAMATCH, bamatch);

	/* Set Provisional ID */
	pid = config_target->pid;
	/* PID[47:33] MIPI manufacturer ID */
	SET_FIELD(i3c_inst->VENDORID, NCT_I3C_VENDORID_VID,
			   (uint32_t)GET_PID_VENDOR_ID(pid));

	/* PID[32] Vendor fixed value(0) or random value(1) */
	if (config_target->pid_random) {
		i3c_inst->CONFIG |= BIT(NCT_I3C_CONFIG_IDRAND);
	} else {
		i3c_inst->CONFIG &= ~BIT(NCT_I3C_CONFIG_IDRAND);
	}

	/* PID[31:0] vendor fixed value */
	i3c_inst->PARTNO = (uint32_t)GET_PID_PARTNO(pid);

	LOG_DBG("pid: %#llx", pid);
	LOG_DBG("vendro id: %#x", (uint32_t)GET_PID_VENDOR_ID(pid));
	LOG_DBG("id type: %d", (uint32_t)GET_PID_ID_TYP(pid));
	LOG_DBG("partno: %#x", (uint32_t)GET_PID_PARTNO(pid));

	SET_FIELD(i3c_inst->IDEXT, NCT_I3C_IDEXT_DCR, config_target->dcr);
	SET_FIELD(i3c_inst->IDEXT, NCT_I3C_IDEXT_BCR, config_target->bcr);
	SET_FIELD(i3c_inst->CONFIG, NCT_I3C_CONFIG_SADDR, config_target->static_addr);
	//i3c_inst->CONFIG &= ~BIT(NCT_I3C_CONFIG_HDRCMD);
	i3c_inst->CONFIG |= BIT(NCT_I3C_CONFIG_HDRCMD);
	SET_FIELD(i3c_inst->MAXLIMITS, NCT_I3C_MAXLIMITS_MAXRD,
			   (config_target->max_read_len) & 0xfff);
	SET_FIELD(i3c_inst->MAXLIMITS, NCT_I3C_MAXLIMITS_MAXWR,
			   (config_target->max_write_len) & 0xfff);

	LOG_DBG("static addr: %#x", config_target->static_addr);
	LOG_DBG("max read len: %d", config_target->max_read_len);
	LOG_DBG("max write len: %d", config_target->max_write_len);

	/* Ignore DA and detect all START and STOP */
	i3c_inst->CONFIG &= ~BIT(NCT_I3C_CONFIG_MATCHSS);

	/* Enable the target interrupt events */
	nct_i3c_enable_target_interrupt(dev, true);

	/* Target mode enable */
	i3c_inst->CONFIG |= BIT(NCT_I3C_CONFIG_TGTENA);

	config_target->enable = true;

	/* setup rx dma in advance to prevent data loss for RX FIFO is too small */
	init_i3c_slave_rx_payload(dev);
	nct_i3c_target_rx_read(dev);

	/* Flush target rx and tx fifo */
	nct_i3c_target_tx_fifo_flush(i3c_inst);
	nct_i3c_target_rx_fifo_flush(i3c_inst);

	return 0;
}

static void nct_i3c_dev_init(const struct device *dev)
{
	struct nct_i3c_data *data = dev->data;
	struct i3c_config_controller *config_cntlr = &data->common.ctrl_config;
	struct i3c_config_target *config_target = &data->config_target;

	/* Reset I3C module */
	nct_i3c_reset_module(dev);

	if (I3C_BCR_DEVICE_ROLE(config_target->bcr) == I3C_BCR_DEVICE_ROLE_I3C_CONTROLLER_CAPABLE) {
		if (config_cntlr->is_secondary) {
			LOG_DBG("Secondary controller");
			/* Set Secondary controller boot as a target */
			nct_i3c_controller_init(dev, MCONFIG_CTRENA_CAPABLE);

			/* Set I3C target */
			nct_i3c_target_init(dev);
		} else {
			LOG_DBG("Primary controller");
			/* Set Primary controller */
			nct_i3c_controller_init(dev, MCONFIG_CTRENA_ON);
		}
	} else {
		LOG_DBG("I3C target");
		/* Disable I3C controller */
		nct_i3c_controller_init(dev, MCONFIG_CTRENA_OFF);

		/* Set I3C target */
		nct_i3c_target_init(dev);
	}
}

static int nct_i3c_configure(const struct device *dev, enum i3c_config_type type, void *config)
{
	struct nct_i3c_data *dev_data = dev->data;
	struct i3c_config_controller *cntlr_cfg = config;
	struct i3c_config_target *config_target;

	if (type == I3C_CONFIG_CONTROLLER) {
		/*
		 * Check for valid configuration parameters.
		 * Currently, must be the primary controller.
		 */
		if ((cntlr_cfg->is_secondary) || (cntlr_cfg->scl.i3c == 0U)) {
			return -EINVAL;
		}

		/* Save requested config to dev */
		(void)memcpy(&dev_data->common.ctrl_config, cntlr_cfg, sizeof(*cntlr_cfg));

		/* Controller init */
		return nct_i3c_controller_init(dev, MCONFIG_CTRENA_ON);
	} else if (type == I3C_CONFIG_TARGET) {
		config_target = config;

		if (config_target->pid == 0) {
			LOG_ERR("configure target failed");
		return -EINVAL;
	}

		nct_i3c_target_init(dev);
	}

	LOG_ERR("Not supported mode %d", type);
	return -EINVAL;
}

static int nct_i3c_config_get(const struct device *dev, enum i3c_config_type type, void *config)
{
	struct nct_i3c_data *data = dev->data;

	if ((type != I3C_CONFIG_CONTROLLER) || (config == NULL)) {
		return -EINVAL;
	}

	(void)memcpy(config, &data->common.ctrl_config, sizeof(data->common.ctrl_config));


	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	uint32_t mstatus;
	uint32_t mintset;
	uint32_t mintmask;


	mstatus = i3c_inst->MSTATUS;
	mintset = i3c_inst->MINTSET;
	mintmask = i3c_inst->MINTMASKED;

	if (IS_BIT_SET(mstatus, NCT_I3C_MSTATUS_TGTSTART)) {
//		IRQ_CONNECT(0x44, 0x03, nct_i3c_isr, dev, 0);
//		irq_enable(0x44);
return 2;		
	} else {
return 1;
	}

	return 0;
}

static int nct_i3c_init(const struct device *dev)
{
	const struct nct_i3c_config *config = dev->config;
	struct nct_i3c_data *data = dev->data;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	const struct device *const clk_dev = DEVICE_DT_GET(NCT_PCC_NODE);
	int ret;

	LOG_DBG("%s", dev->name);

	/* Check clock device ready */
	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s Clk device not ready", clk_dev->name);
		return -ENODEV;
	}

	/* Set I3C_PD operational */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t *)config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on I3C clock fail %d", ret);
		return ret;
	}

	/* Apply pin-muxing */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("Apply pinctrl fail %d", ret);
		return ret;
	}

	/* Lock initial */
	k_mutex_init(&data->lock_mutex);
	k_sem_init(&data->sync_sem, 0, 1);
	k_sem_init(&data->ibi_lock_sem, 1, 1);
	k_sem_init(&data->target_lock_sem, 1, 1);
	k_sem_init(&data->target_event_lock_sem, 1, 1);

	ret = i3c_addr_slots_init(dev);
	if (ret != 0) {
		LOG_ERR("Addr slots init fail %d", ret);
		return ret;
	}

	/* Configure I3C controller */
	ctrl_config->scl.i3c = config->clocks.i3c_pp_scl_hz;	/* Set I3C frequency */
	ctrl_config->scl.i2c = config->clocks.i2c_scl_hz;	/* Set I2C frequency */

	/* Initial I3C device as controller or target */
	nct_i3c_dev_init(dev);

	/* Just in case the bus is not in idle. */
	if (GET_FIELD(i3c_inst->MCONFIG, NCT_I3C_MCONFIG_CTRENA) ==
	    MCONFIG_CTRENA_ON) {
	ret = nct_i3c_recover_bus(dev);
	if (ret != 0) {
		LOG_ERR("Apply i3c_recover_bus() fail %d", ret);
		return ret;
	}
	}

	/* Configure interrupt */
	config->irq_config_func(dev);

	/* Initialize driver status machine */
	set_oper_state(dev, I3C_OP_STATE_IDLE);

	/* Check I3C is controller mode and target device exist in device tree */
	if ((config->common.dev_list.num_i3c > 0) &&
	    (GET_FIELD(i3c_inst->MCONFIG, NCT_I3C_MCONFIG_CTRENA) ==
	     MCONFIG_CTRENA_ON)) {
		/* Perform bus initialization */
		ret = i3c_bus_init(dev, &config->common.dev_list);
		if (ret != 0) {
			LOG_ERR("Apply i3c_bus_init() fail %d", ret);
			return ret;
		}
	}

	return 0;
}

static int nct_i3c_i2c_api_configure(const struct device *dev, uint32_t dev_config)
{
	return -ENOSYS;
}

static int nct_i3c_i2c_api_transfer(const struct device *dev, struct i2c_msg *msgs,
				     uint8_t num_msgs, uint16_t addr)
{
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	uint32_t intmask;
	int xfered_len, ret;
	bool is_xfer_done = true;

	nct_i3c_mutex_lock(dev);

	if (WAIT_FOR((nct_i3c_state_get(i3c_inst) == MSTATUS_STATE_IDLE),
		     I3C_CHK_TIMEOUT_US, NULL) == false) {
		LOG_ERR("xfer state error: %d", nct_i3c_state_get(i3c_inst));
		nct_i3c_mutex_unlock(dev);
		return -ETIMEDOUT;
	}

	/* Disable interrupt */
	intmask = i3c_inst->MINTSET;
	nct_i3c_interrupt_all_disable(i3c_inst);

	nct_i3c_xfer_reset(i3c_inst);

	/* Iterate over all the messages */
	for (int i = 0; i < num_msgs; i++) {
		bool is_rx = (msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
		bool no_ending = false;

		/*
		 * Emit start if this is the first message or that
		 * the RESTART flag is set in message.
		 */
		bool emit_start =
			(i == 0) || ((msgs[i].flags & I2C_MSG_RESTART) == I2C_MSG_RESTART);

		bool emit_stop = (msgs[i].flags & I2C_MSG_STOP) == I2C_MSG_STOP;

		/*
		 * The controller requires special treatment of last byte of
		 * a write message. Since the API permits having a bunch of
		 * write messages without RESTART in between, this is just some
		 * logic to determine whether to treat the last byte of this
		 * message to be the last byte of a series of write mssages.
		 * If not, tell the write function not to treat it that way.
		 */
		if (!is_rx && !emit_stop && ((i + 1) != num_msgs)) {
			bool next_is_write = (msgs[i + 1].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE;
			bool next_is_restart =
				((msgs[i + 1].flags & I2C_MSG_RESTART) == I2C_MSG_RESTART);

			if (next_is_write && !next_is_restart) {
				no_ending = true;
			}
		}

#ifdef CONFIG_I3C_NCT_DMA
                /* Do transfer with target device */
		xfered_len = nct_i3c_do_one_xfer_dma(dev, addr, NCT_I3C_MCTRL_TYPE_I2C, msgs[i].buf,
						      msgs[i].len, is_rx, emit_start, emit_stop,
						      no_ending);
#else
		xfered_len =
			nct_i3c_do_one_xfer(dev, addr, NCT_I3C_MCTRL_TYPE_I2C, msgs[i].buf,
					     msgs[i].len, is_rx, emit_start, emit_stop, no_ending);
#endif
		if (xfered_len < 0) {
			LOG_ERR("do xfer fail");
			ret = xfered_len;
			goto out_xfer_i2c_stop_unlock;
		}

		/* Check emit stop flag including in the final msg */
		if ((i == num_msgs - 1) && (emit_stop == false)) {
			is_xfer_done = false;
		}
	}

	ret = 0;

out_xfer_i2c_stop_unlock:
	/* Emit stop if error occurs or stop flag not in the msg */
	if ((ret != 0) || (is_xfer_done == false)) {
		nct_i3c_request_emit_stop(i3c_inst);
	}

	nct_i3c_errwarn_clear_all(i3c_inst);
	nct_i3c_status_clear_all(i3c_inst);

	nct_i3c_interrupt_enable(i3c_inst, intmask);
	nct_i3c_mutex_unlock(dev);

	return ret;
}

/*
 * brief:  I3C target write data to controller.
 *
 * param[in] dev       Pointer to controller device driver instance.
 * param[in] buf       Buffer containing data to be sent or received.
 *                     The caller must ensure that the buffer is valid until the
 *                     transaction is completed (STOP or Sr received).
 * param[in] len       Number of bytes in buf to send or receive.
 * param[in] hdr_mode  HDR mode.
 *
 * return  Number of bytes transferred, or negative if error.
 */
static int nct_i3c_target_tx_write(const struct device *dev, uint8_t *buf, uint16_t len,
				    uint8_t hdr_mode)
{
	int ret = 0;

	if ((buf == NULL) || (len == 0)) {
		LOG_ERR("Data buffer configuration failed");
		return -EINVAL;
	}

	if (hdr_mode != 0) {
		LOG_ERR("HDR not supported");
		return -ENOSYS;
	}

#ifdef CONFIG_I3C_NCT_DMA
	struct nct_i3c_data *data = dev->data;
	bool is_rx = false;
	bool no_ending = false;

	data->tx_valid = true;

	ret = nct_i3c_target_do_request_dma(dev, is_rx, buf, len, no_ending);
	if (ret < 0) {
		data->tx_valid = false;
		LOG_ERR("do xfer fail");
	}

	return ret;
#else
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct nct_i3c_data *data = dev->data;
	bool no_ending;

	/* Flush buffer, make sure no dummy data remain. */
	nct_i3c_target_tx_fifo_flush(i3c_inst);

	/* Tx buffer point to user buffer */
	data->tx_buf = buf;

	/* Check buffer length */
	if (len > I3C_FIFO_SIZE) {
		no_ending = true;
		data->tx_len = I3C_FIFO_SIZE;
	} else {
		no_ending = false;
		data->tx_len = len;
	}

	/* Write tx fifo */
	ret = nct_i3c_xfer_target_write_fifo(i3c_inst, data->tx_buf, data->tx_len, no_ending);
	if (ret < 0) {
		LOG_ERR("do xfer fail");
	} else if (len > ret) {
		/* Update buffer index */
		data->tx_buf += ret;
		data->tx_len = len - ret;

		/* Update remain len */
		ret = data->tx_len;
	}

	return ret;
#endif
}

static int nct_i3c_target_register(const struct device *dev, struct i3c_target_config *cfg)
{
	struct nct_i3c_data *data = dev->data;

	data->target_config = cfg;

	return 0;
}

static int nct_i3c_target_unregister(const struct device *dev, struct i3c_target_config *cfg)
{
	struct nct_i3c_data *data = dev->data;

	data->target_config = NULL;

	return 0;
}

static const struct i3c_driver_api nct_i3c_driver_api = {
	.i2c_api.configure = nct_i3c_i2c_api_configure,
	.i2c_api.transfer = nct_i3c_i2c_api_transfer,
	.i2c_api.recover_bus = nct_i3c_recover_bus,
	.configure = nct_i3c_configure,
	.config_get = nct_i3c_config_get,
	.recover_bus = nct_i3c_recover_bus,
	.do_daa = nct_i3c_do_daa,
	.do_ccc = nct_i3c_do_ccc,
	.i3c_device_find = nct_i3c_device_find,
	.i3c_xfers = nct_i3c_transfer,

	.target_tx_write = nct_i3c_target_tx_write,
	.target_register = nct_i3c_target_register,
	.target_unregister = nct_i3c_target_unregister,
#ifdef CONFIG_I3C_USE_IBI
	.ibi_enable = nct_i3c_ibi_enable,
	.ibi_disable = nct_i3c_ibi_disable,
	.ibi_raise = nct_i3c_target_ibi_raise,
#endif
};

#define DT_INST_TGT_PID_PROP_OR(id, prop, idx)                                                     \
	COND_CODE_1(DT_INST_PROP_HAS_IDX(id, prop, idx), (DT_INST_PROP_BY_IDX(id, prop, idx)), (0))
#define DT_INST_TGT_PID_RAND_PROP_OR(id, prop, idx)                                                \
	COND_CODE_1(DT_INST_PROP_HAS_IDX(id, prop, idx),                                           \
		    IS_BIT_SET(DT_INST_PROP_BY_IDX(id, prop, 0), 0), (0))

#define I3C_NCT_DEVICE(inst)                                                         \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static void nct_i3c_irq_config_##inst(const struct device *dev)                           \
	{                                                                                             \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), nct_i3c_isr,         \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	};                                                                                            \
                                                                                                   \
	static struct i3c_device_desc nct_i3c_device_array_##inst[] =                             \
		I3C_DEVICE_ARRAY_DT_INST(inst);                                                    \
                                                                                                   \
	static struct i3c_i2c_device_desc nct_i3c_i2c_device_array_##inst[] =                     \
		I3C_I2C_DEVICE_ARRAY_DT_INST(inst);                                                \
                                                                                                   \
	static const struct nct_i3c_config nct_i3c_config_##inst = {                             \
		.base = (struct i3c_reg *)DT_INST_REG_ADDR(inst),                                  \
		.clk_cfg = DT_INST_PHA(inst, clocks, clk_cfg),                                     \
		.irq_config_func = nct_i3c_irq_config_##inst,                                     \
		.common.dev_list.i3c = nct_i3c_device_array_##inst,                               \
		.common.dev_list.num_i3c = ARRAY_SIZE(nct_i3c_device_array_##inst),               \
		.common.dev_list.i2c = nct_i3c_i2c_device_array_##inst,                           \
		.common.dev_list.num_i2c = ARRAY_SIZE(nct_i3c_i2c_device_array_##inst),           \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.clocks.i3c_pp_scl_hz = DT_INST_PROP_OR(inst, i3c_scl_hz, 0),                      \
		.clocks.i3c_od_scl_hz = DT_INST_PROP_OR(inst, i3c_od_scl_hz, 0),                   \
		.clocks.i2c_scl_hz = DT_INST_PROP_OR(inst, i2c_scl_hz, 0),                         \
		.priv_xfer_pec = DT_INST_PROP_OR(inst, priv_xfer_pec, false), 						\
		.ibi_append_pec = DT_INST_PROP_OR(inst, ibi_append_pec, false), 					\
		IF_ENABLED(CONFIG_I3C_NCT_DMA, (                                                     \
			.pdma_rx = (struct pdma_dsct_reg *)DT_INST_REG_ADDR_BY_IDX(inst, 1), \
			.pdma_tx = (struct pdma_dsct_reg *)DT_INST_REG_ADDR_BY_IDX(inst, 2), \
			)) };                  \
	static struct nct_i3c_data nct_i3c_data_##inst = {                                       \
		.common.ctrl_config.is_secondary = DT_INST_PROP_OR(inst, secondary, false),        \
		.config_target.static_addr = DT_INST_PROP_OR(inst, static_address, 0),             \
		.config_target.pid = ((uint64_t)DT_INST_TGT_PID_PROP_OR(inst, tgt_pid, 0) << 32) | \
				     DT_INST_TGT_PID_PROP_OR(inst, tgt_pid, 1),                    \
		.config_target.pid_random = DT_INST_TGT_PID_RAND_PROP_OR(inst, tgt_pid, 0),        \
		.config_target.bcr = DT_INST_PROP(inst, bcr),                                      \
		.config_target.dcr = DT_INST_PROP_OR(inst, dcr, 0),                                \
		.config_target.max_read_len = DT_INST_PROP_OR(inst, maximum_read, 256),              \
		.config_target.max_write_len = DT_INST_PROP_OR(inst, maximum_write, 256),            \
		.config_target.supported_hdr = false,                                              \
	};                                                                                            \
	DEVICE_DT_INST_DEFINE(inst, nct_i3c_init, NULL, &nct_i3c_data_##inst,                    \
			      &nct_i3c_config_##inst, POST_KERNEL,                        \
			      CONFIG_I3C_CONTROLLER_INIT_PRIORITY, &nct_i3c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I3C_NCT_DEVICE)

/*************************************************************************************************/
// For v2.6 mctp

int i3c_nct_master_request_ibi(struct i3c_device_desc *i3cdev, struct i3c_ibi_callbacks *cb)
{
	// For 2.6
	struct i3c_nct_ibi_priv *ibi_priv;

	if (i3cdev->controller_priv != NULL) {
		LOG_ERR("IBI already registered for device %s", i3cdev->dev->name);
		return -EALREADY;
	}

	ibi_priv = (struct i3c_nct_ibi_priv *)malloc(sizeof(struct i3c_nct_ibi_priv) * 1);
	ibi_priv->ibi.enable = false;
	ibi_priv->ibi.callbacks = cb;
	ibi_priv->ibi.context = i3cdev;
	ibi_priv->ibi.incomplete = NULL;

	i3cdev->controller_priv = ibi_priv;
	return 0;
}

int i3c_nct_slave_register(const struct device *dev, struct i3c_slave_setup *slave_data)
{
	struct nct_i3c_data *data = dev->data;

	__ASSERT(slave_data->max_payload_len <= CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE,
		"msg_size should less than %d.\n", CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE);

	data->slave_data.max_payload_len = slave_data->max_payload_len;
	data->slave_data.callbacks = slave_data->callbacks;
	data->slave_data.dev = slave_data->dev;

	return 0;
}


int i3c_nct_slave_put_read_data(const struct device *dev, struct i3c_slave_payload *payload,
				   struct i3c_ibi_payload *ibi_notify)
{
//	const struct nct_i3c_config *config = dev->config;
	struct nct_i3c_data *data = dev->data;
	uint32_t event_en;
	int ret;
//	uint8_t *xfer_buf;
	struct i3c_reg *i3c_inst = HAL_INSTANCE(dev);
	struct i3c_ibi request;
	uint8_t bcr;

	__ASSERT_NO_MSG(payload);
	__ASSERT_NO_MSG(payload->buf);
	__ASSERT_NO_MSG(payload->size);

	k_mutex_lock(&data->lock_mutex, K_FOREVER);

//	if (config->priv_xfer_pec) {
//		uint8_t pec_v;
//		uint8_t addr_rnw;
//	 
//	 	addr_rnw = (uint8_t)I3C_GET_REG_DYNADDR(port) >> 1;
//	 	pec_v = crc8_ccitt(0, &addr_rnw, 1);
//	 
//	 	xfer_buf = (uint8_t *)&data->buf[1];
//	 	pec_v = crc8_ccitt(pec_v, xfer_buf, data->size - 1);
//	 	LOG_DBG("pec = %x", pec_v);
//	 	xfer_buf = (uint8_t *)&data->buf[0];
//	 	xfer_buf[data->size] = pec_v;
//	 	nct_i3c_target_tx_write(dev, data->buf, data->size + 1);
//	} else {
		nct_i3c_target_tx_write(dev, payload->buf, payload->size, 0);
//	}

	target_register_tx_fifo_empty_cb(tx_fifo_empty_handler);

	if (ibi_notify) {
		ret = i3c_slave_get_event_enabling(dev, &event_en);
		if (ret || !(event_en & I3C_SLAVE_EVENT_SIR)) {
			/* master should polling pending interrupt by GetSTATUS */
			SET_FIELD(i3c_inst->CTRL, NCT_I3C_CTRL_PENDINT, 0x01);
			k_mutex_unlock(&data->lock_mutex);
			return 0;
		}

		bcr = data->config_target.bcr;
		if (!(bcr & I3C_BCR_IBI_REQUEST_CAPABLE)) {
			LOG_ERR("Device is not IBI request capable");
			k_mutex_unlock(&data->lock_mutex);
			return -EINVAL;
		}

		if (bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE)
		{
			if (ibi_notify->payload_len == 0) {
				LOG_ERR("IBI payload length is zero, but BCR indicates it should have data byte");
				k_mutex_unlock(&data->lock_mutex);
				return -EINVAL;
			}

//			if (config->ibi_append_pec) {
//				pec = calculate_pec(ibi_notify->payload, ibi_notify->payload_len);
//				ibi_notify->payload[ibi_notify->payload_len] = pec;
//				ibi_notify->payload_len++;
//			} 
		}
		else if (ibi_notify->payload_len > 0) {
			LOG_ERR("IBI payload length is not zero, but BCR indicates it should not have data byte");
			k_mutex_unlock(&data->lock_mutex);
			return -EINVAL;
		}

		request.ibi_type = I3C_IBI_TARGET_INTR;
		request.payload = ibi_notify->payload;
		request.payload_len = ibi_notify->payload_len;
		nct_i3c_target_ibi_raise(dev, &request);
	}

	/*
	 * osEventFlagsClear(obj->data_event, ~osFlagsError);
	 * if (config->priv_xfer_pec) {
	 *   xfer_buf = pec_append(dev, data->buf, data->size);
	 *   i3c_npcm4xx_wr_tx_fifo(obj, xfer_buf, data->size + 1);
	 *   k_free(xfer_buf);
	 * } else {
	 *   i3c_npcm4xx_wr_tx_fifo(obj, data->buf, data->size);
	 * }
	 */

	k_mutex_unlock(&data->lock_mutex);

	return 0;
}

int i3c_nct_slave_get_dynamic_addr(const struct device *dev, uint8_t *dynamic_addr)
{
	struct i3c_reg *i3c_inst;

	if (dev == NULL) {
		LOG_ERR("Device is NULL");
		return -EINVAL;
	}

	if (dynamic_addr == NULL) {
		LOG_ERR("dynamic_addr is NULL");
		return -EINVAL;
	}

	i3c_inst = HAL_INSTANCE(dev);
	*dynamic_addr = GET_FIELD(i3c_inst->DYNADDR, NCT_I3C_DYNADDR_DADDR);
	return 0;
}

int i3c_nct_slave_get_event_enabling(const struct device *dev, uint32_t *event_en)
{
	struct i3c_reg *i3c_inst;
	uint32_t val;

	if (dev == NULL) {
		LOG_ERR("Device is NULL");
		return -EINVAL;
	}

	if (event_en == NULL) {
		LOG_ERR("event_en is NULL");
		return -EINVAL;
	}

	i3c_inst = HAL_INSTANCE(dev);
	val = 0;

	if (!IS_BIT_SET(i3c_inst->STATUS, NCT_I3C_STATUS_IBIDIS)) {
		val |= I3C_SLAVE_EVENT_SIR;
	}

	if (!IS_BIT_SET(i3c_inst->STATUS, NCT_I3C_STATUS_MRDIS)) {
		val |= I3C_SLAVE_EVENT_MR;
	}
	
	if (!IS_BIT_SET(i3c_inst->STATUS, NCT_I3C_STATUS_HJDIS)) {
		val |= I3C_SLAVE_EVENT_HJ;
	}

	*event_en = val;
	return 0;
}
/*************************************************************************************************/