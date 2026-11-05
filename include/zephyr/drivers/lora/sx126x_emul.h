/*
 * Copyright (c) 2026 Embeint Inc
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Backend API for the emulated Semtech SX126x transceiver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LORA_SX126X_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_LORA_SX126X_EMUL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Emulated SX126x transceiver
 * @defgroup sx126x_emul SX126x emulator
 * @ingroup io_emulators
 *
 * The emulator sits on an emulated SPI bus and models the command interface
 * of the SX1261, SX1262, SX1268 and LLCC68 (DS.SX1261-2.W.APP Rev 2.2): the
 * BUSY, DIO1 and NRESET lines, operating modes including sleep and RX duty
 * cycle, register retention across warm-start sleep, the data buffer and the
 * IRQ status. BUSY rises after every write command and stays high for the
 * startup, wake-up and mode switching times of the datasheet; commands sent
 * while it is high are errors. Commands that the datasheet forbids in the current state, that
 * carry out-of-range parameters or that leave a documented workaround
 * unapplied are counted as errors rather than rejected, so a test can assert
 * that a driver never committed one.
 *
 * @{
 */

/** Largest payload the radio carries */
#define SX126X_EMUL_MAX_PAYLOAD 255

/** Operating mode of the emulated radio */
enum sx126x_emul_mode {
	/** NRESET held low */
	SX126X_EMUL_MODE_RESET,
	/** Sleep, BUSY high until the next NSS falling edge */
	SX126X_EMUL_MODE_SLEEP,
	/** Standby on the RC oscillator */
	SX126X_EMUL_MODE_STDBY_RC,
	/** Standby on the crystal oscillator */
	SX126X_EMUL_MODE_STDBY_XOSC,
	/** Frequency synthesis */
	SX126X_EMUL_MODE_FS,
	/** Transmitting a packet, a continuous wave or an infinite preamble */
	SX126X_EMUL_MODE_TX,
	/** Receiving */
	SX126X_EMUL_MODE_RX,
	/** Receive duty cycle, alternating receive windows and sleep with BUSY high */
	SX126X_EMUL_MODE_RX_DUTY_CYCLE,
};

/** Packet type selected with SetPacketType */
enum sx126x_emul_packet_type {
	/** GFSK modem */
	SX126X_EMUL_PACKET_TYPE_GFSK = 0,
	/** LoRa modem */
	SX126X_EMUL_PACKET_TYPE_LORA = 1,
};

/** LoRa modem configuration, decoded from the radio's parameters */
struct sx126x_emul_lora_params {
	/** Spreading factor, 5 to 12 */
	uint8_t sf;
	/** Bandwidth in Hz, 0 if the register value is invalid */
	uint32_t bandwidth;
	/** Coding rate denominator (4/x), 5 to 8, 0 if invalid */
	uint8_t cr;
	/** Coding rate uses the long interleaver */
	bool long_interleave;
	/** Low data rate optimization enabled */
	bool ldro;
	/** Preamble length in symbols */
	uint16_t preamble_len;
	/** Implicit (fixed length) header */
	bool implicit_header;
	/** Payload length from SetPacketParams */
	uint8_t payload_len;
	/** Payload CRC enabled */
	bool crc;
	/** IQ inverted */
	bool iq_inverted;
	/** Sync word register contents */
	uint16_t sync_word;
};

/** GFSK modem configuration, decoded from the radio's parameters */
struct sx126x_emul_gfsk_params {
	/** Bit rate in bit/s */
	uint32_t bitrate;
	/** Frequency deviation in Hz */
	uint32_t freq_deviation;
	/** Receive bandwidth (double sideband) in Hz, 0 if invalid */
	uint32_t bandwidth;
	/** Pulse shape register value */
	uint8_t pulse_shape;
	/** Preamble length in bits */
	uint16_t preamble_bits;
	/** Preamble detector length register value */
	uint8_t preamble_detect;
	/** Sync word length in bits */
	uint8_t sync_word_bits;
	/** Address filtering register value */
	uint8_t addr_comp;
	/** Variable length packet (length byte on air) */
	bool variable_len;
	/** Payload length from SetPacketParams */
	uint8_t payload_len;
	/** CRC type register value */
	uint8_t crc_type;
	/** Data whitening enabled */
	bool whitening;
	/** Sync word register contents */
	uint8_t sync_word[8];
	/** CRC seed register contents */
	uint16_t crc_init;
	/** CRC polynomial register contents */
	uint16_t crc_poly;
	/** 9-bit whitening seed register contents */
	uint16_t whitening_init;
};

/** Radio configuration as the emulator holds it */
struct sx126x_emul_radio {
	/** Selected packet type */
	enum sx126x_emul_packet_type packet_type;
	/** RF frequency in Hz */
	uint32_t frequency;
	/** Output power from SetTxParams in dBm */
	int8_t tx_power;
	/** Ramp time register value from SetTxParams */
	uint8_t ramp_time;
	/** SetPaConfig parameters */
	struct {
		/** paDutyCycle */
		uint8_t duty_cycle;
		/** hpMax */
		uint8_t hp_max;
		/** deviceSel */
		uint8_t device_sel;
		/** paLut */
		uint8_t pa_lut;
	} pa;
	/** RX gain register (0x08AC) */
	uint8_t rx_gain;
	/** IQ polarity register (0x0736) */
	uint8_t iq_polarity;
	/** TX modulation register (0x0889) */
	uint8_t tx_modulation;
	/** DC-DC regulator selected rather than the LDO */
	bool dcdc;
	/** DIO3 drives a TCXO */
	bool tcxo;
	/** TCXO voltage register value */
	uint8_t tcxo_voltage;
	/** DIO2 drives the RF switch */
	bool dio2_rf_switch;
	/** IRQs enabled by SetDioIrqParams */
	uint16_t irq_mask;
	/** IRQs routed to DIO1 */
	uint16_t dio1_mask;
	/** LoRa parameters, valid when @ref packet_type is LoRa */
	struct sx126x_emul_lora_params lora;
	/** GFSK parameters, valid when @ref packet_type is GFSK */
	struct sx126x_emul_gfsk_params gfsk;
};

/** A transmitted packet, captured when SetTx is issued */
struct sx126x_emul_tx_packet {
	/** Radio configuration at the time of SetTx */
	struct sx126x_emul_radio radio;
	/** SetTx timeout in 15.625 us steps, 0 for none */
	uint32_t timeout;
	/** Payload length */
	uint8_t len;
	/** Payload */
	uint8_t data[SX126X_EMUL_MAX_PAYLOAD];
};

/** A packet to be received */
struct sx126x_emul_rx_packet {
	/** Payload length */
	uint8_t len;
	/** Payload */
	uint8_t data[SX126X_EMUL_MAX_PAYLOAD];
	/** Packet RSSI in dBm */
	int16_t rssi;
	/** Packet SNR in dB, LoRa only */
	int8_t snr;
	/** Corrupt the packet: reported as a CRC error when the CRC is enabled */
	bool crc_error;
};

/** Event counters */
struct sx126x_emul_stats {
	/** NRESET assertions */
	uint32_t resets;
	/** Commands executed */
	uint32_t commands;
	/** Entries into sleep */
	uint32_t sleeps;
	/** Wake-ups from sleep or the sleep phase of an RX duty cycle */
	uint32_t wakeups;
	/** SetTx commands */
	uint32_t tx_starts;
	/** Completed transmissions */
	uint32_t tx_done;
	/** Transmissions ended by their timeout */
	uint32_t tx_timeouts;
	/** SetRx and SetRxDutyCycle commands */
	uint32_t rx_starts;
	/** Delivered packets */
	uint32_t rx_done;
	/** Receptions ended by their timeout */
	uint32_t rx_timeouts;
};

/**
 * @brief Transmission callback
 *
 * Called from the system work queue when a transmission completes, with the
 * emulator locked. The callback may queue a reply with
 * sx126x_emul_rx_packet_set().
 *
 * @param target Emulator that transmitted
 * @param pkt Transmitted packet
 * @param user_data User data given to sx126x_emul_tx_cb_set()
 */
typedef void (*sx126x_emul_tx_cb_t)(const struct emul *target,
				    const struct sx126x_emul_tx_packet *pkt, void *user_data);

/**
 * @brief Command callback
 *
 * Called after each command the radio executes, from the thread that issued
 * the SPI transaction and with the emulator locked. The callback runs before
 * the transaction completes and before BUSY and DIO1 are updated, so IRQs it
 * returns are visible to the host as the command finishes. Commands lost
 * waking the radio from sleep or sent while it is held in reset are not
 * reported.
 *
 * The callback may call the other functions in this API but must not block.
 *
 * @param target Emulator that executed the command
 * @param opcode Command opcode
 * @param params Bytes the host sent after the opcode, including any NOPs
 * @param len Number of bytes in @p params
 * @param user_data User data given to sx126x_emul_cmd_cb_set()
 *
 * @return IRQs to raise, 0 for none. Only IRQs enabled in the radio's IRQ mask
 *         latch, and the operating mode is not changed.
 */
typedef uint16_t (*sx126x_emul_cmd_cb_t)(const struct emul *target, uint8_t opcode,
					 const uint8_t *params, size_t len, void *user_data);

/**
 * @brief Get the operating mode
 *
 * @param target Emulator
 *
 * @return Current operating mode
 */
enum sx126x_emul_mode sx126x_emul_mode_get(const struct emul *target);

/**
 * @brief Get the number of protocol errors seen
 *
 * Every error is also logged at error level.
 *
 * @param target Emulator
 *
 * @return Errors since boot or the last sx126x_emul_errors_clear()
 */
uint32_t sx126x_emul_errors_get(const struct emul *target);

/**
 * @brief Reset the protocol error count to zero
 *
 * @param target Emulator
 */
void sx126x_emul_errors_clear(const struct emul *target);

/**
 * @brief Get the event counters
 *
 * @param target Emulator
 * @param stats Filled with the counters
 */
void sx126x_emul_stats_get(const struct emul *target, struct sx126x_emul_stats *stats);

/**
 * @brief Reset the event counters to zero
 *
 * @param target Emulator
 */
void sx126x_emul_stats_clear(const struct emul *target);

/**
 * @brief Get the current radio configuration
 *
 * @param target Emulator
 * @param radio Filled with the configuration
 */
void sx126x_emul_radio_get(const struct emul *target, struct sx126x_emul_radio *radio);

/**
 * @brief Get the most recently transmitted packet
 *
 * @param target Emulator
 * @param pkt Filled with the packet
 *
 * @retval 0 Packet returned
 * @retval -ENODATA Nothing transmitted since boot
 */
int sx126x_emul_last_tx_get(const struct emul *target, struct sx126x_emul_tx_packet *pkt);

/**
 * @brief Get the radio configuration of the most recent reception
 *
 * Captured when SetRx or SetRxDutyCycle is issued.
 *
 * @param target Emulator
 * @param radio Filled with the configuration
 *
 * @retval 0 Configuration returned
 * @retval -ENODATA No reception started since boot
 */
int sx126x_emul_last_rx_get(const struct emul *target, struct sx126x_emul_radio *radio);

/**
 * @brief Queue a packet for reception
 *
 * The packet's preamble is detected shortly after the radio is, or next
 * becomes, receiving. The packet is received once the configured preamble and
 * the sync word (GFSK) or header (LoRa) have taken their time on air; an RX
 * timeout that expires first loses it. One packet is queued at a time; a
 * second call replaces the first.
 *
 * @param target Emulator
 * @param pkt Packet to receive
 *
 * @retval 0 Packet queued
 * @retval -EINVAL Packet longer than @ref SX126X_EMUL_MAX_PAYLOAD
 */
int sx126x_emul_rx_packet_set(const struct emul *target, const struct sx126x_emul_rx_packet *pkt);

/**
 * @brief Set the level GetRssiInst reports
 *
 * @param target Emulator
 * @param rssi Level in dBm
 */
void sx126x_emul_rssi_set(const struct emul *target, int16_t rssi);

/**
 * @brief Set how long a transmission takes
 *
 * The transmission starts when BUSY falls after SetTx, which takes the mode
 * switch and any TCXO start-up delay, rounded up to a system tick. A duration
 * longer than the SetTx timeout ends the transmission with a timeout IRQ
 * instead of TX done. Defaults to 1 ms.
 *
 * @param target Emulator
 * @param duration Time from the start of the transmission to TX done
 */
void sx126x_emul_tx_duration_set(const struct emul *target, k_timeout_t duration);

/**
 * @brief Set the transmission callback
 *
 * @param target Emulator
 * @param cb Callback, NULL to remove it
 * @param user_data Passed to @p cb
 */
void sx126x_emul_tx_cb_set(const struct emul *target, sx126x_emul_tx_cb_t cb, void *user_data);

/**
 * @brief Set the command callback
 *
 * @param target Emulator
 * @param cb Callback, NULL to remove it
 * @param user_data Passed to @p cb
 */
void sx126x_emul_cmd_cb_set(const struct emul *target, sx126x_emul_cmd_cb_t cb, void *user_data);

/**
 * @brief Read registers without going through the SPI bus
 *
 * @param target Emulator
 * @param addr First register address
 * @param buf Filled with the register values
 * @param len Number of registers
 *
 * @retval 0 Registers read
 * @retval -EINVAL Range outside the register space
 */
int sx126x_emul_reg_read(const struct emul *target, uint16_t addr, uint8_t *buf, size_t len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_LORA_SX126X_EMUL_H_ */
