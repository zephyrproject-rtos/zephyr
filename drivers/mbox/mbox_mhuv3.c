/*
 * SPDX-FileCopyrightText: <text>Copyright 2024-2025 Arm Limited and/or its
 * affiliates <open-source-office@arm.com></text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	arm_mhuv3

#include <zephyr/drivers/mbox.h>

#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL	CONFIG_MBOX_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbox_mhuv3);

/* ====== MHUv3 Registers ====== */

/* Maximum number of Doorbell channel windows */
#define MHUV3_DBCW_MAX		      128
/* Number of DBCH combined interrupt status registers */
#define MHUV3_DBCH_CMB_INT_ST_REG_CNT 4

/* Number of FFCH combined interrupt status registers */
#define MHUV3_FFCH_CMB_INT_ST_REG_CNT 2

#define MHUV3_FLAG_BITS		      32

#define MHUV3_MAJOR_VERSION	      2

/* Postbox/Mailbox Block Identifier */
#define BLK_ID_BLK_ID		   GENMASK(3, 0)
/* Postbox/Mailbox Feature Support 0 */
#define FEAT_SPT0_DBE_SPT	   GENMASK(3, 0)
#define FEAT_SPT0_FE_SPT	   GENMASK(7, 4)
#define FEAT_SPT0_FCE_SPT	   GENMASK(11, 8)
/* Postbox/Mailbox Feature Support 1 */
#define FEAT_SPT1_AUTO_OP_SPT      GENMASK(3, 0)
/* Postbox/Mailbox Doorbell Channel Configuration 0 */
#define DBCH_CFG0_NUM_DBCH	   GENMASK(7, 0)
/* Postbox/Mailbox FIFO Channel Configuration 0 */
#define FFCH_CFG0_NUM_FFCH	   GENMASK(7, 0)
#define FFCH_CFG0_P8BA_SPT	   BIT(8)
#define FFCH_CFG0_P16BA_SPT	   BIT(9)
#define FFCH_CFG0_P32BA_SPT	   BIT(10)
#define FFCH_CFG0_P64BA_SPT	   BIT(11)
#define FFCH_CFG0_FFCH_DEPTH	   GENMASK(25, 16)
/* Postbox/Mailbox Fast Channel Configuration 0 */
#define FCH_CFG0_NUM_FCH	   GENMASK(9, 0)
#define FCH_CFG0_FCGI_SPT	   BIT(10)	/* MBX-only */
#define FCH_CFG0_NUM_FCG	   GENMASK(15, 11)
#define FCH_CFG0_NUM_FCH_PER_FCG   GENMASK(20, 16)
#define FCH_CFG0_FCH_WS		   GENMASK(28, 21)
/* Postbox/Mailbox Control */
#define CTRL_OP_REQ		   BIT(0)
#define	CTRL_CH_OP_MSK		   BIT(1)
/* Mailbox Fast Channel Control */
#define FCH_CTRL_INT_EN		   BIT(2)
/* Postbox/Mailbox Implementer Identification Register */
#define IIDR_IMPLEMENTER	   GENMASK(11, 0)
#define IIDR_REVISION		   GENMASK(15, 12)
#define IIDR_VARIANT		   GENMASK(19, 16)
#define IIDR_PRODUCT_ID		   GENMASK(31, 20)
/* Postbox/Mailbox Architecture Identification Register */
#define AIDR_ARCH_MINOR_REV	   GENMASK(3, 0)
#define AIDR_ARCH_MAJOR_REV	   GENMASK(7, 4)
/* Postbox/Mailbox Doorbell/FIFO/Fast Channel Control */
#define XBCW_CTRL_COMB_EN	   BIT(0)
/* Postbox Doorbell Interrupt Status/Clear/Enable */
#define PDBCW_INT_TFR_ACK	   BIT(0)

/* Padding bitfields/fields represents hole in the regs MMIO */

/* CTRL_Page */
struct ctrl_page {
	uint32_t blk_id;
	uint8_t reserved0[12];
	uint32_t feat_spt0;
	uint32_t feat_spt1;
	uint8_t reserved1[8];
	uint32_t dbch_cfg0;
	uint8_t reserved2[12];
	uint32_t ffch_cfg0;
	uint8_t reserved3[12];
	uint32_t fch_cfg0;
	uint8_t reserved4[188];
	uint32_t x_ctrl;
	/*-- MBX-only registers --*/
	uint8_t reserved5[60];
	uint32_t fch_ctrl;
	uint32_t fcg_int_en;
	uint8_t reserved6[696];
	/*-- End of MBX-only ---- */
	uint32_t dbch_int_st[MHUV3_DBCH_CMB_INT_ST_REG_CNT];
	uint32_t ffch_int_st[MHUV3_FFCH_CMB_INT_ST_REG_CNT];
	/*-- MBX-only registers --*/
	uint8_t reserved7[88];
	uint32_t fcg_int_st;
	uint8_t reserved8[12];
	uint32_t fcg_grp_int_st[32];
	uint8_t reserved9[2760];
	/*-- End of MBX-only ---- */
	uint32_t iidr;
	uint32_t aidr;
	uint32_t imp_def_id[12];
} __packed;

/* DBCW_Page */
struct pdbcw_page {
	uint32_t st;
	uint8_t reserved0[8];
	uint32_t set;
	uint32_t int_st;
	uint32_t int_clr;
	uint32_t int_en;
	uint32_t ctrl;
} __packed;

struct mdbcw_page {
	uint32_t st;
	uint32_t st_msk;
	uint32_t clr;
	uint8_t reserved0[4];
	uint32_t msk_st;
	uint32_t msk_set;
	uint32_t msk_clr;
	uint32_t ctrl;
} __packed;

struct dummy_page {
	uint8_t reserved0[KB(4)];
} __packed;

struct mhuv3_pbx_frame_reg {
	struct ctrl_page ctrl;
	struct pdbcw_page dbcw[MHUV3_DBCW_MAX];
	struct dummy_page ffcw;
	struct dummy_page fcw;
	uint8_t reserved0[KB(4) * 11];
	struct dummy_page impdef;
} __packed;

struct mhuv3_mbx_frame_reg {
	struct ctrl_page ctrl;
	struct mdbcw_page dbcw[MHUV3_DBCW_MAX];
	struct dummy_page ffcw;
	struct dummy_page fcw;
	uint8_t reserved0[KB(4) * 11];
	struct dummy_page impdef;
} __packed;

/* ====== MHUv3 data structures ====== */

enum mbox_mhuv3_frame {
	PBX_FRAME,
	MBX_FRAME,
};

static char *mbox_mhuv3_str[] = {
	"PBX",
	"MBX"
};

enum mbox_mhuv3_extension_type {
	DBE_EXT,
	FCE_EXT,
	FE_EXT,
	NUM_EXT
};

static char *mbox_mhuv3_ext_str[] = {
	"DBE",
	"FCE",
	"FE"
};

/** @brief MHUv3 channel information. */
struct mbox_mhuv3_channel {
	/** Channel window index associated to this mailbox channel. */
	uint32_t ch_idx;
	/**
	 * Doorbell bit number within the @ch_idx window.
	 * Only relevant to Doorbell transport.
	 */
	uint32_t doorbell;
	/** Transport protocol specific operations for this channel. */
	const struct mbox_mhuv3_protocol_ops *ops;
	/** Callback function to execute on incoming message interrupts. */
	mbox_callback_t cb;
	/** Pointer to some private data provided at registration time */
	void *user_data;
};

/** @brief MHUv3 operations */
struct mbox_mhuv3_protocol_ops {
	/** Receiver enable callback. */
	int (*rx_enable)(const struct device *dev, struct mbox_mhuv3_channel *chan);
	/** Receiver disable callback. */
	int (*rx_disable)(const struct device *dev, struct mbox_mhuv3_channel *chan);
	/** Read available Sender in-band LE data (if any). */
	int *(*read_data)(const struct device *dev, struct mbox_mhuv3_channel *chan);
	/**
	 * Acknowledge data reception to the Sender. Any out-of-band data
	 * has to have been already retrieved before calling this.
	 */
	int (*rx_complete)(const struct device *dev, struct mbox_mhuv3_channel *chan);
	/** Sender enable callback. */
	int (*tx_enable)(const struct device *dev, struct mbox_mhuv3_channel *chan);
	/** Sender disable callback. */
	int (*tx_disable)(const struct device *dev, struct mbox_mhuv3_channel *chan);
	/** Report back to the Sender if the last transfer has completed. */
	int (*last_tx_done)(const struct device *dev, struct mbox_mhuv3_channel *chan);
	/** Send data to the receiver. */
	int (*send_data)(const struct device *dev, struct mbox_mhuv3_channel *chan,
			 const struct mbox_msg *msg);
};

/**
 * @brief MHUv3 extension descriptor
 */
struct mbox_mhuv3_extension {
	/** Type of extension. */
	enum mbox_mhuv3_extension_type type;
	/** Max number of channels found for this extension. */
	uint32_t num_chans;
	/**
	 * First channel number assigned to this extension, picked from
	 * the set of all mailbox channels descriptors created.
	 */
	uint32_t base_ch_idx;
	/** Extension specific helper to parse DT and lookup associated
	 *  channel from the related 'mboxes' property.
	 */
	struct mbox_mhuv3_channel *(*get_channel)(const struct device *dev,
						  uint32_t channel,
						  uint32_t param);
	/** Extension specific helper to setup the combined irq. */
	void (*combined_irq_setup)(const struct device *dev);
	/** Extension specific helper to initialize channels. */
	int (*channels_init)(const struct device *dev, uint32_t *base_ch_idx);
	/**
	 * Extension specific helper to lookup which channel
	 * triggered the combined irq.
	 */
	struct mbox_mhuv3_channel *(*chan_from_comb_irq_get)(const struct device *dev);
	/** Extension specifier helper to get maximum data size. */
	int (*mtu_get)(void);
	/** Array of per-channel pending doorbells. */
	uint32_t pending_db[MHUV3_DBCW_MAX];
	/** Protect access to pending_db */
	struct k_spinlock pending_lock;
};

/**
 * @brief MHUv3 mailbox configuration data
 */
struct mbox_mhuv3_config {
	/** A reference to the MHUv3 control page for this block. */
	struct ctrl_page *ctrl;
	union {
		/** Base address of the PBX register mapping region. */
		struct mhuv3_pbx_frame_reg *pbx;
		/** Base address of the MBX register mapping region. */
		struct mhuv3_mbx_frame_reg *mbx;
	};
	/** Interrupt configuration function pointer. */
	void (*cmb_irq_config)(const struct device *dev);
};

/**
 * @brief MHUv3 mailbox controller data
 */
struct mbox_mhuv3_data {
	/** Frame type: MBX_FRAME or PBX_FRAME. */
	enum mbox_mhuv3_frame frame;
	/** Flag to indicate if the MHU supports AutoOp full mode. */
	bool auto_op_full;
	/** MHUv3 controller architectural major version. */
	uint32_t major;
	/** MHUv3 controller architectural minor version. */
	uint32_t minor;
	/** MHUv3 controller IIDR implementer. */
	uint32_t implem;
	/** MHUv3 controller IIDR revision. */
	uint32_t rev;
	/** MHUv3 controller IIDR variant. */
	uint32_t var;
	/** MHUv3 controller IIDR product_id. */
	uint32_t prod_id;
	/** The total number of channels discovered across all extensions. */
	uint32_t num_chans;
	/** Array holding descriptors for any found implemented extension. */
	struct mbox_mhuv3_extension ext[NUM_EXT];
	/** Array of channels */
	/**
	 * TODO: The total number of channels should be increased when the rest of the
	 * extensions get supported.
	 */
	struct mbox_mhuv3_channel channels[CONFIG_MBOX_MHUV3_NUM_DBCH][MHUV3_FLAG_BITS];
};

typedef int (*mhuv3_extension_initializer)(const struct device *dev);

/* =========================== Utility Functions =========================== */

/**
 * @brief Reads a bitmask from a 32-bit memory-mapped register.
 *
 * Extracts and returns the value of bits specified by a bitmask
 * from the register at the given memory address.
 *
 * @param addr Address of the 32-bit register.
 * @param bitmask Bitmask specifying the bits to extract.
 *
 * @return Extracted bitmask value.
 */
static ALWAYS_INLINE uint32_t read_bitmask32(mem_addr_t addr, uint32_t bitmask)
{
	uint32_t reg = sys_read32(addr);

	return FIELD_GET(bitmask, reg);
}

/**
 * @brief Writes a bitmask to a 32-bit memory-mapped register.
 *
 * Updates bits specified by a bitmask in the register at the given
 * memory address with the provided data value.
 *
 * @param data Value to write to the specified bits.
 * @param addr Address of the 32-bit register.
 * @param bitmask Bitmask specifying the bits to update.
 */
static ALWAYS_INLINE void write_bitmask32(uint32_t data, mem_addr_t addr, uint32_t bitmask)
{
	uint32_t reg = sys_read32(addr);

	reg &= ~bitmask;
	reg |= FIELD_PREP(bitmask, data);
	sys_write32(reg, addr);
}

/* =================== Doorbell transport protocol operations =============== */

/**
 * @brief Enables transfer acknowledgment events for a channel.
 *
 * @param dev Pointer to the device structure.
 * @param chan Pointer to the MHUv3 channel structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int mbox_mhuv3_doorbell_tx_enable(const struct device *dev, struct mbox_mhuv3_channel *chan)
{
	const struct mbox_mhuv3_config *cfg = dev->config;

	if (chan->ch_idx >= MHUV3_DBCW_MAX) {
		return -EINVAL;
	}

	/* Enable Transfer Acknowledgment events */
	write_bitmask32(0x1,
			(mem_addr_t)&cfg->pbx->dbcw[chan->ch_idx].int_en,
			PDBCW_INT_TFR_ACK);

	return 0;
}

/**
 * @brief Disables transfer acknowledgment events for a channel.
 *
 * Disables Transfer Acknowledgment events and clears the corresponding
 * interrupt and pending doorbell for the specified channel.
 *
 * @param dev Pointer to the device structure.
 * @param chan Pointer to the MHUv3 channel structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int mbox_mhuv3_doorbell_tx_disable(const struct device *dev, struct mbox_mhuv3_channel *chan)
{
	const struct mbox_mhuv3_config *cfg = dev->config;
	struct mbox_mhuv3_data *data = dev->data;
	struct mbox_mhuv3_extension *ext = &data->ext[DBE_EXT];
	struct pdbcw_page *dbcw = cfg->pbx->dbcw;

	if (chan->ch_idx >= MHUV3_DBCW_MAX) {
		return -EINVAL;
	}

	/* Disable Channel Transfer Acknowledgment events */
	write_bitmask32(0x0, (mem_addr_t)&dbcw[chan->ch_idx].int_en, PDBCW_INT_TFR_ACK);

	/* Clear Channel Transfer Acknowledgment and pending doorbell */
	write_bitmask32(0x1, (mem_addr_t)&dbcw[chan->ch_idx].int_clr, PDBCW_INT_TFR_ACK);
	K_SPINLOCK(&ext->pending_lock) {
		ext->pending_db[chan->ch_idx] = 0;
	}

	return 0;
}

/**
 * @brief Enables transfer events for a channel.
 *
 * @param dev Pointer to the device structure.
 * @param chan Pointer to the MHUv3 channel structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int mbox_mhuv3_doorbell_rx_enable(const struct device *dev, struct mbox_mhuv3_channel *chan)
{
	const struct mbox_mhuv3_config *cfg = dev->config;

	if (chan->ch_idx >= MHUV3_DBCW_MAX) {
		return -EINVAL;
	}

	/* Unmask Channel Transfer events */
	sys_set_bit((mem_addr_t)&cfg->mbx->dbcw[chan->ch_idx].msk_clr, chan->doorbell);

	return 0;
}

/**
 * @brief Disables transfer events for a channel.
 *
 * @param dev Pointer to the device structure.
 * @param chan Pointer to the MHUv3 channel structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int mbox_mhuv3_doorbell_rx_disable(const struct device *dev, struct mbox_mhuv3_channel *chan)
{
	const struct mbox_mhuv3_config *cfg = dev->config;

	if (chan->ch_idx >= MHUV3_DBCW_MAX) {
		return -EINVAL;
	}

	/* Mask Channel Transfer events */
	sys_set_bit((mem_addr_t)&cfg->mbx->dbcw[chan->ch_idx].msk_set, chan->doorbell);

	return 0;
}

/**
 * @brief Completes a transfer event for a channel.
 *
 * @param dev Pointer to the device structure.
 * @param chan Pointer to the MHUv3 channel structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int mbox_mhuv3_doorbell_rx_complete(const struct device *dev,
					   struct mbox_mhuv3_channel *chan)
{
	const struct mbox_mhuv3_config *cfg = dev->config;

	if (chan->ch_idx >= MHUV3_DBCW_MAX) {
		return -EINVAL;
	}

	/* Clearing the pending transfer generates the Channel Transfer Acknowledge */
	sys_set_bit((mem_addr_t)&cfg->mbx->dbcw[chan->ch_idx].clr, chan->doorbell);

	return 0;
}

/**
 * @brief Checks if the last transfer for a channel is complete.
 *
 * Determines whether the last transfer for the specified channel in the
 * Doorbell Channel Window (DBCW) register is complete. If the transfer is
 * complete, it also clears the pending doorbell for the channel.
 *
 * @param dev Pointer to the device structure.
 * @param chan Pointer to the MHUv3 channel structure.
 *
 * @return 0 if the last transfer is complete, or a negative error code otherwise.
 */
static int mbox_mhuv3_doorbell_last_tx_done(const struct device *dev,
					    struct mbox_mhuv3_channel *chan)
{
	const struct mbox_mhuv3_config *cfg = dev->config;

	if (chan->ch_idx >= MHUV3_DBCW_MAX) {
		return -EINVAL;
	}

	bool done = !(sys_test_bit((mem_addr_t)&cfg->pbx->dbcw[chan->ch_idx].st, chan->doorbell));

	if (done) {
		struct mbox_mhuv3_data *data = dev->data;
		struct mbox_mhuv3_extension *ext = &data->ext[DBE_EXT];

		/* Take care to clear the pending doorbell also when polling */
		K_SPINLOCK(&ext->pending_lock) {
			sys_clear_bit((mem_addr_t)&ext->pending_db[chan->ch_idx], chan->doorbell);
		}

		return 0;
	}

	return -EBUSY;
}

/**
 * @brief Sends data via the specified channel.
 *
 * Attempts to send data using the specified channel. If the channel is
 * busy (i.e., a transfer is already in progress), it returns an error.
 * It also sets the appropriate flags to indicate pending data.
 *
 * @param dev Pointer to the device structure.
 * @param chan Pointer to the MHUv3 channel structure.
 * @param msg Pointer to the message structure containing data to be sent.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int mbox_mhuv3_doorbell_send_data(const struct device *dev,
					 struct mbox_mhuv3_channel *chan,
					 const struct mbox_msg *msg)
{
	const struct mbox_mhuv3_config *cfg = dev->config;
	struct mbox_mhuv3_data *data = dev->data;
	struct mbox_mhuv3_extension *ext = &data->ext[DBE_EXT];
	int ret = 0;

	if (chan->ch_idx >= MHUV3_DBCW_MAX) {
		return -EINVAL;
	}

	if (msg) {
		LOG_WRN("Doorbell extension does not support sending data");
	}

	K_SPINLOCK(&ext->pending_lock) {
		if (sys_test_bit((mem_addr_t)&ext->pending_db[chan->ch_idx], chan->doorbell)) {
			ret = -EBUSY;
		} else {
			sys_set_bit((mem_addr_t)&ext->pending_db[chan->ch_idx], chan->doorbell);
		}
	}

	if (ret == 0) {
		sys_set_bit((mem_addr_t)&cfg->pbx->dbcw[chan->ch_idx].set, chan->doorbell);
	}

	return ret;
}

static const struct mbox_mhuv3_protocol_ops mhuv3_doorbell_ops = {
	.tx_enable = mbox_mhuv3_doorbell_tx_enable,
	.tx_disable = mbox_mhuv3_doorbell_tx_disable,
	.rx_enable = mbox_mhuv3_doorbell_rx_enable,
	.rx_disable = mbox_mhuv3_doorbell_rx_disable,
	.rx_complete = mbox_mhuv3_doorbell_rx_complete,
	.last_tx_done = mbox_mhuv3_doorbell_last_tx_done,
	.send_data = mbox_mhuv3_doorbell_send_data,
};

/**
 * @brief Retrieves the MHUv3 channel based on the provided channel ID.
 *
 * This function extracts the channel and doorbell numbers from the
 * given Flattened Channel ID, and retrieves the corresponding channel
 * from the Doorbell extension. The Flattened Channel ID is calculated
 * using the formula: Channel ID * MHUV3_FLAG_BITS + Doorbell.
 *
 * @param dev Pointer to the device structure.
 * @param channel_id The Flattened Channel ID that contains both the
 *		     channel and doorbell information.
 *
 * @return Pointer to the corresponding MHUv3 channel structure.
 */
static struct mbox_mhuv3_channel *mbox_mhuv3_get_channel(const struct device *dev,
							 mbox_channel_id_t channel_id)
{
	struct mbox_mhuv3_data *data = dev->data;
	uint32_t channel, doorbell;

	/**
	 * TODO: The type of extension, channel and doorbell should be
	 * extracted from a struct passed to this function from device tree,
	 * but mbox_channel_id_t is only a Flattened Channel ID,
	 * where Flattened Channel ID = Channel ID * MHUV3_FLAG_BITS + Doorbell
	 * Until this is changed, Pin the type to Doorbell extension and use
	 * the next formula to get the actual channel and doorbell numbers.
	 */
	enum mbox_mhuv3_extension_type type = DBE_EXT;

	channel = channel_id / MHUV3_FLAG_BITS;
	doorbell = channel_id % MHUV3_FLAG_BITS;

	return data->ext[type].get_channel(dev, channel, doorbell);
}

/**
 * @brief Sends data using the specified MHUv3 sender channel.
 *
 * This function checks if the last transmission is complete for the
 * specified channel. If not, it returns an error. If the channel is
 * available, it proceeds to send the data using the channel's send_data
 * operation.
 *
 * @param dev Pointer to the device structure.
 * @param channel_id The ID of the channel to use for sending data.
 * @param msg Pointer to the message structure containing the data to be sent.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int mbox_mhuv3_sender_send_data(const struct device *dev,
				       mbox_channel_id_t channel_id,
				       const struct mbox_msg *msg)
{
	struct mbox_mhuv3_channel *chan = mbox_mhuv3_get_channel(dev, channel_id);
	int ret;

	ret = chan->ops->last_tx_done(dev, chan);
	if (ret) {
		return ret;
	}

	return chan->ops->send_data(dev, chan, msg);
}

/**
 * @brief Attempts to send data using the MHUv3 receiver channel (not supported).
 *
 * This function logs an error as transmitting data on an MHUv3 receiver channel
 * is not supported and returns an I/O error.
 *
 * @param dev Pointer to the device structure.
 * @param channel_id The ID of the channel (unused).
 * @param msg Pointer to the message structure (unused).
 *
 * @return -ENOTSUP to indicate an I/O error as transmission is not supported.
 */
static int mbox_mhuv3_receiver_send_data(const struct device *dev,
					 mbox_channel_id_t channel_id,
					 const struct mbox_msg *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(msg);

	LOG_ERR("Trying to transmit on a MBX MHUv3 frame");

	return -ENOTSUP;
}

/**
 * @brief Sends data based on the frame type.
 *
 * This function determines whether to send data using the sender or receiver
 * functions based on the current frame type.
 *
 * @param dev Pointer to the device structure.
 * @param channel_id The ID of the channel to use for sending data.
 * @param msg Pointer to the message structure containing the data to be sent.
 *
 * @return 0 on success, an error code on failure.
 */
static int mbox_mhuv3_send_data(const struct device *dev,
				mbox_channel_id_t channel_id,
				const struct mbox_msg *msg)
{
	struct mbox_mhuv3_data *data = dev->data;

	if (data->frame == PBX_FRAME) {
		return mbox_mhuv3_sender_send_data(dev, channel_id, msg);
	}

	return mbox_mhuv3_receiver_send_data(dev, channel_id, msg);
}

/**
 * @brief Retrieves the MHUv3 channel based on the specified channel and doorbell.
 *
 * This function returns the channel structure for the given channel and doorbell
 * numbers.
 *
 * @param dev Pointer to the device structure.
 * @param channel The channel number to retrieve.
 * @param doorbell The doorbell number to retrieve.
 *
 * @return Pointer to the corresponding MHUv3 channel structure, or NULL if invalid.
 */
static struct mbox_mhuv3_channel *mbox_mhuv3_dbe_get_channel(const struct device *dev,
							     uint32_t channel,
							     uint32_t doorbell)
{
	struct mbox_mhuv3_data *data = dev->data;
	struct mbox_mhuv3_extension *ext = &data->ext[DBE_EXT];

	if (channel >= ext->num_chans || doorbell >= MHUV3_FLAG_BITS) {
		LOG_ERR("Couldn't find a valid channel (%d: %d)", channel, doorbell);
		return NULL;
	}

	return &data->channels[ext->base_ch_idx + channel][doorbell];
}

/**
 * @brief Configures the combined IRQ for the MHUv3 device.
 *
 * This function sets up the combined interrupt handling for the specified device,
 * configuring the interrupt enable, clear, and control registers for each channel.
 * It handles both PBX and non-PBX frame types differently.
 *
 * @param dev Pointer to the device structure.
 */
static void mbox_mhuv3_dbe_combined_irq_setup(const struct device *dev)
{
	const struct mbox_mhuv3_config *cfg = dev->config;
	struct mbox_mhuv3_data *data = dev->data;
	struct mbox_mhuv3_extension *ext = &data->ext[DBE_EXT];
	uint32_t i;

	if (data->frame == PBX_FRAME) {
		struct pdbcw_page *dbcw = cfg->pbx->dbcw;

		for (i = 0; i < ext->num_chans; i++) {
			write_bitmask32(0x1, (mem_addr_t)&dbcw[i].int_clr, PDBCW_INT_TFR_ACK);
			write_bitmask32(0x0, (mem_addr_t)&dbcw[i].int_en, PDBCW_INT_TFR_ACK);
			write_bitmask32(0x1, (mem_addr_t)&dbcw[i].ctrl, XBCW_CTRL_COMB_EN);
		}
	} else {
		struct mdbcw_page *dbcw = cfg->mbx->dbcw;

		for (i = 0; i < ext->num_chans; i++) {
			sys_write32(0xFFFFFFFFU, (mem_addr_t)&dbcw[i].clr);
			sys_write32(0xFFFFFFFFU, (mem_addr_t)&dbcw[i].msk_set);
			write_bitmask32(0x1, (mem_addr_t)&dbcw[i].ctrl, XBCW_CTRL_COMB_EN);
		}
	}
}

/**
 * @brief Initializes the MHUv3 DBE channels.
 *
 * This function initializes the MHUv3 DBE channels by assigning the base index,
 * configuring each channel with its corresponding index and doorbell, and
 * setting the appropriate channel operations. It updates the base channel index
 * after initialization.
 *
 * @param dev Pointer to the device structure.
 * @param base_ch_idx Pointer to the base channel index.
 *
 * @return 0 on success.
 */
static int mbox_mhuv3_dbe_channels_init(const struct device *dev, uint32_t *base_ch_idx)
{
	struct mbox_mhuv3_data *data = dev->data;
	struct mbox_mhuv3_extension *ext = &data->ext[DBE_EXT];
	struct mbox_mhuv3_channel *chan;

	__ASSERT(((*base_ch_idx + ext->num_chans) * MHUV3_FLAG_BITS) < data->num_chans,
		 "The number of allocated channels is less than required by the MHUv3 extension");

	ext->base_ch_idx = *base_ch_idx;
	chan = &data->channels[*base_ch_idx];

	for (uint32_t i = 0; i < ext->num_chans; i++) {
		for (uint32_t k = 0; k < MHUV3_FLAG_BITS; k++) {
			chan->ch_idx = i;
			chan->doorbell = k;
			chan->ops = &mhuv3_doorbell_ops;
			chan++;
		}
	}

	*base_ch_idx += ext->num_chans;

	return 0;
}

/**
 * @brief Searches for the active doorbell for the given MHUv3 DBE channel.
 *
 * This function checks the interrupt status and identifies which doorbell has been
 * triggered for the specified channel. It returns the doorbell index if found or
 * logs a warning if an unexpected interrupt occurs.
 *
 * @param dev Pointer to the device structure.
 * @param channel The channel to check for the doorbell.
 * @param doorbell Pointer to store the found doorbell index.
 *
 * @return true if a doorbell is found, false if an unexpected IRQ is encountered.
 */
static bool mbox_mhuv3_dbe_doorbell_search(const struct device *dev,
					   uint32_t channel,
					   uint32_t *doorbell)
{
	const struct mbox_mhuv3_config *cfg = dev->config;
	struct mbox_mhuv3_data *data = dev->data;
	struct mbox_mhuv3_extension *ext = &data->ext[DBE_EXT];
	uint32_t st;

	__ASSERT(channel < MHUV3_DBCW_MAX,
		 "Number of channels exceeds the maximum number of doorbell channel windows.");

	if (data->frame == PBX_FRAME) {
		uint32_t active_doorbells, fired_doorbells;

		st = read_bitmask32((mem_addr_t)&cfg->pbx->dbcw[channel].int_st, PDBCW_INT_TFR_ACK);
		if (!st) {
			goto unexpected_irq;
		}
		active_doorbells = sys_read32((mem_addr_t)&cfg->pbx->dbcw[channel].st);
		K_SPINLOCK(&ext->pending_lock) {
			fired_doorbells = ext->pending_db[channel] & ~active_doorbells;
			if (!fired_doorbells) {
				K_SPINLOCK_BREAK;
			}

			*doorbell = find_lsb_set(fired_doorbells) - 1;
			ext->pending_db[channel] &= ~BIT(*doorbell);
		}
		if (!fired_doorbells) {
			goto unexpected_irq;
		}
		/* If no other pending doorbells, clear the transfer acknowledge */
		if (!(fired_doorbells & ~BIT(*doorbell))) {
			write_bitmask32(0x1, (mem_addr_t)&cfg->pbx->dbcw[channel].int_clr,
					PDBCW_INT_TFR_ACK);
		}
	} else {
		st = sys_read32((mem_addr_t)&cfg->mbx->dbcw[channel].st_msk);
		if (!st) {
			goto unexpected_irq;
		}

		*doorbell = find_lsb_set(st) - 1;
	}

	return true;

unexpected_irq:
	LOG_WRN("Unexpected IRQ on %s channel:%d", mbox_mhuv3_str[data->frame], channel);

	return false;
}

/**
 * @brief Retrieves the channel corresponding to a combined interrupt.
 *
 * This function checks the combined interrupt status for the MHUv3 DBE device and
 * identifies the channel and doorbell associated with the interrupt. It returns the
 * matching channel if found, or NULL if no valid channel is identified.
 *
 * @param dev Pointer to the device structure.
 *
 * @return Pointer to the mbox_mhuv3_channel if a valid channel is found, NULL otherwise.
 */
static struct mbox_mhuv3_channel *mbox_mhuv3_dbe_chan_from_comb_irq_get(const struct device *dev)
{
	const struct mbox_mhuv3_config *cfg = dev->config;
	struct mbox_mhuv3_data *data = dev->data;
	struct mbox_mhuv3_extension *ext = &data->ext[DBE_EXT];

	for (uint32_t i = 0; i < MHUV3_DBCH_CMB_INT_ST_REG_CNT; i++) {
		uint32_t channel, doorbell, int_st;

		int_st = sys_read32((mem_addr_t)&cfg->ctrl->dbch_int_st[i]);
		if (!int_st) {
			continue;
		}

		channel = i * MHUV3_FLAG_BITS + (find_lsb_set(int_st) - 1);
		if (channel >= ext->num_chans) {
			LOG_ERR("Invalid %s channel: %d", mbox_mhuv3_str[data->frame], channel);
			return NULL;
		}

		if (!mbox_mhuv3_dbe_doorbell_search(dev, channel, &doorbell)) {
			continue;
		}

		LOG_DBG("Found %s channel [%d], doorbell[%d]",
			mbox_mhuv3_str[data->frame], channel, doorbell);
		return &data->channels[channel][doorbell];
	}

	return NULL;
}

/**
 * @brief Retrieves the maximum transmit units for doorbell extension.
 *
 * @return 0 as doorbell extension does not support transmitting data.
 */
static int mbox_mhuv3_dbe_mtu_get(void)
{
	return 0;
}

/**
 * @brief Initializes the Doorbell Extension (DBE) for the MHUv3 device.
 *
 * This function initializes the Doorbell Extension (DBE) by reading the number
 * of channels from the device's control registers, and setting up necessary handlers.
 * If the feature is not supported, it returns early. It also updates the device's
 * data with the number of channels available and the extension's function pointers.
 *
 * @param dev Pointer to the device structure.
 *
 * @return 0 if initialization is successful, negative error code if there is a failure.
 */
static int mbox_mhuv3_dbe_init(const struct device *dev)
{
	const struct mbox_mhuv3_config *cfg = dev->config;
	struct mbox_mhuv3_data *data = dev->data;
	struct mbox_mhuv3_extension *ext = &data->ext[DBE_EXT];
	uint32_t num_chans;

	if (!read_bitmask32((mem_addr_t)&cfg->ctrl->feat_spt0, FEAT_SPT0_DBE_SPT)) {
		return 0;
	}

	LOG_DBG("%s: Initializing Doorbell Extension.", mbox_mhuv3_str[data->frame]);

	ext->type = DBE_EXT;
	/* Note that, by the spec, the number of channels is (num_dbch + 1) */
	num_chans = read_bitmask32((mem_addr_t)&cfg->ctrl->dbch_cfg0, DBCH_CFG0_NUM_DBCH) + 1;
	__ASSERT(CONFIG_MBOX_MHUV3_NUM_DBCH >= num_chans,
		"The number of configured doorbell channels is less than required by the MHUv3 extension");
	ext->num_chans = num_chans;
	ext->get_channel = mbox_mhuv3_dbe_get_channel;
	ext->combined_irq_setup = mbox_mhuv3_dbe_combined_irq_setup;
	ext->channels_init = mbox_mhuv3_dbe_channels_init;
	ext->chan_from_comb_irq_get = mbox_mhuv3_dbe_chan_from_comb_irq_get;
	ext->mtu_get = mbox_mhuv3_dbe_mtu_get;

	data->num_chans += ext->num_chans * MHUV3_FLAG_BITS;

	LOG_DBG("%s: found %d DBE channels.", mbox_mhuv3_str[data->frame], ext->num_chans);

	return 0;
}

/**
 * @brief Initializes the Fast Channel Extension (FCE) for the MHUv3 device.
 *
 * This function initializes the Fast Channel Extension (FCE). Currently,
 * FCE is not supported.
 *
 * @param dev Pointer to the device structure.
 *
 * @return 0, indicating that the function completes successfully.
 */
static int mbox_mhuv3_fce_init(const struct device *dev)
{
	const struct mbox_mhuv3_config *cfg = dev->config;
	struct mbox_mhuv3_data *data = dev->data;

	if (!read_bitmask32((mem_addr_t)&cfg->ctrl->feat_spt0, FEAT_SPT0_FCE_SPT)) {
		return 0;
	}

	LOG_DBG("%s: Fast Channel Extension is not supported by driver.",
		mbox_mhuv3_str[data->frame]);

	return 0;
}

/**
 * @brief Initializes the FIFO Extension (FE) for the MHUv3 device.
 *
 * This function initializes the FIFO Extension (FE). Currently,
 * FE is not supported.
 *
 * @param dev Pointer to the device structure.
 *
 * @return 0, indicating that the function completes successfully.
 */
static int mbox_mhuv3_fe_init(const struct device *dev)
{
	const struct mbox_mhuv3_config *cfg = dev->config;
	struct mbox_mhuv3_data *data = dev->data;

	if (!read_bitmask32((mem_addr_t)&cfg->ctrl->feat_spt0, FEAT_SPT0_FE_SPT)) {
		return 0;
	}

	LOG_DBG("%s: FIFO Extension is not supported by driver.", mbox_mhuv3_str[data->frame]);

	return 0;
}

static mhuv3_extension_initializer mhuv3_extension_init[NUM_EXT] = {
	mbox_mhuv3_dbe_init,
	mbox_mhuv3_fce_init,
	mbox_mhuv3_fe_init,
};

/**
 * @brief Initializes the channels for the MHUv3 device.
 *
 * This function allocates memory for the channels array and initializes
 * each channel based on the device's extensions. It calls the `channels_init`
 * function for each extension and updates the base channel index accordingly.
 *
 * @param dev Pointer to the device structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int mbox_mhuv3_initialize_channels(const struct device *dev)
{
	struct mbox_mhuv3_data *data = dev->data;
	uint32_t base_ch_idx = 0;
	int ret = 0;

	for (uint32_t i = 0; i < NUM_EXT && !ret; i++) {
		struct mbox_mhuv3_extension *ext = &data->ext[i];

		if (ext->num_chans > 0) {
			ret = ext->channels_init(dev, &base_ch_idx);
		}
	}

	return ret;
}

/**
 * @brief Initializes the MHUv3 frame for the device.
 *
 * This function reads the block ID to determine the frame type (PBX or MBX),
 * retrieves the version and implementer information, and checks if the frame
 * is supported. It also handles the initialization of extensions for the MHUv3 device.
 *
 * @param dev Pointer to the device structure.
 *
 * @return 0 on success, or a negative error code on failure (e.g., -EIO, -ENOMEM).
 */
static int mbox_mhuv3_frame_init(const struct device *dev)
{
	const struct mbox_mhuv3_config *cfg = dev->config;
	struct mbox_mhuv3_data *data = dev->data;

	data->frame = read_bitmask32((mem_addr_t)&cfg->ctrl->blk_id, BLK_ID_BLK_ID);
	if (data->frame > MBX_FRAME) {
		LOG_ERR("Invalid Frame type- %d", data->frame);
		return -EIO;
	}

	data->major = read_bitmask32((mem_addr_t)&cfg->ctrl->aidr, AIDR_ARCH_MAJOR_REV);
	data->minor = read_bitmask32((mem_addr_t)&cfg->ctrl->aidr, AIDR_ARCH_MINOR_REV);
	data->implem = read_bitmask32((mem_addr_t)&cfg->ctrl->iidr, IIDR_IMPLEMENTER);
	data->rev = read_bitmask32((mem_addr_t)&cfg->ctrl->iidr, IIDR_REVISION);
	data->var = read_bitmask32((mem_addr_t)&cfg->ctrl->iidr, IIDR_VARIANT);
	data->prod_id = read_bitmask32((mem_addr_t)&cfg->ctrl->iidr, IIDR_PRODUCT_ID);
	if (data->major != MHUV3_MAJOR_VERSION) {
		LOG_ERR("Unsupported MHU %s block - major:%d  minor:%d",
			mbox_mhuv3_str[data->frame], data->major, data->minor);
		return -EIO;
	}

	data->auto_op_full = !!read_bitmask32((mem_addr_t)&cfg->ctrl->feat_spt1,
					      FEAT_SPT1_AUTO_OP_SPT);
	/* Request the PBX/MBX to remain operational */
	if (data->auto_op_full) {
		write_bitmask32(0x1, (mem_addr_t)&cfg->ctrl->x_ctrl, CTRL_OP_REQ);
	}

	LOG_DBG("Found MHU %s block - major:%d  minor:%d\n"
		"  implem:0x%X  rev:0x%X  var:0x%X  prod_id:0x%X",
		mbox_mhuv3_str[data->frame], data->major, data->minor,
		data->implem, data->rev, data->var, data->prod_id);

	for (uint32_t i = 0; i < NUM_EXT; i++) {
		uint32_t ret;
		/*
		 * Note that extensions initialization fails only when such
		 * extension initialization routine fails and the extensions
		 * was found to be supported in hardware and in software.
		 */
		ret = mhuv3_extension_init[i](dev);
		if (ret) {
			LOG_ERR("Failed to initialize %s %s: %d",
				mbox_mhuv3_str[data->frame],
				mbox_mhuv3_ext_str[i],
				ret);
			return -EIO;
		}
	}

	return 0;
}

/**
 * @brief Handles the PBX combined interrupt for the MHUv3 device.
 *
 * This function iterates through all available extensions and checks for channels
 * that are part of the combined interrupt mechanism.
 *
 * @param dev Pointer to the device structure.
 */
static void mbox_mhuv3_pbx_comb_interrupt(const struct device *dev)
{
	uint32_t found = 0;

	for (uint32_t i = 0; i < NUM_EXT; i++) {
		struct mbox_mhuv3_data *data = dev->data;
		struct mbox_mhuv3_extension *ext = &data->ext[i];
		struct mbox_mhuv3_channel *chan;

		/* FCE does not participate to the PBX combined */
		if (i == FCE_EXT || ext->num_chans == 0) {
			continue;
		}

		chan = ext->chan_from_comb_irq_get(dev);
		if (!chan) {
			continue;
		}

		found++;

		if (!chan->ops) {
			LOG_WRN("TX Ack on UNBOUND channel (%u)", chan->ch_idx);
			continue;
		}
	}

	if (found == 0) {
		LOG_WRN("Failed to find channel for the TX interrupt");
	}
}

/**
 * @brief Handles the MBX combined interrupt for the MHUv3 device.
 *
 * This function iterates through all available extensions and checks for channels
 * that are part of the combined interrupt mechanism. It reads data from the channel
 * if available and invokes the appropriate callback for the channel.
 *
 * @param dev Pointer to the device structure.
 */
static void mbox_mhuv3_mbx_comb_interrupt(const struct device *dev)
{
	uint32_t found = 0;

	for (uint32_t i = 0; i < NUM_EXT; i++) {
		struct mbox_mhuv3_data *data = dev->data;
		struct mbox_mhuv3_extension *ext = &data->ext[i];
		struct mbox_mhuv3_channel *chan;
		uint32_t flattened_ch_idx;

		if (ext->num_chans == 0) {
			continue;
		}

		chan = ext->chan_from_comb_irq_get(dev);
		if (!chan) {
			continue;
		}

		found++;

		if (!chan->ops) {
			LOG_WRN("RX Data on UNBOUND channel (%u)", chan->ch_idx);
		}

		if (chan->ops->read_data) {
			void *data = chan->ops->read_data(dev, chan);

			if (!data) {
				LOG_ERR("Failed to read in-band data.");
				goto rx_ack;
			}
		}

		/* Call channel call back */
		flattened_ch_idx = chan->ch_idx * MHUV3_FLAG_BITS + chan->doorbell;
		chan->cb(dev, flattened_ch_idx, chan->user_data, NULL);

rx_ack:
		if (chan->ops->rx_complete) {
			chan->ops->rx_complete(dev, chan);
		}
	}

	if (found == 0) {
		LOG_ERR("Failed to find channel for the RX interrupt");
	}
}

/**
 * @brief Handles the combined interrupt for the MHUv3 device, based on the frame type.
 *
 * This function checks the current frame type of the MHUv3 device (PBX or MBX) and
 * delegates the interrupt handling to the appropriate function.
 *
 * @param dev Pointer to the device structure.
 */
static void mbox_mhuv3_comb_interrupt(const struct device *dev)
{
	struct mbox_mhuv3_data *data = dev->data;

	if (data->frame == PBX_FRAME) {
		mbox_mhuv3_pbx_comb_interrupt(dev);
	} else {
		mbox_mhuv3_mbx_comb_interrupt(dev);
	}
}

/**
 * @brief Sets up the PBX for the MHUv3 device.
 *
 * This function initializes the PBX-related IRQs (interrupts) for the MHUv3 device.
 * If no combined IRQ configuration is provided, it defaults to using PBX in Tx polling mode.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success.
 */
static int mbox_mhuv3_setup_pbx(const struct device *dev)
{
	const struct mbox_mhuv3_config *cfg = dev->config;

	if (cfg->cmb_irq_config != NULL) {
		cfg->cmb_irq_config(dev);

		for (uint32_t i = 0; i < NUM_EXT; i++) {
			struct mbox_mhuv3_data *data = dev->data;

			if (data->ext[i].num_chans > 0) {
				data->ext[i].combined_irq_setup(dev);
			}
		}

		LOG_DBG("MHUv3 PBX IRQs initialized.");

		return 0;
	}

	LOG_INF("Using PBX in Tx polling mode.");

	return 0;
}

/**
 * @brief Sets up the MBX for the MHUv3 device.
 *
 * This function initializes the MBX-related IRQs (interrupts) for the MHUv3 device.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, -EINVAL if combined IRQ configuration is missing.
 */
static int mbox_mhuv3_setup_mbx(const struct device *dev)
{
	const struct mbox_mhuv3_config *cfg = dev->config;

	if (cfg->cmb_irq_config == NULL) {
		LOG_ERR("MBX combined IRQ is missing!");
		return -EINVAL;
	}

	cfg->cmb_irq_config(dev);

	for (uint32_t i = 0; i < NUM_EXT; i++) {
		struct mbox_mhuv3_data *data = dev->data;

		if (data->ext[i].num_chans > 0) {
			data->ext[i].combined_irq_setup(dev);
		}
	}

	LOG_DBG("MHUv3 MBX IRQs initialized.");

	return 0;
}

/**
 * @brief Initializes the IRQs for the MHUv3 device.
 *
 * This function initializes the IRQs (interrupts) based on the frame type
 * of the MHUv3 device.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, or the error code returned by the setup functions.
 */
static int mbox_mhuv3_irqs_init(const struct device *dev)
{
	struct mbox_mhuv3_data *data = dev->data;

	LOG_DBG("Initializing %s block.", mbox_mhuv3_str[data->frame]);

	if (data->frame == PBX_FRAME) {
		return mbox_mhuv3_setup_pbx(dev);
	}

	return mbox_mhuv3_setup_mbx(dev);
}

/**
 * @brief Initializes the MHUv3 device.
 *
 * This function initializes the MHUv3 device by performing the following steps:
 * 1. Initializes the frame type.
 * 2. Initializes the IRQs.
 * 3. Initializes the channels.
 *
 * If any of the initialization steps fail, the error code is returned immediately.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, or the error code from any of the initialization functions.
 */
static int mbox_mhuv3_init(const struct device *dev)
{
	int ret;

	ret = mbox_mhuv3_frame_init(dev);
	if (ret) {
		return ret;
	}

	ret = mbox_mhuv3_irqs_init(dev);
	if (ret) {
		return ret;
	}

	ret = mbox_mhuv3_initialize_channels(dev);
	if (ret) {
		return ret;
	}

	return ret;
}

/**
 * @brief Attempts to register a callback for a specified channel on the sender.
 *
 * @param dev Pointer to the device structure.
 * @param channel_id The ID of the channel for the callback.
 * @param cb The callback function to be registered.
 * @param user_data User data to be passed to the callback.
 *
 * @return Always returns -ENOTSUP as callback registration for a sender device is not supported.
 */
static int mbox_mhuv3_sender_register_callback(const struct device *dev,
					       mbox_channel_id_t channel_id,
					       mbox_callback_t cb, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	LOG_ERR("Trying to register a callback on a PBX MHUv3 frame");

	return -ENOTSUP;
}

/**
 * @brief Registers a callback function for a specified channel on the receiver.
 *
 * This function associates a callback with the specified channel. The callback
 * will be invoked when the receiver processes data for that channel. The user
 * data will be passed to the callback when it is called.
 *
 * @param dev Pointer to the device structure.
 * @param channel_id The channel ID for which the callback is being registered.
 * @param cb The callback function to register for the receiver.
 * @param user_data User-specific data that will be passed to the callback.
 *
 * @return 0 on success, or -EINVAL if the specified channel is invalid.
 */
static int mbox_mhuv3_receiver_register_callback(const struct device *dev,
						 mbox_channel_id_t channel_id,
						 mbox_callback_t cb, void *user_data)
{
	struct mbox_mhuv3_channel *chan = mbox_mhuv3_get_channel(dev, channel_id);

	if (!chan) {
		return -EINVAL;
	}

	chan->cb = cb;
	chan->user_data = user_data;

	return 0;
}

/**
 * @brief Registers a callback function for a specified channel, based on the frame type.
 *
 * @param dev Pointer to the device structure.
 * @param channel_id The channel ID for which the callback is being registered.
 * @param cb The callback function to register.
 * @param user_data User-specific data that will be passed to the callback.
 *
 * @return 0 on success, or an error code from the sender or receiver callback registration.
 */
static int mbox_mhuv3_register_callback(const struct device *dev,
					mbox_channel_id_t channel_id,
					mbox_callback_t cb, void *user_data)
{
	struct mbox_mhuv3_data *data = dev->data;

	if (data->frame == PBX_FRAME) {
		return mbox_mhuv3_sender_register_callback(dev, channel_id, cb, user_data);
	}

	return mbox_mhuv3_receiver_register_callback(dev, channel_id, cb, user_data);
}

/**
 * @brief Retrieves the maximum transmission unit (MTU) for the sender device.
 *
 * @param dev Pointer to the device structure.
 *
 * @return The MTU size for the sender channel.
 */
static int mbox_mhuv3_sender_mtu_get(const struct device *dev)
{
	struct mbox_mhuv3_data *data = dev->data;

	/**
	 * TODO: The maximum transmit units depends on the channel extension.
	 * The type of extension, channel and doorbell should be
	 * extracted from a struct passed to this function from device tree.
	 * Until this is changed, Pin the type to Doorbell extension which
	 * is the only supported extension.
	 */

	enum mbox_mhuv3_extension_type type = DBE_EXT;

	return data->ext[type].mtu_get();
}

/**
 * @brief Attempts to retrieve the maximum transmission unit (MTU) for the receiver device.
 *
 * @param dev Pointer to the device structure.
 *
 * @return Always returns 0.
 */
static int mbox_mhuv3_receiver_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_ERR("Trying to get maximum transmit units on a MBX MHUv3 frame");

	return 0;
}

/**
 * @brief Retrieves the Maximum Transmission Unit (MTU) for the MHUv3 device.
 *
 * This function determines the MTU based on the frame type (PBX or MBX) and calls
 * the appropriate function to retrieve the MTU for the sender or receiver.
 *
 * @param dev Pointer to the device structure.
 *
 * @return The MTU value for the sender or receiver, depending on the frame type.
 */
static int mbox_mhuv3_mtu_get(const struct device *dev)
{
	struct mbox_mhuv3_data *data = dev->data;

	if (data->frame == PBX_FRAME) {
		return mbox_mhuv3_sender_mtu_get(dev);
	}

	return mbox_mhuv3_receiver_mtu_get(dev);
}

/**
 * @brief Retrieves the maximum number of channels available in the MHUv3 device.
 *
 * @param dev Pointer to the device structure.
 *
 * @return The number of available channels for the MHUv3 device.
 */
static uint32_t mbox_mhuv3_max_channels_get(const struct device *dev)
{
	struct mbox_mhuv3_data *data = dev->data;

	return data->num_chans;
}

/**
 * @brief Enables or disables a channel based on the MHUv3 frame type.
 *
 * This function handles enabling or disabling the transmission (TX) or reception (RX)
 * functionality of a channel.
 *
 * @param dev The device handle representing the MHUv3 system.
 * @param channel_id The ID of the channel to enable or disable.
 * @param enabled Boolean flag to enable (true) or disable (false) the channel.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int mbox_mhuv3_set_enabled(const struct device *dev,
				  mbox_channel_id_t channel_id, bool enabled)
{
	struct mbox_mhuv3_data *data = dev->data;
	struct mbox_mhuv3_channel *chan = mbox_mhuv3_get_channel(dev, channel_id);
	int ret;

	if (!chan) {
		return -EINVAL;
	}

	if (data->frame == PBX_FRAME) {
		if (enabled) {
			ret = chan->ops->tx_enable(dev, chan);
		} else {
			ret = chan->ops->tx_disable(dev, chan);
		}
	} else {
		if (enabled) {
			ret = chan->ops->rx_enable(dev, chan);
		} else {
			ret = chan->ops->rx_disable(dev, chan);
		}
	}

	return ret;
}

static DEVICE_API(mbox, mhuv3_driver_api) = {
	.send = mbox_mhuv3_send_data,
	.register_callback = mbox_mhuv3_register_callback,
	.mtu_get = mbox_mhuv3_mtu_get,
	.max_channels_get = mbox_mhuv3_max_channels_get,
	.set_enabled = mbox_mhuv3_set_enabled,
};

#define MHUV3_INIT(n)							\
									\
static void mbox_mhuv3_cmb_irq_config_##n(const struct device *dev)	\
{									\
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, combined, irq),		\
		    DT_INST_IRQ_BY_NAME(n, combined, priority),		\
		    mbox_mhuv3_comb_interrupt,				\
		    DEVICE_DT_INST_GET(n),				\
		    0);							\
	irq_enable(DT_INST_IRQ_BY_NAME(n, combined, irq));		\
}									\
									\
static const struct mbox_mhuv3_config mhuv3_cfg_##n = {			\
	(struct ctrl_page *)DT_INST_REG_ADDR(n),			\
	DT_INST_REG_ADDR(n),						\
	mbox_mhuv3_cmb_irq_config_##n,					\
};									\
									\
struct mbox_mhuv3_data mhuv3_data_##n;					\
									\
DEVICE_DT_INST_DEFINE(n,						\
		      &mbox_mhuv3_init,					\
		      NULL,						\
		      &mhuv3_data_##n,					\
		      &mhuv3_cfg_##n,					\
		      POST_KERNEL,					\
		      CONFIG_MBOX_INIT_PRIORITY,			\
		      &mhuv3_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MHUV3_INIT);
