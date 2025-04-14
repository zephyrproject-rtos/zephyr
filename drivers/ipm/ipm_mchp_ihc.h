/*
 * Copyright (c) 2024 Microchip Technology Inc
 *
 * SPDX-License-Identifier: (GPL-2.0 OR MIT)
 */

#ifndef ZEPHYR_DRIVERS_IPM_MCHP_IHC_H_
#define ZEPHYR_DRIVERS_IPM_MCHP_IHC_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/device.h>

/***************/
/* IHC Register*/
/***************/
/** @brief Microchip IHC Version	register offset */
 #define MCHP_IHC_REGS_IP_VERSION_OFFSET 	(0x0003BFC)

/************************/
/* IHC Module Registers */
/************************/


/** @brief IHC Module IRQ Mask offset from IHCM base, enable/disable Module IRQ */
#define MCHP_IHCM_REGS_IRQ_MASK_OFFSET		(0U)
/** @brief IHC Module IRQ Status offset from IHCM base, pending Module IRQ */
#define MCHP_IHCM_REGS_IRQ_STATUS_OFFSET	(0x8U)

#define MCHP_IHC_REGS_IRQ_STATUS_MP_OFFSET(remote_hart_id) \
	(remote_hart_id * 2)
#define MCHP_IHC_REGS_IRQ_STATUS_MP_NS_MASK    (1U)
/**
 * @brief IHC Module IRQ STATUS register Message Present bit nask
 * @param remote_hart_id remote hart id from the channel point of view
 * @return bit nask for the Message Present IRQ status
 * @note Message Present is set when a message is sent and must be cleared by
 *       the receiver.
 * @note IRQ STATUS register contains IHC IRQ pending bits for all channels on that IHC Module
 */
#define MCHP_IHC_REGS_IRQ_STATUS_MP(remote_hart_id) \
	(MCHP_IHC_REGS_IRQ_STATUS_MP_NS_MASK << \
	 MCHP_IHC_REGS_IRQ_STATUS_MP_OFFSET(remote_hart_id))


#define MCHP_IHC_REGS_IRQ_STATUS_ACK_OFFSET(remote_hart_id) \
	(remote_hart_id * 2)
#define MCHP_IHC_REGS_IRQ_STATUS_ACK_NS_MASK    (2U)
/**
 * @brief IHC Module IRQ STATUS register Message Ack bit nask
 * @param remote_hart_id remote hart id from the channel point of view
 * @return bit nask for the Message Ack IRQ status
 * @note Message Ack is set when a message have been processed by receiver and
 *       must be cleared by the sender.
 * @note IRQ STATUS register contains IHC IRQ pending bits for all channels on that IHC Module
 */
#define MCHP_IHC_REGS_IRQ_STATUS_ACK(remote_hart_id) \
	(MCHP_IHC_REGS_IRQ_STATUS_ACK_NS_MASK << \
	 MCHP_IHC_REGS_IRQ_STATUS_ACK_OFFSET(remote_hart_id))

#define MCHP_IHC_REGS_IRQ_MASK_MP_OFFSET(remote_hart_id) \
	(remote_hart_id * 2)
#define MCHP_IHC_REGS_IRQ_MASK_MP_NS_MASK    (1U)
/**
 * @brief IHC Module IRQ MASK register Message Present bit nask
 * @param remote_hart_id remote hart id from the channel point of view
 * @return bit nask for the Message Present IRQ mask
 * @note Message Present is set when a message is sent and must be cleared by
 *       the receiver.
 * @note IRQ MASK register contains IHC IRQ enable/disable bits for all channels on that IHC Module
 */
#define MCHP_IHC_REGS_IRQ_MASK_MP(remote_hart_id) \
	(MCHP_IHC_REGS_IRQ_MASK_MP_NS_MASK << \
	 MCHP_IHC_REGS_IRQ_MASK_MP_OFFSET(remote_hart_id))


#define MCHP_IHC_REGS_IRQ_MASK_ACK_OFFSET(remote_hart_id) \
	(remote_hart_id * 2)
#define MCHP_IHC_REGS_IRQ_MASK_ACK_NS_MASK    (2U)
/**
 * @brief IHC Module IRQ MASK register Message Ack bit nask
 * @param remote_hart_id remote hart id from the channel point of view
 * @return bit nask for the Message Ack IRQ mask
 * @note Message Ack is set when a message have been processed by receiver
 * @note IRQ MASK register contains IHC IRQ enable/disable bits for all channels on that IHC Module
 */
#define MCHP_IHC_REGS_IRQ_MASK_ACK(remote_hart_id)\
	(MCHP_IHC_REGS_IRQ_MASK_ACK_NS_MASK << \
	 MCHP_IHC_REGS_IRQ_MASK_ACK_OFFSET(remote_hart_id))

/** @brief IHC Module  */
#define MCHP_IHC_REGS_IRQ_DISABLE_MASK		(0x0U)

/**************************/
/** IHC Channel Registers */
/**************************/

/** @brief IHC Channel control register	offset from IHCC base address */
#define MCHP_IHCC_REGS_CTRL_OFFSET    (0U)
/**
 * @brief IHC Channel debug ID register offset from IHCC base address
 * This register contains local hart id and remote hart id for the channel
 */
#define MCHP_IHCC_REGS_DEBUG_ID_OFFSET    (0x4U)
/**
 * @brief IHC Channel message depth register offset from IHCC base address
 * This register is used to indicate the maximum depth of the message queue in and out
 */
#define MCHP_IHCC_REGS_MSG_DEPTH_OFFSET    (0x8U)
/** @brief IHC Channel message in register offset from IHCC base address */
#define MCHP_IHCC_REGS_MSG_IN_OFFSET    (0x20U)
/** @brief IHC Channel message out register offset from IHCC base address */
#define MCHP_IHCC_REGS_MSG_OUT_OFFSET    (0x90U)

/**
 * @brief Field Name: ACKIE(AACKIE)
 * Field Desc: Message Ack Interrupt Enable. Interrupt to indicate to the
 * core accessing this channel that a message has been consumed and sends an
 * acknowledgment to this channel. Field Type: read-write
 */
#define MCHP_IHC_REGS_CH_CTRL_ACKIE_OFFSET	(5U)
#define MCHP_IHC_REGS_CH_CTRL_ACKIE_NS_MASK	(BIT_MASK(1))
#define MCHP_IHC_REGS_CH_CTRL_ACKIE_MASK \
	(MCHP_IHC_REGS_CH_CTRL_ACKIE_NS_MASK << MCHP_IHC_REGS_CH_CTRL_ACKIE_OFFSET)

/**
 * @brief Field Name: ACKCLR (AMPACK)
 * Field Desc: Ack received. Used to indicate that associated channel has
 * acknowledgment a message now message can be cleared. Field Type: read-write
 */
#define MCHP_IHC_REGS_CH_CTRL_ACKCLR_OFFSET	(4U)
#define MCHP_IHC_REGS_CH_CTRL_ACKCLR_NS_MASK	(BIT_MASK(1))
#define MCHP_IHC_REGS_CH_CTRL_ACKCLR_MASK \
	(MCHP_IHC_REGS_CH_CTRL_ACKCLR_NS_MASK << MCHP_IHC_REGS_CH_CTRL_ACKCLR_OFFSET)

/**
 * @brief Field Name: ACK (BMPACK)
 * Field Desc: Ack Sent. Used to indicate to associated channel that it has sent
 * a acknowledgment. Field Type: read-write
 */
#define MCHP_IHC_REGS_CH_CTRL_ACK_OFFSET	(3U)
#define MCHP_IHC_REGS_CH_CTRL_ACK_NS_MASK	(BIT_MASK(1))
#define MCHP_IHC_REGS_CH_CTRL_ACK_MASK \
	(MCHP_IHC_REGS_CH_CTRL_ACK_NS_MASK << MCHP_IHC_REGS_CH_CTRL_ACK_OFFSET)

/**
 * @brief Field Name: MPIE (AMPIE)
 * Field Desc: Message Present Interrupt Enable. Interrupt to indicate to the
 * core accessing this channel that a message is present and read the msg from
 * channel. Field Type: read-write
 */
#define MCHP_IHC_REGS_CH_CTRL_MPIE_OFFSET	(2U)
#define MCHP_IHC_REGS_CH_CTRL_MPIE_NS_MASK 	(BIT_MASK(1))
#define MCHP_IHC_REGS_CH_CTRL_MPIE_MASK \
	(MCHP_IHC_REGS_CH_CTRL_MPIE_NS_MASK << MCHP_IHC_REGS_CH_CTRL_MPIE_OFFSET)

/**
 * @brief Field Name: MP (AMP)
 * Field Desc: Message Present received. Used to indicate that it new message is
 * sent from associated channel. Field Type: read-write
 */
#define MCHP_IHC_REGS_CH_CTRL_MP_OFFSET		(1U)
#define MCHP_IHC_REGS_CH_CTRL_MP_NS_MASK	(BIT_MASK(1))
#define MCHP_IHC_REGS_CH_CTRL_MP_MASK \
	(MCHP_IHC_REGS_CH_CTRL_MP_NS_MASK << MCHP_IHC_REGS_CH_CTRL_MP_OFFSET)

/**
 * @brief Field Name: RMP (BMP)
 * Field Desc: Message send. Used to indicate to associated channel that it has
 * send new message. Field Type: read-write
 */
#define MCHP_IHC_REGS_CH_CTRL_RMP_OFFSET	(0U)
#define MCHP_IHC_REGS_CH_CTRL_RMP_NS_MASK	(BIT_MASK(1))
#define MCHP_IHC_REGS_CH_CTRL_RMP_MASK \
	(MCHP_IHC_REGS_CH_CTRL_RMP_NS_MASK << MCHP_IHC_REGS_CH_CTRL_RMP_OFFSET)

/**
 * @brief Field Name: HART_ID_LOCAL
 * Field Desc: Local hart id owning the channel.
 * Field Type: read-write
 */
#define MCHP_IHC_REGS_LOCAL_HART_ID_OFFSET	(0U)
#define MCHP_IHC_REGS_LOCAL_HART_ID_NS_MASK	(BIT_MASK(8))
#define MCHP_IHC_REGS_LOCAL_HART_ID_MASK \
	(MCHP_IHC_REGS_LOCAL_HART_ID_NS_MASK << MCHP_IHC_REGS_LOCAL_HART_ID_OFFSET)

/**
 * @brief Field Name: HART_ID_REMOTE
 * Field Desc: Remote hart id to which the channel is connected.
 * Field Type: read-only
 */
#define MCHP_IHC_REGS_REMOTE_HART_ID_OFFSET	(8U)
#define MCHP_IHC_REGS_REMOTE_HART_ID_NS_MASK	(BIT_MASK(8))
#define MCHP_IHC_REGS_REMOTE_HART_ID_MASK \
	(MCHP_IHC_REGS_REMOTE_HART_ID_NS_MASK << MCHP_IHC_REGS_REMOTE_HART_ID_OFFSET)

/**
 * @brief Field Name: MESSAGE_OUT
 * Field Desc: Write to indicate MSGOUT size to associated channel.
 * Field Type: read-write
 */
#define MCHP_IHC_REGS_MESSAGE_SIZE_MESSAGE_OUT_OFFSET	(0U)
#define MCHP_IHC_REGS_MESSAGE_SIZE_MESSAGE_OUT_NS_MASK	(BIT_MASK(8))
#define MCHP_IHC_REGS_MESSAGE_SIZE_MESSAGE_OUT_MASK \
	(MCHP_IHC_REGS_MESSAGE_SIZE_MESSAGE_OUT_NS_MASK << \
	 MCHP_IHC_REGS_MESSAGE_SIZE_MESSAGE_OUT_OFFSET)

/**
 * @brief Field Name: MESSAGE_IN
 * Field Desc: Read indication of MSGIN size from associated channel.
 * Field Type: read-only
 */
#define MCHP_IHC_REGS_MESSAGE_SIZE_MESSAGE_IN_OFFSET	(8U)
#define MCHP_IHC_REGS_MESSAGE_SIZE_MESSAGE_IN_NS_MASK	(BIT_MASK(8))
#define MCHP_IHC_REGS_MESSAGE_SIZE_MESSAGE_IN_MASK \
	(MCHP_IHC_REGS_MESSAGE_SIZE_MESSAGE_IN_NS_MASK << \
	 MCHP_IHC_REGS_MESSAGE_SIZE_MESSAGE_IN_OFFSET)

/** @brief IHC Channel reg map */
struct mchp_ihcc_reg_map_t {
	/** @brief (R/W) 0x00 control reg */
	volatile uint32_t ctrl;
	/** @brief (R/ ) 0x04 local_hart_id: my hart id, set at local init */
	const volatile uint32_t debug_id;
	/** @brief (R/ ) 0x08 Size of msg buffer instantiated in fabric */
	const volatile uint32_t message_depth;
	/** @brief (R/ ) 0x0C not used */
	const volatile uint32_t reserved1[5U];
	/** @brief (R/ ) 0x20  message in */
	volatile uint32_t msg_in[28];
	/** @brief (R/W) 0x90  message out */
	volatile uint32_t msg_out[28];
};

/** @brief IHC Module reg map */
struct mchp_ihcm_reg_map_t {
	/** @brief (R/W) 0x00  interrupt mask       */
	volatile uint32_t irq_mask;
	/** @brief (R/ ) 0x04  not used             */
	const volatile uint32_t reserved1;
	/** @brief (R/ ) 0x08  interrupt status     */
	const volatile uint32_t irq_status;
	/** @brief (R/ ) 0x0C  not used             */
	const volatile uint32_t reserved2[61];
};

/** @brief IHC configuration structure */
struct mchp_ihc_config {
	/** @brief IHC base address*/
	uintptr_t ihc_regs;
	/** @brief DT enabled IHCM (Inter Hart Communication Modul) child nodes*/
	const struct device ** ihcm_list;
	/** @brief Number of DT enabled IHCM */
	size_t num_ihcm;
};

/** @brief IHC Module configuration structure */
struct mchp_ihcm_config {
	/** @brief IHCM node base address*/
	uintptr_t ihcm_regs;
	/** @brief DT enabled IHCC (Inter Hart Communication Channel) child nodes*/
	const struct device ** ihcc_list;
	/** @brief Number of DT enabled IHCC */
	size_t num_ihcc;
	/** @brief IRQ index associated to the IHC Module*/
	uint32_t irq_idx;
	/** @brief Config function pointer mainly use for IRQ static configuration */
	int (*config_func)(void);
};

/** @brief IHC Module data structure */
struct mchp_ihcm_data {
	/** @brief IHC Moulde ISR counter (incremented for each Channel with irq enabled) */
	uint32_t isr_counter;
	/** @brief IHC Module mutex */
	struct k_mutex *module_lock;
	/** @brief IHC Channel callback list */
	ipm_callback_t *cb_list;
	/** @brief IHC Channel callback user data list */
	void **cb_user_data_list;
	/**
	 * @brief IHC Channel callback index list, used to map callback with channel.
	 * the index stored are the remote hart id stored in the same order than
	 * callback and user data.
	 */
	uint32_t *cb_idx_list;
	/**
	 * @brief Number of callback, in practice number of IHC Channel DT enabled
	 * in that IHC Module
	 */
	size_t num_cb;
};

/** @brief IHC Channel configuration structure */
struct mchp_ihcc_config {
	/** @brief Parent device, ie: associated IHC Module */
	const struct device *parent_node;
	/** @brief Grandparent device, ie: associated base IHC instance */
	const struct device *gparent_node;
	/** @brief IHC base register address*/
	uintptr_t ihc_regs;
	/** @brief IHC Module base register address*/
	uintptr_t ihcm_regs;
	/** @brief IHC Channel base register address*/
	uintptr_t ihcc_regs;
};

/** @brief IHC Channel data structure */
struct mchp_ihcc_data {
	/** @brief channel enable */
	bool enabled;
	/** @brief channel lock */
	struct k_mutex *channel_lock;
	/** @brief message ack flag */
	atomic_t ack;
};

#endif /* ZEPHYR_DRIVERS_IPM_MCHP_IHC_H_ */