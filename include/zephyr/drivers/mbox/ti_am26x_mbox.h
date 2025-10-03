/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
  * @brief This file is the mailbox configuration for AM261x SOC series
  * This file contains a constant array of mailbox configurations  
  */

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#define AM26X_MBOX_MAX_MSGS_IN_QUEUE     4

/* size of MSG in Bytes */
#define AM26X_MBOX_MSG_SIZE              4

/* Total Number of possible Channels CONFIG_MAX_CPUS_NUM is configured via KConfig*/
#define AM26X_MBOX_MAX_CH_NUM	(CONFIG_MAX_CPUS_NUM * CONFIG_MAX_CPUS_NUM)

/* Total Number of Valid Channels */
#define AM26X_MBOX_MAX_CH_NUM_VALID	(CONFIG_MAX_CPUS_NUM * (CONFIG_MAX_CPUS_NUM - 1))
#define AM26X_MBOX_IS_CH_VALID(ch_number)	(((ch_number) < AM26X_MBOX_MAX_CH_NUM) && ((ch_number % AM26X_MBOX_MAX_CH_NUM) != 0))
#define AM26X_MBOX_GET_CH_NUM(sender, receiver)	((sender) * CONFIG_MAX_CPUS_NUM + (receiver))

#define AM26X_MBOX_GET_SW_Q_IDX(ch_number)	(AM26X_MBOX_MAX_CH_NUM_VALID + 1 - ((ch_number) - (ch_number / (CONFIG_MAX_CPUS_NUM + 1))))

#define AM26X_MBOX_GET_SENDER(ch_number)	((ch_number) / CONFIG_MAX_CPUS_NUM)
#define AM26X_MBOX_GET_RECEIVER(ch_number)	((ch_number) % CONFIG_MAX_CPUS_NUM)

typedef struct am26x_mbox_cfg_s
{
    uint32_t write_done_offset;   /**< Mailbox register address at which core will post interrupt */
    uint32_t read_req_offset;   /**< Mailbox register address at which core will receive interrupt */
    uint8_t intr_bit_pos;    /**< Bit pos in the mailbox register which should be set or cleared to post or clear a interrupt to other core */
    uint32_t swQ_addr;      /**< Infomration about the SW queue associated with this HW mailbox */
} am26x_mbox_cfg;