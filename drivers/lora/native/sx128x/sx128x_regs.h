/*
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * SX128X SPI command opcodes, IRQ bits and LoRa modulation/packet parameter
 * encodings, taken from the Semtech SX1280/SX1281 datasheet.
 */

#ifndef ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_REGS_H_
#define ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_REGS_H_

#include <zephyr/sys/util.h>

/* SPI commands */
#define SX128X_CMD_GET_STATUS                 0xC0
#define SX128X_CMD_WRITE_REGISTER             0x18
#define SX128X_CMD_READ_REGISTER              0x19
#define SX128X_CMD_WRITE_BUFFER               0x1A
#define SX128X_CMD_READ_BUFFER                0x1B
#define SX128X_CMD_SET_SLEEP                  0x84
#define SX128X_CMD_SET_STANDBY                0x80
#define SX128X_CMD_SET_FS                     0xC1
#define SX128X_CMD_SET_TX                     0x83
#define SX128X_CMD_SET_RX                     0x82
#define SX128X_CMD_SET_RX_DUTY_CYCLE          0x94
#define SX128X_CMD_SET_CAD                    0xC5
#define SX128X_CMD_SET_TX_CONTINUOUS_WAVE     0xD1
#define SX128X_CMD_SET_TX_CONTINUOUS_PREAMBLE 0xD2
#define SX128X_CMD_SET_PACKET_TYPE            0x8A
#define SX128X_CMD_GET_PACKET_TYPE            0x03
#define SX128X_CMD_SET_RF_FREQUENCY           0x86
#define SX128X_CMD_SET_TX_PARAMS              0x8E
#define SX128X_CMD_SET_CAD_PARAMS             0x88
#define SX128X_CMD_SET_BUFFER_BASE_ADDRESS    0x8F
#define SX128X_CMD_SET_MODULATION_PARAMS      0x8B
#define SX128X_CMD_SET_PACKET_PARAMS          0x8C
#define SX128X_CMD_GET_RX_BUFFER_STATUS       0x17
#define SX128X_CMD_GET_PACKET_STATUS          0x1D
#define SX128X_CMD_GET_RSSI_INST              0x1F
#define SX128X_CMD_SET_DIO_IRQ_PARAMS         0x8D
#define SX128X_CMD_GET_IRQ_STATUS             0x15
#define SX128X_CMD_CLEAR_IRQ_STATUS           0x97
#define SX128X_CMD_SET_REGULATOR_MODE         0x96
#define SX128X_CMD_SET_SAVE_CONTEXT           0xD5
#define SX128X_CMD_SET_AUTO_TX                0x98
#define SX128X_CMD_SET_AUTO_FS                0x9E
#define SX128X_CMD_SET_LONG_PREAMBLE          0x9B

#define SX128X_REG_LORA_SF_CONFIG  0x0925
#define SX128X_REG_LORA_CAD_PEAK   0x0942
#define SX128X_REG_LORA_SYNC_WORD  0x0944
#define SX128X_REG_LORA_FREQ_ERROR 0x093C

/* LoRa sync word register values for the public_network setting */
#define SX128X_LORA_SYNC_WORD_PUBLIC  0x3444
#define SX128X_LORA_SYNC_WORD_PRIVATE 0x1424

/* IRQ bits (16-bit) */
#define SX128X_IRQ_TX_DONE                   BIT(0)
#define SX128X_IRQ_RX_DONE                   BIT(1)
#define SX128X_IRQ_SYNCWORD_VALID            BIT(2)
#define SX128X_IRQ_SYNCWORD_ERROR            BIT(3)
#define SX128X_IRQ_HEADER_VALID              BIT(4)
#define SX128X_IRQ_HEADER_ERROR              BIT(5)
#define SX128X_IRQ_CRC_ERROR                 BIT(6)
#define SX128X_IRQ_RANGING_SLAVE_RESP_DONE   BIT(7)
#define SX128X_IRQ_RANGING_SLAVE_REQ_DISCARD BIT(8)
#define SX128X_IRQ_RANGING_MASTER_RES_VALID  BIT(9)
#define SX128X_IRQ_RANGING_MASTER_TIMEOUT    BIT(10)
#define SX128X_IRQ_RANGING_SLAVE_REQ_VALID   BIT(11)
#define SX128X_IRQ_CAD_DONE                  BIT(12)
#define SX128X_IRQ_CAD_DETECTED              BIT(13)
#define SX128X_IRQ_RX_TX_TIMEOUT             BIT(14)
#define SX128X_IRQ_PREAMBLE_DETECTED         BIT(15)
#define SX128X_IRQ_ALL                       0xFFFF

/* Packet types (SetPacketType / GetPacketType) */
#define SX128X_PACKET_TYPE_GFSK    0x00
#define SX128X_PACKET_TYPE_LORA    0x01
#define SX128X_PACKET_TYPE_RANGING 0x02
#define SX128X_PACKET_TYPE_FLRC    0x03
#define SX128X_PACKET_TYPE_BLE     0x04

/* SetStandby argument */
#define SX128X_STDBY_RC   0x00
#define SX128X_STDBY_XOSC 0x01

/* SetRegulatorMode argument */
#define SX128X_REGULATOR_LDO  0x00
#define SX128X_REGULATOR_DCDC 0x01

/* SetTxParams ramp time used for LoRa operations. */
#define SX128X_TX_RAMP_20_US 0xE0

/* LoRa modulation params: spreading factor (ModulationParam1) */
#define SX128X_LORA_SF5  0x50
#define SX128X_LORA_SF6  0x60
#define SX128X_LORA_SF7  0x70
#define SX128X_LORA_SF8  0x80
#define SX128X_LORA_SF9  0x90
#define SX128X_LORA_SF10 0xA0
#define SX128X_LORA_SF11 0xB0
#define SX128X_LORA_SF12 0xC0

/* LoRa modulation params: bandwidth (ModulationParam2) - only 4 bandwidths
 * are valid in the SX128X's 2.4 GHz LoRa mode.
 */
#define SX128X_LORA_BW_1625 0x0A
#define SX128X_LORA_BW_812  0x18
#define SX128X_LORA_BW_406  0x26
#define SX128X_LORA_BW_203  0x34

/* LoRa modulation params: coding rate (ModulationParam3) */
#define SX128X_LORA_CR_4_5    0x01
#define SX128X_LORA_CR_4_6    0x02
#define SX128X_LORA_CR_4_7    0x03
#define SX128X_LORA_CR_4_8    0x04
#define SX128X_LORA_CR_LI_4_5 0x05
#define SX128X_LORA_CR_LI_4_6 0x06
#define SX128X_LORA_CR_LI_4_8 0x07

/* LoRa packet params: header type (PacketParam2) */
#define SX128X_LORA_HEADER_EXPLICIT 0x00
#define SX128X_LORA_HEADER_IMPLICIT 0x80

/* LoRa packet params: CRC (PacketParam4) */
#define SX128X_LORA_CRC_ON  0x20
#define SX128X_LORA_CRC_OFF 0x00

/* LoRa packet params: IQ (PacketParam5) */
#define SX128X_LORA_IQ_STD      0x40
#define SX128X_LORA_IQ_INVERTED 0x00

/* RF frequency register: Freq_reg = round(Freq_Hz * 2^18 / Fxtal), Fxtal = 52 MHz */
#define SX128X_XTAL_FREQ_HZ    52000000ULL
#define SX128X_FREQ_STEP_SHIFT 18

#define SX128X_MAX_PAYLOAD_LEN 255

/* Half of the 256-byte data buffer: two frames this long fit side by side */
#define SX128X_TX_HALF_LEN 128

/* The chip tunes across the 2.4 GHz ISM band and a little beyond it */
#define SX128X_FREQ_MIN_HZ 2400000000U
#define SX128X_FREQ_MAX_HZ 2500000000U

/* GFSK modulation params: bit rate and DSB bandwidth (ModulationParam1) */
#define SX128X_GFSK_BR_2000_BW_2400 0x04
#define SX128X_GFSK_BR_1600_BW_2400 0x28
#define SX128X_GFSK_BR_1000_BW_2400 0x4C
#define SX128X_GFSK_BR_1000_BW_1200 0x45
#define SX128X_GFSK_BR_800_BW_2400  0x70
#define SX128X_GFSK_BR_800_BW_1200  0x69
#define SX128X_GFSK_BR_500_BW_1200  0x8D
#define SX128X_GFSK_BR_500_BW_600   0x86
#define SX128X_GFSK_BR_400_BW_1200  0xB1
#define SX128X_GFSK_BR_400_BW_600   0xAA
#define SX128X_GFSK_BR_250_BW_600   0xCE
#define SX128X_GFSK_BR_250_BW_300   0xC7
#define SX128X_GFSK_BR_125_BW_300   0xEF

/* GFSK modulation params: modulation index 0.35, then 0.5 to 4.0 in 0.25
 * steps (ModulationParam2)
 */
#define SX128X_GFSK_MOD_IND_0_35 0x00
#define SX128X_GFSK_MOD_IND_4_00 0x0F

/* GFSK modulation params: Gaussian filter (ModulationParam3) */
#define SX128X_GFSK_BT_OFF 0x00
#define SX128X_GFSK_BT_1_0 0x10
#define SX128X_GFSK_BT_0_5 0x20

/* GFSK packet params: preamble of 4 to 32 bits (PacketParam1) */
#define SX128X_GFSK_PREAMBLE_BITS(bits) ((((bits) / 4) - 1) << 4)
#define SX128X_GFSK_PREAMBLE_MAX_BITS   32

/* GFSK packet params: sync word length, 1 to 5 bytes (PacketParam2) */
#define SX128X_GFSK_SYNC_WORD_LEN(bytes) (((bytes) - 1) << 1)
#define SX128X_GFSK_SYNC_WORD_MAX_LEN    5

/* GFSK packet params: sync word matching (PacketParam3) */
#define SX128X_GFSK_SYNC_WORD_OFF 0x00
#define SX128X_GFSK_SYNC_WORD_1   0x10

/* GFSK packet params: packet length mode (PacketParam4) */
#define SX128X_GFSK_PACKET_FIXED    0x00
#define SX128X_GFSK_PACKET_VARIABLE 0x20

/* GFSK packet params: CRC length (PacketParam6) */
#define SX128X_GFSK_CRC_OFF     0x00
#define SX128X_GFSK_CRC_2_BYTES 0x20

/* GFSK packet params: whitening (PacketParam7) */
#define SX128X_GFSK_WHITENING_ON  0x00
#define SX128X_GFSK_WHITENING_OFF 0x08

/*
 * Sync word 1 occupies 0x09CE (bits 39:32) to 0x09D2 (bits 7:0). A shorter
 * sync word is its low bytes, so it ends at the same register.
 */
#define SX128X_REG_GFSK_SYNC_WORD_1_END 0x09D3

/* Length byte of a variable-length GFSK frame, plus one RFU bit */
#define SX128X_GFSK_HEADER_BITS 9

#endif /* ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_REGS_H_ */
