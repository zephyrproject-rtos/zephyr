/* cc2520.c - IEEE 802.15.4 driver for TI CC2520 */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Copyright (c) 2011, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>

#include <nanokernel.h>

#include <errno.h>
#include <gpio.h>
#include <spi.h>

#include <board.h>
#include <init.h>
#include <sections.h>

#include <802.15.4/cc2520.h>

#include <net/l2_buf.h>
#include "packetbuf.h"
#include "net_driver_15_4.h"

#include "cc2520.h"
#include "cc2520_arch.h"

#ifndef CC2520_CONF_AUTOACK
#define CC2520_CONF_AUTOACK 0
#endif /* CC2520_CONF_AUTOACK */

#define WITH_SEND_CCA 1

#define FOOTER_LEN 2

#define AUTOCRC (1 << 6)
#define AUTOACK (1 << 5)
#define FRAME_MAX_VERSION ((1 << 3) | (1 << 2))
#define FRAME_FILTER_ENABLE (1 << 0)
#define CORR_THR(n) (((n) & 0x1f) << 6)
#define FIFOP_THR(n) ((n) & 0x7f)

#define FOOTER1_CRC_OK      0x80
#define FOOTER1_CORRELATION 0x7f

#define WAIT_100ms 100
#define WAIT_1000ms 1000
#define WAIT_500ms 500
#define WAIT_200ms 200
#define WAIT_10ms 10
#define WAIT_1ms 1

struct cc2520_gpio_config cc2520_gpio_config[CC2520_GPIO_IDX_LAST_ENTRY];
struct cc2520_config cc2520_config;

/* CC2520 is currently a singleton instance
 * This would be needed no get fixed. The main
 * issue is the gpio cb handler: it would need to
 * get access to the relevant instance of cc2520 driver
 */
struct device *cc2520_sgl_dev;

/* static int cc2520_authority_level_of_sender; */
static int cc2520_packets_seen;
static int cc2520_packets_read;

static uint8_t lock_on;
static uint8_t lock_off;
static uint8_t locked;
static bool init_ok;

/* max time is in millisecs */
#define BUSYWAIT_UNTIL(cond, max_time)					\
	do {								\
		uint32_t t0 = clock_get_cycle();			\
		uint32_t limit = t0 + CLOCK_MSEC_TO_CYCLES(max_time);	\
		while (!(cond) && CLOCK_CYCLE_LT(clock_get_cycle(),	\
							limit));	\
	} while (0)

#define CC2520_STROBE_PLUS_NOP(strobe) cc2520_strobe_plus_nop(strobe)
#define CC2520_ENABLE_FIFOP_INT() cc2520_enable_fifop_int(1)
#define CC2520_DISABLE_FIFOP_INT() cc2520_enable_fifop_int(0)
#define CC2520_FIFOP_INT_INIT() cc2520_init_fifop_int(cc2520_gpio_int_handler)
#define CC2520_CLEAR_FIFOP_INT() cc2520_clear_fifop_int()
#define SET_VREG_ACTIVE() cc2520_set_vreg(1)
#define SET_VREG_INACTIVE() cc2520_set_vreg(0)
#define SET_RESET_ACTIVE() cc2520_set_reset(0)
#define SET_RESET_INACTIVE() cc2520_set_reset(1)

#define CC2520_FIFOP_IS_1 (cc2520_get_fifop() != 0)
#define CC2520_FIFO_IS_1 (cc2520_get_fifo() != 0)
#define CC2520_SFD_IS_1 (cc2520_get_sfd() != 0)
#define CC2520_CCA_IS_1 (cc2520_get_cca() != 0)

static volatile uint8_t cc2520_sfd_counter;
static volatile uint16_t cc2520_sfd_start_time;
static volatile uint16_t cc2520_sfd_end_time;

static volatile uint16_t last_packet_timestamp;

static signed char cc2520_last_rssi;
static uint8_t cc2520_last_correlation;

static uint8_t receive_on;
static int channel;
static bool cc2520_read_fifo_byte(uint8_t *byte)
{
	return cc2520_read_fifo_buf(byte, 1);
}

static uint8_t getreg(uint16_t regname)
{
	uint16_t reg = 0;

	if (!cc2520_read_reg(regname, &reg)) {
		DBG("%s: cannot read reg %d value\n", __func__,
		       regname);
	}
	return reg;
}

static void setreg(uint16_t regname, uint8_t value)
{
	if (!cc2520_write_reg(regname, value)) {
		DBG("%s: cannot set reg %d to %d\n", __func__,
		       regname, value);
	}
}

#ifdef CONFIG_TI_CC2520_DEBUG
static void print_radio_status(void)
{
	uint8_t value = getreg(CC2520_FSMSTAT1);

	DBG("Radio status FSMSTAT1: ");
	if (value & BIT(CC2520_STATUS_FIFO)) {
		DBG("FIFO ");
	}
	if (value & BIT(CC2520_STATUS_FIFOP)) {
		DBG("FIFOP ");
	}
	if (value & BIT(CC2520_STATUS_SFD)) {
		DBG("SFD ");
	}
	if (value & BIT(CC2520_STATUS_CCA)) {
		DBG("CCA ");
	}
	if (value & BIT(CC2520_STATUS_SAMPLED_CCA)) {
		DBG("SAMPLED_CCA ");
	}
	if (value & BIT(CC2520_STATUS_LOCK_STATUS)) {
		DBG("LOCK_STATUS ");
	}
	if (value & BIT(CC2520_STATUS_TX_ACTIVE)) {
		DBG("TX_ACTIVE ");
	}
	if (value & BIT(CC2520_STATUS_RX_ACTIVE)) {
		DBG("RX_ACTIVE ");
	}
	DBG("\n");
}

static inline void print_exceptions_0(void)
{
	uint8_t flag = getreg(CC2520_EXCFLAG0);

	DBG("EXCFLAG0: ");
	if (flag & BIT(CC2520_EXCFLAGS0_RF_IDLE)) {
		DBG("RF_IDLE ");
	}
	if (flag & BIT(CC2520_EXCFLAGS0_TX_FRM_DONE)) {
		DBG("TX_FRM_DONE ");
	}
	if (flag & BIT(CC2520_EXCFLAGS0_TX_ACK_DONE)) {
		DBG("TX_ACK_DONE ");
	}
	if (flag & BIT(CC2520_EXCFLAGS0_TX_UNDERFLOW)) {
		DBG("TX_UNDERFLOW ");
	}
	if (flag & BIT(CC2520_EXCFLAGS0_TX_OVERFLOW)) {
		DBG("TX_OVERFLOW ");
	}
	if (flag & BIT(CC2520_EXCFLAGS0_RX_UNDERFLOW)) {
		DBG("RX_UNDERFLOW ");
	}
	if (flag & BIT(CC2520_EXCFLAGS0_RX_OVERFLOW)) {
		DBG("RX_OVERFLOW ");
	}
	if (flag & BIT(CC2520_EXCFLAGS0_RXENABLE_ZERO)) {
		DBG("RXENABLE_ZERO");
	}
	DBG("\n");
}

static inline void print_exceptions_1(void)
{
	uint8_t flag = getreg(CC2520_EXCFLAG1);

	DBG("EXCFLAG1: ");
	if (flag & BIT(CC2520_EXCFLAGS1_RX_FRM_DONE)) {
		DBG("RX_FRM_DONE ");
	}
	if (flag & BIT(CC2520_EXCFLAGS1_RX_FRM_ACCEPTED)) {
		DBG("RX_FRM_ACCEPTED ");
	}
	if (flag & BIT(CC2520_EXCFLAGS1_SRC_MATCH_DONE)) {
		DBG("SRC_MATCH_DONE ");
	}
	if (flag & BIT(CC2520_EXCFLAGS1_SRC_MATCH_FOUND)) {
		DBG("SRC_MATCH_FOUND ");
	}
	if (flag & BIT(CC2520_EXCFLAGS1_FIFOP)) {
		DBG("FIFOP ");
	}
	if (flag & BIT(CC2520_EXCFLAGS1_SFD)) {
		DBG("SFD ");
	}
	if (flag & BIT(CC2520_EXCFLAGS1_DPU_DONE_L)) {
		DBG("DPU_DONE_L ");
	}
	if (flag & BIT(CC2520_EXCFLAGS1_DPU_DONE_H)) {
		DBG("DPU_DONE_H");
	}
	DBG("\n");
}

static inline void print_errors(void)
{
	uint8_t flag = getreg(CC2520_EXCFLAG2);

	DBG("EXCFLAG2: ");
	if (flag & BIT(CC2520_EXCFLAGS2_MEMADDR_ERROR)) {
		DBG("MEMADDR_ERROR ");
	}
	if (flag & BIT(CC2520_EXCFLAGS2_USAGE_ERROR)) {
		DBG("USAGE_ERROR ");
	}
	if (flag & BIT(CC2520_EXCFLAGS2_OPERAND_ERROR)) {
		DBG("OPERAND_ERROR ");
	}
	if (flag & BIT(CC2520_EXCFLAGS2_SPI_ERROR)) {
		DBG("SPI_ERROR ");
	}
	if (flag & BIT(CC2520_EXCFLAGS2_RF_NO_LOCK)) {
		DBG("RF_NO_LOCK ");
	}
	if (flag & BIT(CC2520_EXCFLAGS2_RX_FRM_ABORTED)) {
		DBG("RX_FRM_ABORTED ");
	}
	if (flag & BIT(CC2520_EXCFLAGS2_RFBUFMOV_TIMEOUT)) {
		DBG("RFBUFMOV_TIMEOUT");
	}
	DBG("\n");
}

static void clear_exceptions(void)
{
	DBG("Clearing up exceptions & errors\n");

	setreg(CC2520_EXCFLAG0, 0);
	setreg(CC2520_EXCFLAG1, 0);
	setreg(CC2520_EXCFLAG2, 0);
}

static void cc2520_print_gpio_config(void)
{
	DBG("GPIOCTRL0: 0x%x\n", getreg(CC2520_GPIOCTRL0));
	DBG("GPIOCTRL1: 0x%x\n", getreg(CC2520_GPIOCTRL1));
	DBG("GPIOCTRL2: 0x%x\n", getreg(CC2520_GPIOCTRL2));
	DBG("GPIOCTRL3: 0x%x\n", getreg(CC2520_GPIOCTRL3));
	DBG("GPIOCTRL4: 0x%x\n", getreg(CC2520_GPIOCTRL4));
	DBG("GPIOCTRL5: 0x%x\n", getreg(CC2520_GPIOCTRL5));
	DBG("GPIOPOLARITY: 0x%x\n", getreg(CC2520_GPIOPOLARITY));
	DBG("GPIOCTRL: 0x%x\n", getreg(CC2520_GPIOCTRL));
}

#else
#define print_radio_status()
#define print_exceptions_0()
#define print_exceptions_1()
#define print_errors()
#define clear_exceptions()
#define cc2520_print_gpio_config()
#endif

static inline unsigned int status(void)
{
	uint8_t status = 0x00;

	if (!cc2520_get_status(&status)) {
		DBG("Reading status 0x%x failed\n", status);
		return 0x00;
	}

	return status;
}

static inline int cc2520_pending_packet(void)
{
	return CC2520_FIFOP_IS_1;
}

static void flushrx(void)
{
	uint8_t dummy;

	cc2520_read_fifo_byte(&dummy);
	dummy++;

	cc2520_strobe(CC2520_INS_SFLUSHRX);
	cc2520_strobe(CC2520_INS_SFLUSHRX);

#if 0
	/* SFLUSHRX causes recalibration so wait small delay after
	 * it before continuing. Errata chapter 1.3
	 */
	clock_delay_usec_busywait(500);
#endif
}

static void on(void)
{
	DBG("cc2520 radio on\n");

	CC2520_ENABLE_FIFOP_INT();
	cc2520_strobe(CC2520_INS_SRXON);

	BUSYWAIT_UNTIL(status() & (BIT(CC2520_XOSC16M_STABLE)), WAIT_10ms);
	if (!(status() & BIT(CC2520_XOSC16M_STABLE))) {
		DBG("Clock is not stabilized, radio is not on\n");
		return;
	}

	print_radio_status();

	receive_on = 1;
}

static void off(void)
{
	DBG("cc2520 radio off\n");
	receive_on = 0;

	/* Wait for transmission to end before turning radio off. */
	BUSYWAIT_UNTIL(!(status() & BIT(CC2520_TX_ACTIVE)), WAIT_100ms);

	cc2520_strobe(CC2520_INS_SRFOFF);
	CC2520_DISABLE_FIFOP_INT();

	if (!cc2520_pending_packet()) {
		flushrx();
	}
}

static inline void cc2520_radio_lock(void)
{
	struct cc2520_config *info = cc2520_sgl_dev->config->config_info;

	nano_fiber_sem_take(&info->radio_lock, TICKS_UNLIMITED);
	locked++;
}

static inline void cc2520_radio_unlock(void)
{
	struct cc2520_config *info = cc2520_sgl_dev->config->config_info;

	if (lock_on) {
		on();
		lock_on = 0;
	}

	if (lock_off) {
		off();
		lock_off = 0;
	}

	locked--;
	nano_fiber_sem_give(&info->radio_lock);
}

static int cc2520_off(void)
{
	/* Don't do anything if we are already turned off. */
	if (!receive_on) {
		return 1;
	}

	if (locked) {
		lock_off = 1;
		return 1;
	}

	cc2520_radio_lock();

	if (status() & BIT(CC2520_TX_ACTIVE)) {
		lock_off = 1;
	} else {
		off();
	}

	cc2520_radio_unlock();

	return 1;
}

int cc2520_on(void)
{
	if (!init_ok) {
		DBG("cc2520 not initialized, radio will stay off\n");
		return 0;
	}

	if (receive_on) {
		return 1;
	}

	if (locked) {
		lock_on = 1;
		return 1;
	}

	cc2520_radio_lock();

	on();

	cc2520_radio_unlock();

	return 1;
}

static radio_result_t cc2520_set_rx_mode(radio_value_t value)
{
	static radio_value_t old_value = -1;

	if (value == old_value) {
		return RADIO_RESULT_OK;
	}

#if CC2520_CONF_AUTOACK
	value |= RADIO_RX_MODE_AUTOACK;
#endif /* CC2520_CONF_AUTOACK */

	/*
	 * Writing RAM requires crystal oscillator to be stable.
	 */
	BUSYWAIT_UNTIL(status() & (BIT(CC2520_XOSC16M_STABLE)), WAIT_100ms);
	if (!(status() & (BIT(CC2520_XOSC16M_STABLE)))) {
		DBG("cc2520_set_rx_mode: CC2520_XOSC16M_STABLE not set\n");
	}

	/* Wait for any transmission to end. */
	BUSYWAIT_UNTIL(!(status() & BIT(CC2520_TX_ACTIVE)), WAIT_100ms);

	if ((value & RADIO_RX_MODE_AUTOACK) !=
	    (old_value & RADIO_RX_MODE_AUTOACK)) {
		if (value & RADIO_RX_MODE_AUTOACK) {
			setreg(CC2520_FRMCTRL0, AUTOCRC | AUTOACK);
		} else {
			setreg(CC2520_FRMCTRL0, AUTOCRC);
		}
	}

	if ((value & RADIO_RX_MODE_ADDRESS_FILTER) !=
	    (old_value & RADIO_RX_MODE_ADDRESS_FILTER)) {
		if (value & RADIO_RX_MODE_ADDRESS_FILTER) {
			setreg(CC2520_FRMFILT0,
			       FRAME_MAX_VERSION | FRAME_FILTER_ENABLE);
		} else {
			setreg(CC2520_FRMFILT0, FRAME_MAX_VERSION);
		}
	}
	old_value = value;

#if 1
	if (receive_on) {
		cc2520_strobe(CC2520_INS_SRXON);

		BUSYWAIT_UNTIL((status() & BIT(CC2520_RSSI_VALID)), WAIT_100ms);
		if (!(status() & BIT(CC2520_RSSI_VALID))) {
			return RADIO_RESULT_ERROR;
		}
	}
#endif /* 1/0 */

	return RADIO_RESULT_OK;
}

radio_result_t cc2520_get_value(radio_param_t param, radio_value_t *value)
{
	if (!value) {
		return RADIO_RESULT_INVALID_VALUE;
	}

	switch (param) {
	case RADIO_PARAM_POWER_MODE:
		*value = receive_on ? RADIO_POWER_MODE_ON :
					RADIO_POWER_MODE_OFF;
		return RADIO_RESULT_OK;
	case RADIO_PARAM_CHANNEL:
		*value = cc2520_get_channel();
		return RADIO_RESULT_OK;
	case RADIO_CONST_CHANNEL_MIN:
		*value = 11;
		return RADIO_RESULT_OK;
	case RADIO_CONST_CHANNEL_MAX:
		*value = 26;
		return RADIO_RESULT_OK;
	default:
		return RADIO_RESULT_NOT_SUPPORTED;
	}
}

radio_result_t cc2520_set_value(radio_param_t param, radio_value_t value)
{
	switch (param) {
	case RADIO_PARAM_POWER_MODE:
		if (value == RADIO_POWER_MODE_ON) {
			cc2520_on();
			return RADIO_RESULT_OK;
		}
		if (value == RADIO_POWER_MODE_OFF) {
			cc2520_off();
			return RADIO_RESULT_OK;
		}
		return RADIO_RESULT_INVALID_VALUE;
	case RADIO_PARAM_CHANNEL:
		if (value < 11 || value > 26) {
			return RADIO_RESULT_INVALID_VALUE;
		}
		cc2520_set_channel(value);
		return RADIO_RESULT_OK;
	case RADIO_PARAM_PAN_ID:
		cc2520_set_pan_addr(value, 0x0000, NULL);
		return RADIO_RESULT_OK;
	case RADIO_PARAM_RX_MODE:
		return cc2520_set_rx_mode(value);
	default:
		return RADIO_RESULT_NOT_SUPPORTED;
	}
}

static void getrxdata(void *buf, int len)
{
	cc2520_read_fifo_buf(buf, len);
}

static void getrxbyte(uint8_t *byte)
{
	cc2520_read_fifo_byte(byte);
}

static inline bool strobe(uint8_t regname)
{
	return cc2520_strobe(regname);
}

static void set_txpower(uint8_t power)
{
	setreg(CC2520_TXPOWER, power);
}

static inline int cc2520_receiving_packet(void)
{
	return CC2520_SFD_IS_1;
}

static int cc2520_transmit(struct net_buf *buf, unsigned short payload_len)
{
	int txpower;
	uint32_t tx_start_wait;
	uint8_t sampled_cca;

	if (!init_ok) {
		return -EIO;
	}

	txpower = 0;

	if (packetbuf_attr(buf, PACKETBUF_ATTR_RADIO_TXPOWER) > 0) {
		/* Remember the current transmission power */
		txpower = cc2520_get_txpower();
		/* Set the specified transmission power */
		set_txpower(packetbuf_attr(buf, PACKETBUF_ATTR_RADIO_TXPOWER) - 1);
	}

	/* The TX FIFO can only hold one packet. Make sure to not overrun
	 * FIFO by waiting for transmission to start here and synchronizing
	 * with the CC2520_TX_ACTIVE check in cc2520_send.
	 *
	 * Note that we may have to wait up to 320 us (20 symbols) before
	 * transmission starts.
	 */

#if WITH_SEND_CCA
	strobe(CC2520_INS_SRXON);
	BUSYWAIT_UNTIL(status() & BIT(CC2520_RSSI_VALID), WAIT_100ms);
	strobe(CC2520_INS_STXONCCA);
	BUSYWAIT_UNTIL((sampled_cca = (getreg(CC2520_FSMSTAT1) &
					CC2520_FSMSTAT1_SAMPLED_CCA)),
			WAIT_10ms);
	if (sampled_cca == 0) {
		DBG("cc2520: sample_cca is 0, TX ERROR\n");
		return RADIO_TX_ERR;
	}
#else /* WITH_SEND_CCA */
	strobe(CC2520_INS_STXON);
#endif /* WITH_SEND_CCA */

	tx_start_wait = clock_get_cycle() + CLOCK_MSEC_TO_CYCLES(3000) + 1;
	while (CLOCK_CYCLE_LT(clock_get_cycle(), tx_start_wait)) {
		if (!CC2520_SFD_IS_1) {
			continue;
		}

#if PACKETBUF_WITH_PACKET_TYPE
		{
			uint32_t sfd_timestamp;

			sfd_timestamp = cc2520_sfd_start_time;
			if (packetbuf_attr(buf, PACKETBUF_ATTR_PACKET_TYPE) ==
					PACKETBUF_ATTR_PACKET_TYPE_TIMESTAMP) {
				/* Write timestamp to last two bytes of packet
				 * in TXFIFO.
				 */
				cc2520_write_ram(&sfd_timestamp,
					CC2520RAM_TXFIFO + payload_len - 1, 2);
			}
		}
#endif

		if (!(status() & BIT(CC2520_TX_ACTIVE))) {
			/* SFD went high but we are not transmitting.
			 * This means that we just started receiving a packet,
			 * so we drop the transmission.
			 */
			DBG("TX collision 0x%x\n", status());
			return RADIO_TX_COLLISION;
		}

		/* We wait until transmission has ended so that we get an
		 * accurate measurement of the transmission time.
		 */
		BUSYWAIT_UNTIL(!(status() & BIT(CC2520_TX_ACTIVE)), WAIT_500ms);

		DBG("status 0x%x\n", status());

		if (!receive_on) {
			/* We need to explicitly turn off the radio,
			 * since STXON[CCA] -> TX_ACTIVE -> RX_ACTIVE
			 */
			off();
		}

		if (packetbuf_attr(buf, PACKETBUF_ATTR_RADIO_TXPOWER) > 0) {
			/* Restore the transmission power */
			set_txpower(txpower & 0xff);
		}

		return RADIO_TX_OK;
	}

	/* If we are using WITH_SEND_CCA, we get here if the packet wasn't
	 * transmitted because of other channel activity.
	 */
	DBG("cc2520: transmission never started\n");

	print_exceptions_0();
	print_exceptions_1();

	if (packetbuf_attr(buf, PACKETBUF_ATTR_RADIO_TXPOWER) > 0) {
		/* Restore the transmission power */
		set_txpower(txpower & 0xff);
	}

	return RADIO_TX_COLLISION;
}

static int cc2520_prepare(const void *payload, unsigned short payload_len)
{
	uint8_t *buf = (uint8_t *)payload;
	uint8_t total_len;

	if (!init_ok) {
		return -EIO;
	}

	DBG("cc2520: sending %d bytes\n", payload_len);

	clear_exceptions();

	/* Write packet to TX FIFO. */
	strobe(CC2520_INS_SFLUSHTX);

	total_len = payload_len + FOOTER_LEN;
	DBG("TX FIFO has %u bytes\n", getreg(CC2520_TXFIFOCNT));
	cc2520_write_fifo_buf(&total_len, 1);
	cc2520_write_fifo_buf(buf, payload_len);
	DBG("TX FIFO has %u bytes\n", getreg(CC2520_TXFIFOCNT));

	print_errors();

	return 0;
}

static int cc2520_send(struct net_buf *buf, const void *payload,
		       unsigned short payload_len)
{
	int ret;

	cc2520_radio_lock();

	cc2520_prepare(payload, payload_len);
	ret = cc2520_transmit(buf, payload_len);

	cc2520_radio_unlock();

	return ret;
}

int cc2520_get_channel(void)
{
	return channel;
}

int cc2520_set_channel(int c)
{
	int ret = RADIO_RESULT_OK;
	uint16_t f;

	cc2520_radio_lock();

	/*
	 * Subtract the base channel (11), multiply by 5, which is the
	 * channel spacing. 357 is 2405-2048 and 0x4000 is LOCK_THR = 1.
	 */
	channel = c;

	f = MIN_CHANNEL + ((channel - MIN_CHANNEL) * CHANNEL_SPACING);
	/*
	 * Writing RAM requires crystal oscillator to be stable.
	 */
	BUSYWAIT_UNTIL((status() & (BIT(CC2520_XOSC16M_STABLE))), WAIT_100ms);

	/* Wait for any transmission to end. */
	BUSYWAIT_UNTIL(!(status() & BIT(CC2520_TX_ACTIVE)), WAIT_100ms);

	/* Define radio channel (between 11 and 25) */
	setreg(CC2520_FREQCTRL, f);

	/* If we are in receive mode, we issue an SRXON command to ensure
	 * that the VCO is calibrated.
	 */
	if (receive_on) {
		strobe(CC2520_INS_SRXON);
		BUSYWAIT_UNTIL((status() & BIT(CC2520_RSSI_VALID)), \
								WAIT_100ms);
		if (!(status() & BIT(CC2520_RSSI_VALID))) {
			ret = RADIO_RESULT_ERROR;
		}
	}

	cc2520_radio_unlock();

	return ret;
}

bool cc2520_set_pan_addr(unsigned pan, unsigned addr,
			 const uint8_t *ieee_addr)
{
	uint8_t tmp[2];

	cc2520_radio_lock();

	/*
	 * Writing RAM requires crystal oscillator to be stable.
	 */
	BUSYWAIT_UNTIL((status()) & (BIT(CC2520_XOSC16M_STABLE)), WAIT_1000ms);

	tmp[0] = pan & 0xff;
	tmp[1] = pan >> 8;
	cc2520_write_ram(tmp, CC2520RAM_PANID, 2);

	tmp[0] = addr & 0xff;
	tmp[1] = addr >> 8;
	cc2520_write_ram(tmp, CC2520RAM_SHORTADDR, 2);

	if (ieee_addr) {
		uint8_t tmp_addr[8];
		int f;

		/* LSB first, MSB last for 802.15.4 addresses in CC2520 */
		for (f = 0; f < 8; f++) {
			tmp_addr[7 - f] = ieee_addr[f];
		}

		cc2520_write_ram(tmp_addr, CC2520RAM_IEEEADDR, 8);
	}

	cc2520_radio_unlock();

	return true;
}

static int cc2520_read(void *buf, unsigned short bufsize)
{
	struct cc2520_config *info = cc2520_sgl_dev->config->config_info;
	uint8_t footer[2];
	uint8_t len;

	if (!init_ok) {
		return -EIO;
	}

	if (!cc2520_pending_packet()) {
		return -EAGAIN;
	}

	cc2520_packets_read++;

	getrxbyte(&len);

	DBG("%s: Incoming packet length: %d\n", __func__, len);

	/* Error cases:
	 * 1     -> out of sync!
	 * 2 & 3 -> bogus length
	 */
	if ((len > CC2520_MAX_PACKET_LEN) ||
	    (len - FOOTER_LEN > bufsize) ||
	    (len <= FOOTER_LEN)) {
		goto error;
	}

	getrxdata(buf, len - FOOTER_LEN);
	getrxdata(footer, FOOTER_LEN);

	if (footer[1] & FOOTER1_CRC_OK) {
		cc2520_last_rssi = footer[0];
		cc2520_last_correlation = footer[1] & FOOTER1_CORRELATION;
	} else {
		goto error;
	}

	if (cc2520_pending_packet()) {
		if (!CC2520_FIFO_IS_1) {
			/* Clean up in case of FIFO overflow!  This happens
			 * for every full length frame and is signaled by
			 * FIFOP = 1 and FIFO = 0
			 */
			flushrx();
		} else {
			/* Another packet might be waiting
			 * Let's unlock reading_packet_fiber()
			 */
			nano_fiber_sem_give(&info->read_lock);
		}
	}

	return len - FOOTER_LEN;

error:
	print_exceptions_0();
	print_exceptions_1();
	print_errors();

	flushrx();
	return -EINVAL;
}

static void read_packet(void)
{
	struct net_buf *buf;
	int len;

	buf = l2_buf_get_reserve(0);
	if (!buf) {
		DBG("%s: Could not allocate buffer\n", __func__);
		return;
	}

	packetbuf_set_attr(buf, PACKETBUF_ATTR_TIMESTAMP,
			   last_packet_timestamp);

	len = cc2520_read(packetbuf_dataptr(buf), PACKETBUF_SIZE);
	if (len < 0) {
		goto out;
	}

	packetbuf_set_attr(buf, PACKETBUF_ATTR_RSSI, cc2520_last_rssi);
	packetbuf_set_attr(buf, PACKETBUF_ATTR_LINK_QUALITY,
			   cc2520_last_correlation);
	packetbuf_set_datalen(buf, len);

	DBG("%s: received %d bytes\n", __func__, len);

	if (net_driver_15_4_recv_from_hw(buf) < 0) {
		DBG("%s: rdc input failed, packet discarded\n", __func__);
		goto out;
	}

	return;
out:
	l2_buf_unref(buf);
}

/* Reading incoming packet, so through SPI, cannot be done directly
 * the gpio callback since it's running in ISR context. Thus doing
 * it in an internal fiber
 */
static char __noinit __stack cc2520_read_stack[CC2520_READING_STACK_SIZE];

static void reading_packet_fiber(int unused1, int unused2)
{
	struct cc2520_config *info = cc2520_sgl_dev->config->config_info;

	while (1) {
		nano_fiber_sem_take(&info->read_lock, TICKS_UNLIMITED);

		cc2520_radio_lock();
		read_packet();
		cc2520_radio_unlock();

		last_packet_timestamp = cc2520_sfd_start_time;
		cc2520_packets_seen++;

		net_analyze_stack("CC2520 Rx Fiber stack", cc2520_read_stack,
				  CC2520_READING_STACK_SIZE);
	}
}

static void cc2520_gpio_int_handler(struct device *port, uint32_t pin)
{
	struct cc2520_config *info = cc2520_sgl_dev->config->config_info;

	DBG("%s: RX interrupt in pin %lu\n", __func__, pin);

	/* In order to make this driver available for 2+ instances
	 * it would require this handler to get access to the concerned
	 * instance
	 */

	CC2520_CLEAR_FIFOP_INT();

	nano_isr_sem_give(&info->read_lock);
}

void cc2520_set_txpower(uint8_t power)
{
	cc2520_radio_lock();
	set_txpower(power);
	cc2520_radio_unlock();
}

int cc2520_get_txpower(void)
{
	uint8_t power;

	cc2520_radio_lock();
	power = getreg(CC2520_TXPOWER);
	cc2520_radio_unlock();

	return power;
}

int cc2520_rssi(void)
{
	int radio_was_off = 0;
	int rssi;

	if (!locked) {
		return 0;
	}

	cc2520_radio_lock();

	if (!receive_on) {
		radio_was_off = 1;
		cc2520_on();
	}
	BUSYWAIT_UNTIL(status() & BIT(CC2520_RSSI_VALID), WAIT_10ms);

	rssi = (int)((signed char)getreg(CC2520_RSSI));

	if (radio_was_off) {
		cc2520_off();
	}

	cc2520_radio_unlock();

	return rssi;
}

int cc2520_cca_valid(void)
{
	int valid;

	if (locked) {
		return 0;
	}

	cc2520_radio_lock();
	valid = !!(status() & BIT(CC2520_RSSI_VALID));
	cc2520_radio_unlock();

	return valid;
}

static int cc2520_cca(void)
{
	int radio_was_off = 0;
	int cca = 1;

	if (locked) {
		return 1;
	}

	cc2520_radio_lock();

	if (!receive_on) {
		radio_was_off = 1;
		cc2520_on();
	}

	/* Make sure that the radio really got turned on. */
	if (receive_on) {
		BUSYWAIT_UNTIL(status() & BIT(CC2520_RSSI_VALID), WAIT_10ms);
		cca = CC2520_CCA_IS_1;
	}

	cc2520_radio_unlock();

	if (radio_was_off) {
		cc2520_off();
	}

	return cca;
}

void cc2520_set_cca_threshold(int value)
{
	cc2520_radio_lock();
	setreg(CC2520_CCACTRL0, value & 0xff);
	cc2520_radio_unlock();
}

static struct device *cc2520_spi_configure(void)
{
	struct device *spi;
	struct spi_config spi_conf = {
		.config = (8 << 4),
		.max_sys_freq = CONFIG_TI_CC2520_SPI_FREQ,
	};

	spi = device_get_binding(CONFIG_TI_CC2520_SPI_DRV_NAME);
	spi_configure(spi, &spi_conf);

	return spi;
}

static void cc2520_configure(struct device *dev)
{
	CC2520_DISABLE_FIFOP_INT();
	CC2520_FIFOP_INT_INIT();

	/* Initially reset must be set */
	SET_RESET_ACTIVE();
	SET_VREG_INACTIVE();
	clock_delay_usec_busywait(250);

	/* Turn on voltage regulator. */
	SET_VREG_ACTIVE();
	clock_delay_usec_busywait(400);

	/* Release reset */
	SET_RESET_INACTIVE();
	clock_delay_usec_busywait(800);

	/* Turn on the crystal oscillator. */
	if (!CC2520_STROBE_PLUS_NOP(CC2520_INS_SXOSCON)) {
		DBG("Strobe SXOSCON sending failed\n");
		return;
	}

	clock_delay_usec_busywait(800);

	BUSYWAIT_UNTIL((status() & BIT(CC2520_XOSC16M_STABLE)),	WAIT_10ms);
	if (!(status() & BIT(CC2520_XOSC16M_STABLE))) {
		DBG("Clock is not stabilized.\n");
		return;
	}

	/* Change default values as recommended in the data sheet, */
	/* correlation threshold = 20, RX bandpass filter = 1.3uA.*/

	setreg(CC2520_TXCTRL, 0x94);
	setreg(CC2520_TXPOWER, 0x13); /* Output power 1 dBm */

	/* TXPOWER values
	 * 0x03 -> -18 dBm
	 * 0x2C -> -7 dBm
	 * 0x88 -> -4 dBm
	 * 0x81 -> -2 dBm
	 * 0x32 -> 0 dBm
	 * 0x13 -> 1 dBm
	 * 0x32 -> 0 dBm
	 * 0x13 -> 1 dBm
	 * 0xAB -> 2 dBm
	 * 0xF2 -> 3 dBm
	 * 0xF7 -> 5 dBm
	 */
	setreg(CC2520_CCACTRL0, 0xF8); /* CCA threshold -80dBm */

	/* Recommended RX settings */
	setreg(CC2520_MDMCTRL0, 0x84); /* Controls modem */
	setreg(CC2520_MDMCTRL1, 0x14); /* Controls modem */
	setreg(CC2520_RXCTRL, 0x3F); /* Adjust currents in RX analog */
	setreg(CC2520_FSCTRL, 0x5A); /* Adjust currents in synt. */
	setreg(CC2520_FSCAL1, 0x2B); /* Adjust currents in VCO */
	setreg(CC2520_AGCCTRL1, 0x11); /* Adjust target for AGC ctrl loop */
	setreg(CC2520_AGCCTRL2, 0xEB);

	/*  Disable external clock */
	setreg(CC2520_EXTCLOCK, 0x00);

	/*  Tune ADC performance */
	setreg(CC2520_ADCTEST0, 0x10);
	setreg(CC2520_ADCTEST1, 0x0E);
	setreg(CC2520_ADCTEST2, 0x03);

	/* Set auto CRC on frame. */
#if CC2520_CONF_AUTOACK
	setreg(CC2520_FRMCTRL0, AUTOCRC | AUTOACK);
	setreg(CC2520_FRMFILT0, FRAME_MAX_VERSION|FRAME_FILTER_ENABLE);
#else
	/* setreg(CC2520_FRMCTRL0,    0x60); */
	setreg(CC2520_FRMCTRL0, AUTOCRC);
	/* Disable filter on @ (remove if you want to address specific wismote) */
	setreg(CC2520_FRMFILT0, 0x00);
#endif /* CC2520_CONF_AUTOACK */
	/* SET_RXENMASK_ON_TX */
	setreg(CC2520_FRMCTRL1, 1);
	/* Set FIFOP threshold to maximum .*/
	setreg(CC2520_FIFOPCTRL, FIFOP_THR(0x7F));

	if (!cc2520_set_pan_addr(0xffff, 0x0000, NULL)) {
		return;
	}

	cc2520_set_channel(CONFIG_TI_CC2520_CHANNEL);

	flushrx();

	cc2520_print_gpio_config();

	init_ok = true;
}

static radio_result_t get_object(radio_param_t param,
				 void *dest, size_t size)
{
	return RADIO_RESULT_NOT_SUPPORTED;
}

static radio_result_t set_object(radio_param_t param,
				 const void *src, size_t size)
{
	return RADIO_RESULT_NOT_SUPPORTED;
}

static int cc2520_contiki_init(void)
{
	return init_ok;
}

/* Contiki IP stack needs radio driver that it uses to deal with the
 * hardware. This driver API acts as a middle man between Contiki and
 * the Zephyr CC2520 driver. This api needs to external so that
 * Contiki stack can call the API functions.
 */
cc2520_driver_api_t cc2520_15_4_radio_driver = {
	.init = cc2520_contiki_init,
	.prepare = cc2520_prepare,
	.transmit = cc2520_transmit,
	.send = cc2520_send,
	.read = cc2520_read,
	.channel_clear = cc2520_cca,
	.receiving_packet = cc2520_receiving_packet,
	.pending_packet = cc2520_pending_packet,
	.on = cc2520_on,
	.off = cc2520_off,
	.get_value = cc2520_get_value,
	.set_value = cc2520_set_value,
	.get_object = get_object,
	.set_object = set_object,
};

static int cc2520_init(struct device *dev)
{
	struct cc2520_config *info = dev->config->config_info;

	DBG("%s setup\n", DRIVER_STR);

	dev->driver_api = &cc2520_15_4_radio_driver;
	cc2520_sgl_dev = dev;

	info->gpios = cc2520_gpio_configure();
	info->spi = cc2520_spi_configure();
	info->spi_slave = CONFIG_TI_CC2520_SPI_SLAVE;
	nano_sem_init(&info->read_lock);
	nano_sem_init(&info->radio_lock);

	cc2520_configure(dev);

	if (init_ok) {
		DBG("%s initialized on device: %p\n", DRIVER_STR, dev);

		nano_sem_give(&info->radio_lock);
		task_fiber_start(cc2520_read_stack, CC2520_READING_STACK_SIZE,
				 reading_packet_fiber, 0, 0, 0, 0);
	} else {
		cc2520_sgl_dev = NULL;
		DBG("%s initialization failed\n", DRIVER_STR);

		return DEV_FAIL;
	}

	return DEV_OK;
}

DEVICE_INIT(cc2520, CONFIG_CC2520_DRV_NAME,
	    cc2520_init, NULL, &cc2520_config,
	    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
