/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INTEL_COMM_WIDGET_H
#define INTEL_COMM_WIDGET_H

#include <zephyr/arch/xtensa/cache.h>
#include <zephyr/kernel.h>

#define CW_DT_NODE			DT_NODELABEL(ace_comm_widget)
#define CW_BASE				DT_REG_ADDR(CW_DT_NODE)

/*
 * @brief DSP Communication Widget for Intel ADSP
 *
 * These registers control the DSP Communication Widget for generic sideband message transmit /
 * receive.
 */

/*
 * Downstream Attributes
 * Attribute register for downstream message.
 */
#define DSATTR				0x00

/*
 * Destination Port ID
 * type: RO/V, rst: 00h
 *
 * Destination Port ID received in message.
 */
#define DSATTR_DSTPID			GENMASK(7, 0)

/*
 * Source Port ID
 * type: RO/V, rst: 00h
 *
 * Source Port ID received in message.
 */
#define DSATTR_SRCPID			GENMASK(15, 8)

/*
 * Opcode
 * type: RO/V, rst: 00h
 *
 * Opcode received in message.
 */
#define DSATTR_OPC			GENMASK(23, 16)

/*
 * Byte Enable
 * type: RO/V, rst: 0h
 *
 * Byte enables received in message.
 */
#define DSATTR_BE			GENMASK(27, 24)

/*
 * Reserved
 * type: RO, rst: 0h
 */
#define DSATTR_RSVD31			GENMASK(31, 28)


/*
 * Downstream Lower Address
 * Address register (lower 32 bits) for downstream message.
 *
 * type: RO/V, rst: 0000 0000h
 *
 * LSB 32 address bits received in message. Bits 32:16 of the LSB address only
 * valid when DSUADDR.ADDRLEN bit is set to 1.
 */
#define DSLADDR				0x04


/*
 * Downstream Upper Address
 * Address register (upper 32 bits) for downstream message.
 */
#define DSUADDR				0x08

/*
 * Upper Address
 * type: RO/V, rst: 0000h
 *
 * MSB 16 address bits received in message. Valid only when ADDRLEN bit is set to 1.
 */
#define DSUADDR_UADDR			GENMASK(15, 0)

/*
 * Reserved
 * type: RO, rst: 0000h
 */
#define DSUADDR_RSVD30			GENMASK(30, 16)

/*
 * Address Length
 * type: RO/V, rst: 0b
 *
 * Address length indication received in message.
 * 0: 16-bit address
 * 1: 48-bit address
 */
#define DSUADDR_ADDRLEN			BIT(31)


/*
 * Downstream SAI
 * Extended header (SAI / RS) register for downstream message.
 */
#define DSSAI				0x0C

/*
 * SAI
 * type: RO/V, rst: 0000h
 *
 * SAI received in message. Valid if EHP bit set to 1.
 */
#define DSSAI_SAI			GENMASK(15, 0)

/*
 * Root Space
 * type: RO/V, rst: 0h
 *
 * Root space received in message. Valid if EHP bit set to 1.
 */
#define DSSAI_RS			GENMASK(19, 16)

/*
 * Reserved
 * type: RO, rst: 000h
 */
#define DSSAI_RSVD30			GENMASK(30, 20)

/*
 * Extended Header Present
 * type: RO/V, rst: 0b
 *
 * Extended header present indication received in message. When set to 1 indicates
 * extended header is present, and RS / SAI fields are valid.
 */
#define DSSAI_EHP			BIT(31)


/*
 * Downstream Data
 * Receive data register for downstream message.
 *
 * type: RO/V, rst: 0000 0000h
 *
 * Data received in message.
 */
#define DSDATA				0x10


/*
 * Downstream Control & Status
 * Control & status register for downstream message.
 */
#define DSCTLSTS			0x14

/*
 * Transaction Type
 * type: RO/V, rst: 00b
 *
 * Indicates type of transaction received as follows:
 * 01: Posted message
 * 10: Non-posted message
 * 11: Completion message.
 */
#define DSCTLSTS_TRANTYP		GENMASK(1, 0)

/*
 * Reserved
 * type: RO, rst: 000 0000h
 */
#define DSCTLSTS_RSVD29			GENMASK(29, 2)

/*
 * Interrupt GENMASK
 * type: RW, rst: 0b, rst domain: gHSTRST
 *
 * Interrupt GENMASK register for message received interrupt. When set to 1 interrupt
 * is not generated to DSP Core. GENMASK does not affect interrupt status bit.
 */
#define DSCTLSTS_IM			BIT(30)

/*
 * Message Received
 * type: RO/V, rst: 0b
 *
 * Message received interrupt status register. Set by HW when message is received,
 * and cleared by HW when FW writes to upstream completion control register
 * indicating completion is available.
 */
#define DSCTLSTS_MSGRCV			BIT(31)


/*
 * Downstream Source Port ID
 * Source port ID register for ACE IP.
 */
#define ACESRCPID			0x18

/*
 * Source Port ID
 * type: RO/V, rst: ACE_SRCID
 *
 * Source Port ID of ACE IP. Default value is hardcoded to parameter ACE_SRCID.
 */
#define ACESRCPID_SRCPID		GENMASK(7, 0)

/*
 * Reserved
 * type: RO, rst: 00 0000h
 */
#define ACESRCPID_RSVD31		GENMASK(31, 8)


/*
 * Downstream Completion Data
 *
 * type: RO/V, rst: 0000 0000h
 *
 * Completion data received for upstream non-posted read.
 */
#define DSCPDATA			0x20


/*
 * Downstream Completion SAI
 *
 * type: RO/V, rst: 0000 0000h
 *
 * Completion SAI received for upstream non-posted message.
 */
#define DSCPSAI				0x24


/*
 * Downstream Completion Control & Status
 */
#define DSCPCTLSTS			0x28

/*
 * Completion Status
 * type: RO/V, rst: 0b
 *
 * Completion status received for upstream non-posted message
 * 000: Successful Completion (SC)
 * 001: Unsupported Request (UR).
 */
#define DSCPCTLSTS_CPSTS		GENMASK(2, 0)

/*
 * Reserved
 * type: RO, rst: 0b
 */
#define DSCPCTLSTS_RSVD7		GENMASK(7, 3)

/*
 * Completion Extended Header Present
 * type: RO/V, rst: 0b
 *
 * Completion EH present indication received for upstream non-posted message.
 */
#define DSCPCTLSTS_CPEHP		BIT(8)

/*
 * Reserved
 * type: RO, rst: 00 0000h
 */
#define DSCPCTLSTS_RSVD29		GENMASK(29, 9)

/*
 * Interrupt GENMASK
 * type: RW, rst: 0b, rst domain: gHSTRST
 *
 * Interrupt GENMASK register for completion received interrupt. When set to 1
 * interrupt is not generated to DSP Core. GENMASK does not affect interrupt status
 * bit.
 */
#define DSCPCTLSTS_IM			BIT(30)

/*
 * Completion Received
 * type: RW/1C, rst: 0b, rst domain: gHSTRST
 *
 * Completion received status register. Set by HW when completion is received, and
 * cleared by FW when writing 1 to it.
 */
#define DSCPCTLSTS_CPRCV		BIT(31)


/*
 * Upstream Status
 * Status register for upstream message.
 */
#define USSTS				0x40

/*
 * State Machine Status
 * type: RO/V, rst: 0b
 *
 * 0: The Endpoint Status Machine is idle
 * 1: The Endpoint State Machine is busy
 * This is OR of posted SM and non-posted SM busy signals.
 */
#define USSTS_SMSTS			BIT(0)

/*
 * Message Sent
 * type: RW/1C, rst: 0b, rst domain: gHSTRST
 *
 * Upstream message sent interrupt status. Set by HW when upstream message has been
 * sent out, and cleared by FW when writing 1 to it.
 */
#define USSTS_MSGSENT			BIT(1)

/*
 * Reserved
 * type: RO, rst: 0000 0000h
 */
#define USSTS_RSVD31			GENMASK(31, 2)


/*
 * Upstream Command
 * Command register for upstream message.
 */
#define USCMD				0x44

/*
 * Send Sideband Transaction
 * type: WO, rst: 0b
 *
 * DSP FW writes a 1 to this bit to cause an IOSF SB Transaction. The type of
 * transaction is determined by Bit 1 in this register. This bit will always be
 * read as 0 but FW can write it to 1 to start the upstream transaction. In that way
 * it won't be readable as 1 after writing to. The write to this bit is ignored by
 * hardware if opcode is 0x20 - 0x2F.
 */
#define USCMD_SSBTRAN			BIT(0)

/*
 * Transaction Type
 * type: RW, rst: 0b, rst domain: gHSTRST
 *
 * 0: Transaction will be a Write
 * 1: Transaction will be a Read
 * For posted message this register bit value will be ignored and hardcoded to 0 in
 * the actual message as reads cannot be posted.
 */
#define USCMD_TRANTYP			BIT(1)

/*
 * Message Type
 * type: RW, rst: 0b, rst domain: gHSTRST
 *
 * 0: Transaction will be non-posted
 * 1: Transaction will be posted
 */
#define USCMD_MSGTYP			BIT(2)

/*
 * Interrupt Enable
 * type: RW, rst: 0b, rst domain: gHSTRST
 *
 * Interrupt enable register for message sent interrupt. When cleared to 0 interrupt
 * is not generated to DSP Core. Enable does not affect interrupt status bit.
 */
#define USCMD_IE			BIT(3)

/*
 * Reserved
 * type: RO, rst: 000 0000h
 */
#define USCMD_RSVD31			GENMASK(31, 4)


/*
 * Upstream Lower Address
 * Address register (lower 32 bits) for upstream message.
 *
 * type: RW, rst: 0000 0000h, rst domain: gHSTRST
 *
 * LSB 32 address bits for message to be sent.  Bits 32:16 of the LSB address only
 * valid when USUADDR.ADDRLEN bit is set to 1.
 */
#define USLADDR				0x48


/*
 * Upstream Upper Address
 * Address register (upper 32 bits) for upstream message.
 */
#define USUADDR				0x4C

/*
 * Upper Address
 * type: RW, rst: 0000h, rst domain: gHSTRST
 *
 * MSB 16 address bits for message to be sent. Valid only when ADDRLEN bit is set
 * to 1.
 */
#define USUADDR_UADDR			GENMASK(15, 0)

/*
 * Reserved
 * type: RO, rst: 0000h
 */
#define USUADDR_RSVD30			GENMASK(30, 16)

/*
 * Address Length
 * type: RW, rst: 0b, rst domain: gHSTRST
 *
 * Address length indication for message to be sent.
 * 0: 16-bit address
 * 1: 48-bit address
 * For simple message / message with data, this field will carry the MISC[3] of the
 * IOSF 1.2 message format.
 */
#define USUADDR_ADDRLEN			BIT(31)


/*
 * Upstream Data
 * Transmit data register for upstream message.
 *
 * type: RW, rst: 0000 0000h, rst domain: gHSTRST
 *
 * Data for message to be sent.
 */
#define USDATA				0x50


/*
 * Upstream Attributes
 * Attribute register for upstream message.
 */
#define USATTR				0x54

/*
 * Destination Port ID
 * type: RW, rst: 00h, rst domain: gHSTRST
 *
 * Destination ID for message to be sent.
 */
#define USATTR_DSTPID			GENMASK(7, 0)

/*
 * Opcode
 * type: RW, rst: 00h, rst domain: gHSTRST
 *
 * Opcode for message to be sent.
 */
#define USATTR_OPC			GENMASK(15, 8)

/*
 * Byte Enable
 * type: RW, rst: 0h, rst domain: gHSTRST
 *
 * Byte enables for message to be sent.
 */
#define USATTR_BE			GENMASK(19, 16)

/*
 * Base Address Register
 * type: RW, rst: 0h, rst domain: gHSTRST
 *
 * BAR for register access to be sent. For simple message / message with data, this
 * field will carry the MISC[2:0] of the IOSF 1.2 message format. Note: MSB of this
 * register field is not used given the IOSF Sideband message BAR field is ONLY 3
 * bits wide; and MISC[3] of the IOSF 1.2 message format is supplied by
 * USUADDR.ADDRLEN.
 */
#define USATTR_BAR			GENMASK(23, 20)

/*
 * Function ID
 * type: RW, rst: 00h, rst domain: gHSTRST
 *
 * Function ID for register access to be sent.
 */
#define USATTR_FID			GENMASK(31, 24)


/*
 * Upstream SAI
 * Extended header (SAI / RS) register for upstream message.
 */
#define USSAI				0x58

/*
 * SAI
 * type: RO, rst: DSPISAI_2SB
 *
 * SAI for message to be sent, if EHP bit set to 1. Reset value is hardcoded to
 * parameter DSPISAI_2SB.
 */
#define USSAI_SAI			GENMASK(7, 0)

/*
 * Reserved
 * type: RO, rst: 00h
 */
#define USSAI_RSVD15			GENMASK(15, 8)

/*
 * Root Space
 * type: RW, rst: 0h, rst domain: gHSTRST
 *
 * Root space for message to be sent, if EHP bit set to 1.
 */
#define USSAI_RS			GENMASK(19, 16)

/*
 * Reserved
 * type: RO, rst: 000h
 */
#define USSAI_RSVD30			GENMASK(30, 20)

/*
 * Extended Header Present
 * type: RO, rst: 1b
 *
 * Extended header present indication for message to be sent. When set to 1
 * indicates extended header is present. Currently tied to 1 in RTL as ACE IP
 * supports only EH=1 transactions.
 */
#define USSAI_EHP			BIT(31)


/*
 * Upstream Completion Data
 *
 * type: RW, rst: 0000 0000h, rst domain: gHSTRST
 *
 * Completion data to be sent for downstream non-posted read.
 */
#define USCPDATA			0x5C


/*
 * Upstream Completion Control & Status
 */
#define USCPCTLSTS			0x60

/*
 * Completion Status
 * type: RW, rst: 0h, rst domain: gHSTRST
 *
 * Completion status to be sent for downstream non-posted message.
 * 000: Successful Completion (SC)
 * 001: Unsupported Request (UR).
 */
#define USCPCTLSTS_CPSTS		GENMASK(2, 0)

/*
 * Reserved
 * type: RO, rst: 000 0000h
 */
#define USCPCTLSTS_RSVD30		GENMASK(30, 3)

/*
 * Sideband Completion
 * type: RW/1S, rst: 0h, rst domain: gHSTRST
 *
 * Completion for downstream request handling. DSP FW writes a 1 to this bit to
 * indicate downstream message received is consumed. This internally causes an IOSF
 * SB completion transaction if original downstream request is non-posted. If
 * original message is posted then upstream completion is not generated by HW. This
 * bit is cleared by HW.
 */
#define USCPCTLSTS_SBCP			BIT(31)


/*
 * Upstream completion SAI
 */
#define USCPSAI				0x64

/*
 * SAI
 * type: RO, rst: DSPISAI_2SB
 *
 * Completion SAI to be sent for downstream non-posted message. Reset value is
 * hardcoded to parameter DSPISAI_2SB.
 */
#define USCPSAI_CPSAI			GENMASK(7, 0)

/*
 * Reserved
 * type: RO, rst: 00h
 */
#define USCPSAI_RSVD15			GENMASK(15, 8)

/*
 * Spare 1
 * type: RW, rst: 0h, rst domain: gHSTRST
 *
 * 4 Spare bits.
 */
#define USCPSAI_SPARE1			GENMASK(19, 16)

/*
 * Reserved
 * type: RO, rst: 000h
 */
#define USCPSAI_RSVD30			GENMASK(30, 20)

/*
 * Spare 0
 * type: RW, rst: 0b, rst domain: gHSTRST
 *
 * 1 Spare bit.
 */
#define USCPSAI_SPARE0			BIT(31)


/*
 * SAI Width
 */
#define SAIWDTH				0x68

/*
 * SAI Width
 * type: RO, rst: SAI_WIDTH
 *
 * Specifies the SAI width value.
 * 0-based value.
 */
#define SAIWDTH_SAIWDTH			GENMASK(3, 0)

/*
 * Reserved
 * type: RO, rst: 000 0000h
 */
#define SAIWDTH_RSVD31			GENMASK(31, 4)


/*
 * Side Clock Gate
 * Sideband clock gating enable register.
 */
#define SCLKG				0x6C

/*
 * Local Clock Gate
 * type: RW, rst: 0b, rst domain: gHSTRST
 *
 * 0: Clk is un-gated
 * 1: Clk is gated
 * Implementation Note: Local clock gating is not implemented as there are only ~10
 * flops in the design.
 */
#define SCLKG_LCG			BIT(0)

/*
 * Trunk Clock Gate
 * type: RW, rst: 0b, rst domain: gHSTRST
 *
 * 0: Clk request enabled
 * 1: Clk is gated
 * Implementation Note: This FW managed TCG bit is not used as HW has been improved
 * to support trunk clock gating based on FSM operation.
 */
#define SCLKG_TCG			BIT(1)

/*
 * Reserved
 * type: RO, rst: 0000 0000h
 */
#define SCLKG_RSVD31			GENMASK(31, 2)


/*
 * Downstream Data 2
 * Receive data (second DW) register for downstream message.
 *
 * type: RW, rst: 0000 0000h, rst domain: gHSTRST
 *
 * Data received in message. Second DW, if valid.
 */
#define DSDATA2				0x74


/*
 * Downstream Access Enable
 *
 * Note: boot prep handling is an artifact of re-using the component from ISH. There is no usage
 * model in ACE IP to support any boot prep message handling by FW.
 */
#define DSACCEN				0x80

/*
 * Access Enable
 * type: RW, rst: 0b, rst domain: gHSTRST
 *
 * This bit is set by DSP FW to enable SBEP HW to accept downstream cycles from
 * Sideband peer agents (as well as access control policy owner for survivability).
 * NOTE: If a BOOTPREP message is received, DSP FW is interrupted unconditionally,
 * i.e. irrespective of this bit being 1 or 0.
 */
#define DSACCEN_ACCEN			BIT(0)

/*
 * Reserved
 * type: RO, rst: 0000 0000h
 */
#define DSACCEN_RSVD31			GENMASK(31, 1)


/*
 * Boot Prep Control
 * Boot prep message control register.
 * Note: boot prep handling is an artifact of re-using the component from ISH.  There is no usage
 * model in ACE IP to support any boot prep message handling by FW.
 */
#define BPCTL				0x84

/*
 * type: RW/1C, rst: 0b, rst domain: gHSTRST
 * Boot Prep Received Status
 *
 * This bit is set by SBEP HW upon the reception of BOOTPREP message. DSP FW is
 * required to clear this status bit by writing a 0 to the bit, upon issuing
 * BOOTPREPACK message on the upstream path of SBEP HW. SBEP HW will clear this bit,
 * upon auto ack'ing of BOOTPREP due to timeout condition.
 */
#define BPCTL_BPRCVSTS			BIT(0)

/*
 * Reserved
 * type: RO, rst: 0000 0000h
 */
#define BPCTL_RSVD31			GENMASK(31, 1)


#define CW_TRANSACTION_NONPOSTED	0
#define CW_TRANSACTION_POSTED		1

#define CW_TRANSACTION_WRITE		0
#define CW_TRANSACTION_READ		1

/*
 * @brief Check the endpoint state machine is idle
 *
 * @retval false The Endpoint State Machine is busy
 * @retval true The Endpoint State Machine is idle
 */
static inline bool cw_upstream_ready(void)
{
	uint32_t status = sys_read32(CW_BASE + USSTS);

	status &= ~USSTS_SMSTS;
	sys_write32(status, CW_BASE + USSTS);
	return !(sys_read32(CW_BASE + USSTS) & USSTS_SMSTS);
}

/*
 * @brief Configure attributes of upstream message
 *
 * @param dest Destination Port ID for message to be sent.
 * @param func Function ID for register access to be sent.
 * @param opcode Opcode for message to be sent.
 * @param be Byte enables for message to be sent.
 * @param bar Base Address Register for register access to be sent.
 */
static inline void cw_upstream_set_attr(uint32_t dest, uint32_t func, uint32_t opcode,
					uint32_t be, uint32_t bar)
{
	uint32_t attr = FIELD_PREP(USATTR_DSTPID, dest) | FIELD_PREP(USATTR_FID, func) |
			FIELD_PREP(USATTR_OPC, opcode) | FIELD_PREP(USATTR_BE, be) |
			FIELD_PREP(USATTR_BAR, bar);
	sys_write32(attr, CW_BASE + USATTR);
}

/*
 * @brief Set 16bit address for upstream message.
 *
 * @param address Address for message to be sent.
 */
static inline void cw_upstream_set_address16(uint16_t address)
{
	sys_write32(address, CW_BASE + USLADDR);
	sys_write32(0, CW_BASE + USUADDR);
}

/*
 * @brief Set transmit data for upstream message.
 *
 * @param data Data for message to be sent.
 */
static inline void cw_upstream_set_data(uint32_t data)
{
	sys_write32(data, CW_BASE + USDATA);
}

/*
 * @brief Interrupt enable / disable for message sent interrupt.
 *
 * @param enable Interrupt state
 */
static inline void cw_upstream_enable_sent_intr(bool enable)
{
	uint32_t cmd = sys_read32(CW_BASE + USCMD);

	if (enable)
		cmd |= USCMD_IE;
	else
		cmd &= ~USCMD_IE;

	sys_write32(cmd, CW_BASE + USCMD);
}

/*
 * @brief Write posted message.
 */
static inline void cw_upstream_do_pw(void)
{
	uint32_t cmd = sys_read32(CW_BASE + USCMD);

	cmd &= ~(USCMD_MSGTYP | USCMD_TRANTYP);
	cmd |= FIELD_PREP(USCMD_MSGTYP, CW_TRANSACTION_POSTED) |
	       FIELD_PREP(USCMD_TRANTYP, CW_TRANSACTION_WRITE) |
	       USCMD_SSBTRAN;

	sys_write32(cmd, CW_BASE + USCMD);
}

/*
 * @brief Clear message send interrupt status
 */
static inline void cw_upstream_clear_msgsent(void)
{
	uint32_t sts = sys_read32(CW_BASE + USSTS);

	sts |= USSTS_MSGSENT;
	sys_write32(sts, CW_BASE + USSTS);
}

/*
 * @brief Wait for message to be send.
 */
static inline void cw_upstream_wait_for_sent(void)
{
	WAIT_FOR(sys_read32(CW_BASE + USSTS) & USSTS_MSGSENT, 100, k_busy_wait(1));

	cw_upstream_clear_msgsent();
}

/*
 * @brief Write a sideband message.
 */
void cw_sb_write(uint32_t dest, uint32_t func, uint16_t address, uint32_t data);


#endif /* INTEL_COMM_WIDGET_H */
