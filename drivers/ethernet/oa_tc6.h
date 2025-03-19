/*
 * Copyright (c) 2023 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef OA_TC6_CFG_H__
#define OA_TC6_CFG_H__

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/net_pkt.h>

#define MMS_REG(m, r)        ((((m) & GENMASK(3, 0)) << 16) | ((r) & GENMASK(15, 0)))
/* Memory Map Sector (MMS) 0 */
#define OA_ID                MMS_REG(0x0, 0x000) /* expect 0x11 */
#define OA_PHYID             MMS_REG(0x0, 0x001)
#define OA_RESET             MMS_REG(0x0, 0x003)
#define OA_RESET_SWRESET     BIT(0)
#define OA_CONFIG0           MMS_REG(0x0, 0x004)
#define OA_CONFIG0_SYNC      BIT(15)
#define OA_CONFIG0_RFA_ZARFE BIT(12)
#define OA_CONFIG0_PROTE     BIT(5)
#define OA_STATUS0           MMS_REG(0x0, 0x008)
#define OA_STATUS0_RESETC    BIT(6)
#define OA_STATUS1           MMS_REG(0x0, 0x009)
#define OA_BUFSTS            MMS_REG(0x0, 0x00B)
#define OA_BUFSTS_TXC        GENMASK(15, 8)
#define OA_BUFSTS_RCA        GENMASK(7, 0)
#define OA_IMASK0            MMS_REG(0x0, 0x00C)
#define OA_IMASK0_TXPEM      BIT(0)
#define OA_IMASK0_TXBOEM     BIT(1)
#define OA_IMASK0_TXBUEM     BIT(2)
#define OA_IMASK0_RXBOEM     BIT(3)
#define OA_IMASK0_LOFEM      BIT(4)
#define OA_IMASK0_HDREM      BIT(5)
#define OA_IMASK1            MMS_REG(0x0, 0x00D)
#define OA_IMASK0_UV18M      BIT(19)

/* OA Control header */
#define OA_CTRL_HDR_DNC  BIT(31)
#define OA_CTRL_HDR_HDRB BIT(30)
#define OA_CTRL_HDR_WNR  BIT(29)
#define OA_CTRL_HDR_AID  BIT(28)
#define OA_CTRL_HDR_MMS  GENMASK(27, 24)
#define OA_CTRL_HDR_ADDR GENMASK(23, 8)
#define OA_CTRL_HDR_LEN  GENMASK(7, 1)
#define OA_CTRL_HDR_P    BIT(0)

/* OA Data header */
#define OA_DATA_HDR_DNC  BIT(31)
#define OA_DATA_HDR_SEQ  BIT(30)
#define OA_DATA_HDR_NORX BIT(29)
#define OA_DATA_HDR_DV   BIT(21)
#define OA_DATA_HDR_SV   BIT(20)
#define OA_DATA_HDR_SWO  GENMASK(19, 16)
#define OA_DATA_HDR_EV   BIT(14)
#define OA_DATA_HDR_EBO  GENMASK(13, 8)
#define OA_DATA_HDR_P    BIT(0)

/* OA Data footer */
#define OA_DATA_FTR_EXST BIT(31)
#define OA_DATA_FTR_HDRB BIT(30)
#define OA_DATA_FTR_SYNC BIT(29)
#define OA_DATA_FTR_RCA  GENMASK(28, 24)
#define OA_DATA_FTR_DV   BIT(21)
#define OA_DATA_FTR_SV   BIT(20)
#define OA_DATA_FTR_SWO  GENMASK(19, 16)
#define OA_DATA_FTR_FD   BIT(15)
#define OA_DATA_FTR_EV   BIT(14)
#define OA_DATA_FTR_EBO  GENMASK(13, 8)
#define OA_DATA_FTR_TXC  GENMASK(5, 1)
#define OA_DATA_FTR_P    BIT(0)

#define OA_TC6_HDR_SIZE          4
#define OA_TC6_FTR_SIZE          4
#define OA_TC6_BUF_ALLOC_TIMEOUT K_MSEC(10)
#define OA_TC6_FTR_RCA_MAX       GENMASK(4, 0)
#define OA_TC6_FTR_TXC_MAX       GENMASK(4, 0)

/* PHY Clause 22 registers base address and mask */
#define OA_TC6_PHY_STD_REG_ADDR_BASE 0xFF00
#define OA_TC6_PHY_STD_REG_ADDR_MASK 0x1F

/* PHY – Clause 45 registers memory map selector (MMS) as per table 6 in the
 * OPEN Alliance specification.
 */
#define OA_TC6_PHY_C45_PCS_MMS2      2 /* MMD 3 */
#define OA_TC6_PHY_C45_PMA_PMD_MMS3  3 /* MMD 1 */
#define OA_TC6_PHY_C45_VS_PLCA_MMS4  4 /* MMD 31 */
#define OA_TC6_PHY_C45_AUTO_NEG_MMS5 5 /* MMD 7 */

/**
 * @brief OA TC6 data.
 */
struct oa_tc6 {
	/** Pointer to SPI device */
	const struct spi_dt_spec *spi;

	/** OA data payload (chunk) size */
	uint8_t cps;

	/**
	 * Number of available chunks buffers in OA TC6 device to store
	 * data for transmission
	 */
	uint8_t txc;

	/** Number of available chunks to read from OA TC6 device */
	uint8_t rca;

	/** Indication of pending interrupt in OA TC6 device */
	bool exst;

	/** Indication of OA TC6 device being ready for transmission */
	bool sync;

	/** Indication of protected control transmission mode */
	bool protected;

	/** Pointer to network buffer concatenated from received chunk */
	struct net_buf *concat_buf;
};

/**
 * @brief Calculate parity bit from data
 *
 * @param x data to calculate parity
 *
 * @return 0 if number of ones is odd, 1 otherwise.
 */
static inline bool oa_tc6_get_parity(const uint32_t x)
{
	uint32_t y;

	y = x ^ (x >> 1);
	y = y ^ (y >> 2);
	y = y ^ (y >> 4);
	y = y ^ (y >> 8);
	y = y ^ (y >> 16);

	return !(y & 1);
}

/**
 * @brief Read OA TC6 compliant device single register
 *
 * @param tc6 OA TC6 specific data
 *
 * @param reg register to read

 * @param val pointer to variable to store read value
 *
 * @return 0 if read was successful, <0 otherwise.
 */
int oa_tc6_reg_read(struct oa_tc6 *tc6, const uint32_t reg, uint32_t *val);

/**
 * @brief Write to OA TC6 compliant device a single register
 *
 * @param tc6 OA TC6 specific data
 *
 * @param reg register to read

 * @param val data to send to device
 *
 * @return 0 if write was successful, <0 otherwise.
 */
int oa_tc6_reg_write(struct oa_tc6 *tc6, const uint32_t reg, uint32_t val);

/**
 * @brief Enable or disable the protected mode for control transactions
 *
 * @param tc6 OA TC6 specific data
 *
 * @param prote enable or disable protected control transactions
 *
 * @return 0 if operation was successful, <0 otherwise.
 */
int oa_tc6_set_protected_ctrl(struct oa_tc6 *tc6, bool prote);

/**
 * @brief Send OA TC6 data chunks to the device
 *
 * @param tc6 OA TC6 specific data
 *
 * @param pkt network packet to be send
 *
 * @return 0 if data send was successful, <0 otherwise.
 */
int oa_tc6_send_chunks(struct oa_tc6 *tc6, struct net_pkt *pkt);

/**
 * @brief Read data chunks from OA TC6 device
 *
 * @param tc6 OA TC6 specific data
 *
 * @param pkt network packet to store received data
 *
 * @return 0 if read was successful, <0 otherwise.
 */
int oa_tc6_read_chunks(struct oa_tc6 *tc6, struct net_pkt *pkt);

/**
 * @brief Perform SPI transfer of single chunk from/to OA TC6 device
 *
 * @param tc6 OA TC6 specific data
 *
 * @param buf_rx buffer to store read data
 *
 * @param buf_tx buffer with data to send
 *
 * @param hdr OA TC6 data transmission header value
 *
 * @param ftr poniter to OA TC6 data received footer
 *
 * @return 0 if transmission was successful, <0 otherwise.
 */
int oa_tc6_chunk_spi_transfer(struct oa_tc6 *tc6, uint8_t *buf_rx, uint8_t *buf_tx, uint32_t hdr,
			      uint32_t *ftr);

/**
 * @brief Read status from OA TC6 device
 *
 * @param tc6 OA TC6 specific data
 *
 * @param ftr poniter to OA TC6 data received footer
 *
 * @return 0 if successful, <0 otherwise.
 */
int oa_tc6_read_status(struct oa_tc6 *tc6, uint32_t *ftr);

/**
 * @brief Read, modify and write control register from OA TC6 device
 *
 * @param tc6 OA TC6 specific data
 *
 * @param reg register to modify
 *
 * @param mask bit mask for modified register
 *
 * @param value to be stored in the register
 *
 * @return 0 if successful, <0 otherwise.
 */
int oa_tc6_reg_rmw(struct oa_tc6 *tc6, const uint32_t reg, uint32_t mask, uint32_t val);

/**
 * @brief Check the status of OA TC6 device
 *
 * @param tc6 OA TC6 specific data
 *
 * @return 0 if successful, <0 otherwise.
 */
int oa_tc6_check_status(struct oa_tc6 *tc6);

/**
 * @brief      Read C22 registers using MDIO Bus
 *
 * This routine provides an interface to perform a C22 register read on the
 * MAC-PHY MDIO bus.
 *
 * @param[in]  tc6         Pointer to the tc6 structure for the MAC-PHY
 * @param[in]  prtad       Port address
 * @param[in]  regad       Register address
 * @param      data        Pointer to receive read data
 *
 * @return 0 if successful, <0 otherwise.
 */
int oa_tc6_mdio_read(struct oa_tc6 *tc6, uint8_t prtad, uint8_t regad, uint16_t *data);

/**
 * @brief      Write C22 registers using MDIO Bus
 *
 * This routine provides an interface to perform a C22 register write on the
 * MAC-PHY MDIO bus.
 *
 * @param[in]  tc6         Pointer to the tc6 structure for the MAC-PHY
 * @param[in]  prtad       Port address
 * @param[in]  regad       Register address
 * @param[in]  data        Write data
 *
 * @return 0 if successful, <0 otherwise.
 */
int oa_tc6_mdio_write(struct oa_tc6 *tc6, uint8_t prtad, uint8_t regad, uint16_t data);

/**
 * @brief      Read C45 registers using MDIO Bus
 *
 * This routine provides an interface to perform a C45 register read on the
 * MAC-PHY MDIO bus.
 *
 * @param[in]  tc6         Pointer to the tc6 structure for the MAC-PHY
 * @param[in]  prtad       Port address
 * @param[in]  devad       MMD device address
 * @param[in]  regad       Register address
 * @param      data        Pointer to receive read data
 *
 * @return 0 if successful, <0 otherwise.
 */
int oa_tc6_mdio_read_c45(struct oa_tc6 *tc6, uint8_t prtad, uint8_t devad, uint16_t regad,
			 uint16_t *data);

/**
 * @brief      Write C45 registers using MDIO Bus
 *
 * This routine provides an interface to perform a C45 register write on the
 * MAC-PHY MDIO bus.
 *
 * @param[in]  tc6         Pointer to the tc6 structure for the MAC-PHY
 * @param[in]  prtad       Port address
 * @param[in]  devad       MMD device address
 * @param[in]  regad       Register address
 * @param[in]  data        Write data
 *
 * @return 0 if successful, <0 otherwise.
 */
int oa_tc6_mdio_write_c45(struct oa_tc6 *tc6, uint8_t prtad, uint8_t devad, uint16_t regad,
			  uint16_t data);
#endif /* OA_TC6_CFG_H__ */
