/*
 * Copyright (c) 2023 Basalte bv
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for RFID ISO 14443 Type A
 */

#ifndef ZEPHYR_INCLUDE_RFID_ISO14443A_H_
#define ZEPHYR_INCLUDE_RFID_ISO14443A_H_

#include <errno.h>

#include <zephyr/drivers/rfid.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RFID ISO 14443 Type A Interface
 * @defgroup rfid_iso14443a_interface RFID ISO 14443 Type A Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Command to request the presence of a card (SENS_REQ).
 */
#define RFID_ISO14443A_CMD_SENS_REQ 0x26U

/**
 * @brief Command to request data from all cards (ALL_REQ).
 */
#define RFID_ISO14443A_CMD_ALL_REQ 0x52U

/**
 * @brief Command to put the card into halt state (HALT).
 */
#define RFID_ISO14443A_CMD_HALT 0x50U

/**
 * @brief Command for Single Device Detection select class 1 (SDD_SEL_CL1).
 */
#define RFID_ISO14443A_CMD_SDD_SEL_CL1 0x93U

/**
 * @brief Command for Single Device Detection select class 2 (SDD_SEL_CL2).
 */
#define RFID_ISO14443A_CMD_SDD_SEL_CL2 0x95U

/**
 * @brief Command for Single Device Detection select class 3 (SDD_SEL_CL3).
 */
#define RFID_ISO14443A_CMD_SDD_SEL_CL3 0x97U

/**
 * @brief Command to request the Answer To Select (RATS).
 */
#define RFID_ISO14443A_CMD_RATS 0xE0U

/**
 * @brief Command to request Protocol Parameter Selection Start (PPSS).
 */
#define RFID_ISO14443A_CMD_PPSS 0xD0U

/**
 * @brief Command to request Protocol Parameter Selection byte 0 (PPS0).
 *
 */
#define RFID_ISO14443A_PPS_PPS0 0x01U

/**
 * @brief Command to request Protocol Parameter Selection byte 1 (PPS1).
 *
 */
#define RFID_ISO14443A_PPS_PPS1 0x00U

/**
 * @brief Tag indicating cascade level during UID processing (CASCADE_TAG).
 */
#define RFID_ISO14443A_CASCADE_TAG 0x88U

/**
 * @brief Mask for PCB block indicators.
 */
#define RFID_ISO14443_PCB_BLOCK_MASK 0xC0U

/**
 * @brief Indicates the presence of a block number in the PCB.
 */
#define RFID_ISO14443_PCB_BLOCK_NUM 0x01U

/**
 * @brief Indicates the presence of Node Addressing (NAD) in the PCB.
 */
#define RFID_ISO14443_PCB_BLOCK_NAD 0x04U

/**
 * @brief Indicates the presence of Card Identifier (CID) in the PCB.
 */
#define RFID_ISO14443_PCB_BLOCK_CID 0x08U

/**
 * @brief I-Block type used for data transfer.
 */
#define RFID_ISO14443_PCB_IBLOCK 0x00U

/**
 * @brief I-Block with fixed length.
 */
#define RFID_ISO14443_PCB_IBLOCK_FXD 0x02U

/**
 * @brief Indicates I-Block with chaining feature.
 */
#define RFID_ISO14443_PCB_IBLOCK_CHAINING 0x10U

/**
 * @brief R-Block type used for acknowledgment.
 */
#define RFID_ISO14443_PCB_RBLOCK 0x80U

/**
 * @brief R-Block with fixed length.
 */
#define RFID_ISO14443_PCB_RBLOCK_FXD 0x22U

/**
 * @brief R-Block indicating a negative acknowledgment (NAK).
 */
#define RFID_ISO14443_PCB_RBLOCK_NAK 0x10U

/**
 * @brief S-Block type used for control messages.
 */
#define RFID_ISO14443_PCB_SBLOCK 0xC0U

/**
 * @brief S-Block with fixed length.
 */
#define RFID_ISO14443_PCB_SBLOCK_FXD 0x02U

/**
 * @brief Mask for S-Block indicators.
 */
#define RFID_ISO14443_PCB_SBLOCK_MASK 0x30U

/**
 * @brief S-Block indicating a request for a wait time extension (WTX).
 */
#define RFID_ISO14443_PCB_SBLOCK_WTX 0x30U

/**
 * @brief S-Block indicating deselection of the card.
 */
#define RFID_ISO14443_PCB_SBLOCK_DESELECT 0x00U

/**
 * @brief Maximum data length code for ISO/IEC 14443 A ATQA.
 */
#define RFID_ISO14443A_MAX_ATQA_LEN    2U
/**
 * @brief Maximum data length code for ISO/IEC 14443 A UID.
 */
#define RFID_ISO14443A_MAX_UID_LEN     10U
/**
 * @brief Maximum data length code for ISO/IEC 14443 A ATS.
 */
#define RFID_ISO14443A_MAX_ATS_LEN     254U
/**
 * @brief Maximum data length code for ISO/IEC 14443 A history in ATS.
 */
#define RFID_ISO14443A_MAX_HISTORY_LEN 249U

/**
 * @brief ATQA UID single size type (4 bytes).
 */
#define RFID_ISO14443A_ATQA_UID_SINGLE 0x0U
/**
 * @brief ATQA UID double size type (7 bytes).
 */
#define RFID_ISO14443A_ATQA_UID_DOUBLE 0x1U
/**
 * @brief ATQA UID triple size type (10 bytes).
 */
#define RFID_ISO14443A_ATQA_UID_TRIPLE 0x2U

/**
 * @brief Indicates that the card supports cascade level processing in the SAK.
 */
#define RFID_ISO14443A_SAK_CASCADE 0x04U

/**
 * @brief Indicates that the card supports ATS (Answer to Select).
 */
#define RFID_ISO14443A_SAK_ATS_SUPPORTED 0x20U

#define RFID_ISO14443A_ATS_TA_PRESENT 0x10U
#define RFID_ISO14443A_ATS_TB_PRESENT 0x20U
#define RFID_ISO14443A_ATS_TC_PRESENT 0x40U

/**
 * @brief Seed value used for CRC16 calculations.
 */
#define RFID_ISO14443A_CRC16_SEED 0x6363

/**
 * @brief Polynomial used for CRC16 calculations.
 */
#define RFID_ISO14443A_CRC16_POLY 0x8408

/**
 * @brief Structure to hold information related to ISO 14443 A RFID cards.
 */
struct rfid_iso14443a_info {
	/**
	 * @brief ATQA (Answer To Request) response from the card.
	 *
	 * This array holds the ATQA response, which indicates the capabilities
	 * of the card. The length of this array is defined by the
	 * RFID_ISO14443A_MAX_ATQA_LEN constant.
	 */
	uint8_t atqa[RFID_ISO14443A_MAX_ATQA_LEN];

	/**
	 * @brief UID (Unique Identifier) of the card.
	 *
	 * This array holds the unique identifier for the card. The length of
	 * this array is defined by the RFID_ISO14443A_MAX_UID_LEN constant.
	 */
	uint8_t uid[RFID_ISO14443A_MAX_UID_LEN];

	/**
	 * @brief Length of the UID.
	 *
	 * This variable indicates the actual length of the UID received from
	 * the card.
	 */
	uint8_t uid_len;

	/**
	 * @brief SAK (Select Acknowledge) response from the card.
	 *
	 * This byte indicates the processing capabilities of the card, such
	 * as support for cascade level processing.
	 */
	uint8_t sak;

	/**
	 * @brief Union for card-specific parameters.
	 *
	 * This union can hold either the FSCI (Frame Size Control Indicator)
	 * or the FSDI (Frame Size Data Indicator), depending on the context.
	 */
	union {
		uint8_t fsci; /**< Frame Size Control Indicator. */
		uint8_t fsdi; /**< Frame Size Data Indicator. */
	};

	/**
	 * @brief CID (Card Identifier).
	 *
	 * This byte holds the identifier for the card when CID is supported.
	 */
	uint8_t cid;

	/**
	 * @brief Flag indicating whether the CID is supported by the card.
	 */
	bool cid_supported;

	/**
	 * @brief Flag indicating whether NAD (Node Addressing) is supported.
	 *
	 * This indicates if the card can support addressing features.
	 */
	bool nad_supported;

	/**
	 * @brief Communication modes supported by the card.
	 *
	 * This variable defines the modes that can be used for communication
	 * with the card as specified in the ISO 14443 A standard.
	 */
	rfid_mode_t modes;

	/**
	 * @brief SFGI (Start Frame Guard Interval).
	 *
	 * This value indicates the start frame guard interval for the card's
	 * communication.
	 */
	uint8_t sfgi;

	/**
	 * @brief FWI (Frame Wait Indicator).
	 *
	 * This value indicates the frame wait time that the card requires.
	 */
	uint8_t fwi;

	/**
	 * @brief History of communication with the card.
	 *
	 * This array holds the historical data received during ATS (Answer to
	 * Select) and other interactions. The length of this array is defined
	 * by the RFID_ISO14443A_MAX_HISTORY_LEN constant.
	 */
	uint8_t history[RFID_ISO14443A_MAX_HISTORY_LEN];

	/**
	 * @brief Length of the history data.
	 *
	 * This variable indicates the actual length of the history data
	 * received from the card.
	 */
	uint8_t history_len;

	/**
	 * @brief Number of blocks available on the card.
	 *
	 * This variable indicates the number of blocks that the card supports
	 * for data storage.
	 */
	uint8_t block_num;
};

/**
 * @brief Requests the presence of a card and receives the ATQA.
 *
 * This function sends a SENS_REQ command to request the presence of a card
 * and expects an ATQA response.
 *
 * @param dev Pointer to the device structure.
 * @param atqa Pointer to an array where the ATQA response will be stored.
 * @param sens Flag that specifies whether to enable the SENS_REQ functionality.
 * @return 0 on success, or a negative error code on failure.
 */
int rfid_iso14443a_request(const struct device *dev, uint8_t *atqa, bool sens);

/**
 * @brief Performs Single Device Detection (SDD).
 *
 * This function sends a Single Device Detection command to the card
 * and updates the given info structure with the card's UID and SAK.
 *
 * @param dev Pointer to the device structure.
 * @param info Pointer to the structure where UID and SAK will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
int rfid_iso14443a_sdd(const struct device *dev, struct rfid_iso14443a_info *info);

/**
 * @brief Requests the Answer To Select (RATS) from the card.
 *
 * This function sends a RATS command and expects a response containing
 * the protocol parameters for the selected card.
 *
 * @param dev Pointer to the device structure.
 * @param info Pointer to the structure where card information will be updated.
 * @param cid Card Identifier.
 * @return 0 on success, or a negative error code on failure.
 */
int rfid_iso14443a_rats(const struct device *dev, struct rfid_iso14443a_info *info, uint8_t cid);

/**
 * @brief Performs Protocol and Parameter Selection (PPS).
 *
 * This function sends a PPS command to select specific communication
 * parameters for the card.
 *
 * @param dev Pointer to the device structure.
 * @param info Pointer to the structure where card information will be updated.
 * @param modes Modes specifying the desired communication parameters.
 * @return 0 on success, or a negative error code on failure.
 */
int rfid_iso14443a_pps(const struct device *dev, struct rfid_iso14443a_info *info,
		       rfid_mode_t modes);

/**
 * @brief Puts the card into halt state.
 *
 * This function sends a HALT command to the card, causing it to enter
 * the halt state and stop responding until reactivated.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, or a negative error code on failure.
 */
int rfid_iso14443a_halt(const struct device *dev);

/**
 * @brief Exchanges data with the card.
 *
 * This function sends data to the card and waits for a response.
 * The card's response is stored in the provided rx_data buffer.
 *
 * @param dev Pointer to the device structure.
 * @param info Pointer to the structure where card information will be updated.
 * @param tx_data Pointer to the data to be sent.
 * @param tx_len Length of the data to be sent.
 * @param rx_data Pointer to buffer where the response data will be stored.
 * @param rx_len Pointer to variable that holds the size of the response data.
 * @param nad Node Address (NAD) for communication.
 * @return 0 on success, or a negative error code on failure.
 */
int rfid_iso14443a_exchange(const struct device *dev, struct rfid_iso14443a_info *info,
			    const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data,
			    uint16_t *rx_len, uint8_t nad);

/**
 * @brief Handles the target mode for ISO 14443 A.
 *
 * This function puts the device into a listening state to receive commands
 * sent by a reader while acting as an RFID card in target mode.
 *
 * @param dev Pointer to the device structure.
 * @param info Pointer to the structure where card information will be updated.
 * @return 0 on success, or a negative error code on failure.
 */
int rfid_iso14443a_listen(const struct device *dev, struct rfid_iso14443a_info *info);

/**
 * @brief Receives data while acting as an RFID card in target mode.
 *
 * This function receives data from a reader and updates the provided
 * buffer with the response data. It should be used when the device is
 * operating as an RFID card.
 *
 * @param dev Pointer to the device structure.
 * @param info Pointer to the structure where card information will be updated.
 * @param rx_data Pointer to buffer where the received data will be stored.
 * @param rx_len Pointer to variable that holds the size of the received data.
 * @param nad Node Address (NAD) for communication.
 * @return 0 on success, or a negative error code on failure.
 */
int rfid_iso14443a_receive(const struct device *dev, struct rfid_iso14443a_info *info,
			   uint8_t *rx_data, uint16_t *rx_len, uint8_t *nad);

/**
 * @brief Transmits data while acting as an RFID card in target mode.
 *
 * This function sends data to a reader while operating in target mode
 * as an RFID card. It allows for communication from the card to the reader.
 *
 * @param dev Pointer to the device structure.
 * @param info Pointer to the structure where card information will be updated.
 * @param tx_data Pointer to the data to be sent.
 * @param tx_len Length of the data to be sent.
 * @param nad Node Address (NAD) for communication.
 * @return 0 on success, or a negative error code on failure.
 */
int rfid_iso14443a_transmit(const struct device *dev, struct rfid_iso14443a_info *info,
			    const uint8_t *tx_data, uint16_t tx_len, uint8_t nad);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_RFID_ISO14443A_H_ */
