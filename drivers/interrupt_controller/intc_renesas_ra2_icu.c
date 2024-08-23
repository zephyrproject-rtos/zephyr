/**
 * intc_renesas_ra2_icu.c
 *
 * @brief Driver for ICU of RA2 series processor.
 *
 * Copyright (c) 2022-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>
#include <errno.h>
#include <zephyr/drivers/interrupt_controller/intc_ra2_icu.h>
#include <zephyr/sw_isr_table.h>

#define DT_DRV_COMPAT renesas_ra2_icu

#define ICU_BASE_ADDR (DT_INST_REG_ADDR(0))

#define IRQCR(i) ((mm_reg_t) (ICU_BASE_ADDR + (i)))

/* The ICU kinda strange. You can't put any event on any interrupt. There are
 * actually 8 groups of IRQ (4 in each group) and events are linked to a
 * specific group (sometimes two groups).
 *
 * So even though there are 32 interrupts available in total, you might get
 * stuck if you are trying to use too many interrupts from the same group.
 *
 * Concerning the API, the user must currently select which specific event in
 * which specific group to enable, by selecting the matching#define below. For
 * instance with ra_icu_setup_event_irq(ICU_EVENT_GROUP1_RTC_ALM, ..., ...),
 * this would enable the RTC_ALM event on an IRQ from group1, i.e. on IRQs 1, 9,
 * 17 or 25 only.
 *
 * If none of the IRQs are available, ra_icu_setup_event_irq returns NULL.
 *
 * As an improvement, we could try to automate selecting the group based on the
 * event number alone, possibly with a fallback on the second group (if there is
 * one) if the first is not available for this event. But this does not really
 * solve the problem, because events could be assigned to one group or another
 * based on the initialisation order and introduce mystery assignment
 * failures for the sequential peripheral initialisations.
 */

/* ========================================================================== */
/* =====================          R_ICU         ============================= */
/* ========================================================================== */

/* =============================  IRQCR  ==================================== */
#define R_ICU_IRQCR_FLTEN_Pos      (7UL)    /*!< FLTEN (Bit 7)                */
#define R_ICU_IRQCR_FLTEN_Msk      (0x80UL) /*!< FLTEN (Bitfield-Mask: 0x01)  */
#define R_ICU_IRQCR_FCLKSEL_Pos    (4UL)    /*!< FCLKSEL (Bit 4)              */
#define R_ICU_IRQCR_FCLKSEL_Msk    (0x30UL) /*!< FCLKSEL (Bitfield-Mask: 0x03)*/
#define R_ICU_IRQCR_IRQMD_Pos      (0UL)    /*!< IRQMD (Bit 0)                */
#define R_ICU_IRQCR_IRQMD_Msk      (0x3UL)  /*!< IRQMD (Bitfield-Mask: 0x03)  */

/* =============================  NMISR  ==================================== */
#define R_ICU_NMISR    ((mm_reg_t) (ICU_BASE_ADDR + 0x140))
#define R_ICU_NMISR_SPEST_Pos      (12UL)     /*!< SPEST (Bit 12)              */
#define R_ICU_NMISR_SPEST_Msk      (0x1000UL) /*!< SPEST (Bitfield-Mask: 0x01) */
#define R_ICU_NMISR_BUSMST_Pos     (11UL)     /*!< BUSMST (Bit 11)             */
#define R_ICU_NMISR_BUSMST_Msk     (0x800UL)  /*!< BUSMST (Bitfield-Mask: 0x01)*/
#define R_ICU_NMISR_BUSSST_Pos     (10UL)     /*!< BUSSST (Bit 10)             */
#define R_ICU_NMISR_BUSSST_Msk     (0x400UL)  /*!< BUSSST (Bitfield-Mask: 0x01)*/
#define R_ICU_NMISR_RECCST_Pos     (9UL)      /*!< RECCST (Bit 9)              */
#define R_ICU_NMISR_RECCST_Msk     (0x200UL)  /*!< RECCST (Bitfield-Mask: 0x01)*/
#define R_ICU_NMISR_RPEST_Pos      (8UL)      /*!< RPEST (Bit 8)               */
#define R_ICU_NMISR_RPEST_Msk      (0x100UL)  /*!< RPEST (Bitfield-Mask: 0x01) */
#define R_ICU_NMISR_NMIST_Pos      (7UL)      /*!< NMIST (Bit 7)               */
#define R_ICU_NMISR_NMIST_Msk      (0x80UL)   /*!< NMIST (Bitfield-Mask: 0x01) */
#define R_ICU_NMISR_OSTST_Pos      (6UL)      /*!< OSTST (Bit 6)               */
#define R_ICU_NMISR_OSTST_Msk      (0x40UL)   /*!< OSTST (Bitfield-Mask: 0x01) */
#define R_ICU_NMISR_VBATTST_Pos    (4UL)      /*!< VBATTST (Bit 4)             */
#define R_ICU_NMISR_VBATTST_Msk    (0x10UL)   /*!< VBATTST (Bitfield-Mask: 0x01)*/
#define R_ICU_NMISR_LVD2ST_Pos     (3UL)      /*!< LVD2ST (Bit 3)               */
#define R_ICU_NMISR_LVD2ST_Msk     (0x8UL)    /*!< LVD2ST (Bitfield-Mask: 0x01) */
#define R_ICU_NMISR_LVD1ST_Pos     (2UL)      /*!< LVD1ST (Bit 2)               */
#define R_ICU_NMISR_LVD1ST_Msk     (0x4UL)    /*!< LVD1ST (Bitfield-Mask: 0x01) */
#define R_ICU_NMISR_WDTST_Pos      (1UL)      /*!< WDTST (Bit 1)                */
#define R_ICU_NMISR_WDTST_Msk      (0x2UL)    /*!< WDTST (Bitfield-Mask: 0x01)  */
#define R_ICU_NMISR_IWDTST_Pos     (0UL)      /*!< IWDTST (Bit 0)               */
#define R_ICU_NMISR_IWDTST_Msk     (0x1UL)    /*!< IWDTST (Bitfield-Mask: 0x01) */
#define R_ICU_NMISR_TZFST_Pos      (13UL)     /*!< TZFST (Bit 13)               */
#define R_ICU_NMISR_TZFST_Msk      (0x2000UL) /*!< TZFST (Bitfield-Mask: 0x01)  */
#define R_ICU_NMISR_CPEST_Pos      (15UL)     /*!< CPEST (Bit 15)               */
#define R_ICU_NMISR_CPEST_Msk      (0x8000UL) /*!< CPEST (Bitfield-Mask: 0x01)  */

/* ===============================  NMIER  ================================== */
#define R_ICU_NMIER    ((mm_reg_t) (ICU_BASE_ADDR + 0x120))
#define R_ICU_NMIER_SPEEN_Pos      (12UL)     /*!< SPEEN (Bit 12)               */
#define R_ICU_NMIER_SPEEN_Msk      (0x1000UL) /*!< SPEEN (Bitfield-Mask: 0x01)  */
#define R_ICU_NMIER_BUSMEN_Pos     (11UL)     /*!< BUSMEN (Bit 11)              */
#define R_ICU_NMIER_BUSMEN_Msk     (0x800UL)  /*!< BUSMEN (Bitfield-Mask: 0x01) */
#define R_ICU_NMIER_BUSSEN_Pos     (10UL)     /*!< BUSSEN (Bit 10)              */
#define R_ICU_NMIER_BUSSEN_Msk     (0x400UL)  /*!< BUSSEN (Bitfield-Mask: 0x01) */
#define R_ICU_NMIER_RECCEN_Pos     (9UL)      /*!< RECCEN (Bit 9)               */
#define R_ICU_NMIER_RECCEN_Msk     (0x200UL)  /*!< RECCEN (Bitfield-Mask: 0x01) */
#define R_ICU_NMIER_RPEEN_Pos      (8UL)      /*!< RPEEN (Bit 8)                */
#define R_ICU_NMIER_RPEEN_Msk      (0x100UL)  /*!< RPEEN (Bitfield-Mask: 0x01)  */
#define R_ICU_NMIER_NMIEN_Pos      (7UL)      /*!< NMIEN (Bit 7)                */
#define R_ICU_NMIER_NMIEN_Msk      (0x80UL)   /*!< NMIEN (Bitfield-Mask: 0x01)  */
#define R_ICU_NMIER_OSTEN_Pos      (6UL)      /*!< OSTEN (Bit 6)                */
#define R_ICU_NMIER_OSTEN_Msk      (0x40UL)   /*!< OSTEN (Bitfield-Mask: 0x01)  */
#define R_ICU_NMIER_VBATTEN_Pos    (4UL)      /*!< VBATTEN (Bit 4)              */
#define R_ICU_NMIER_VBATTEN_Msk    (0x10UL)   /*!< VBATTEN (Bitfield-Mask: 0x01)*/
#define R_ICU_NMIER_LVD2EN_Pos     (3UL)      /*!< LVD2EN (Bit 3)               */
#define R_ICU_NMIER_LVD2EN_Msk     (0x8UL)    /*!< LVD2EN (Bitfield-Mask: 0x01) */
#define R_ICU_NMIER_LVD1EN_Pos     (2UL)      /*!< LVD1EN (Bit 2)               */
#define R_ICU_NMIER_LVD1EN_Msk     (0x4UL)    /*!< LVD1EN (Bitfield-Mask: 0x01) */
#define R_ICU_NMIER_WDTEN_Pos      (1UL)      /*!< WDTEN (Bit 1)                */
#define R_ICU_NMIER_WDTEN_Msk      (0x2UL)    /*!< WDTEN (Bitfield-Mask: 0x01)  */
#define R_ICU_NMIER_IWDTEN_Pos     (0UL)      /*!< IWDTEN (Bit 0)               */
#define R_ICU_NMIER_IWDTEN_Msk     (0x1UL)    /*!< IWDTEN (Bitfield-Mask: 0x01) */
#define R_ICU_NMIER_TZFEN_Pos      (13UL)     /*!< TZFEN (Bit 13)               */
#define R_ICU_NMIER_TZFEN_Msk      (0x2000UL) /*!< TZFEN (Bitfield-Mask: 0x01)  */
#define R_ICU_NMIER_CPEEN_Pos      (15UL)     /*!< CPEEN (Bit 15)               */
#define R_ICU_NMIER_CPEEN_Msk      (0x8000UL) /*!< CPEEN (Bitfield-Mask: 0x01)  */

/* ===============================  NMICLR  ================================= */
#define R_ICU_NMICLR   ((mm_reg_t)(ICU_BASE_ADDR + 0x130))
#define R_ICU_NMICLR_SPECLR_Pos    (12UL)     /*!< SPECLR (Bit 12)               */
#define R_ICU_NMICLR_SPECLR_Msk    (0x1000UL) /*!< SPECLR (Bitfield-Mask: 0x01)  */
#define R_ICU_NMICLR_BUSMCLR_Pos   (11UL)     /*!< BUSMCLR (Bit 11)              */
#define R_ICU_NMICLR_BUSMCLR_Msk   (0x800UL)  /*!< BUSMCLR (Bitfield-Mask: 0x01) */
#define R_ICU_NMICLR_BUSSCLR_Pos   (10UL)     /*!< BUSSCLR (Bit 10)              */
#define R_ICU_NMICLR_BUSSCLR_Msk   (0x400UL)  /*!< BUSSCLR (Bitfield-Mask: 0x01) */
#define R_ICU_NMICLR_RECCCLR_Pos   (9UL)      /*!< RECCCLR (Bit 9)               */
#define R_ICU_NMICLR_RECCCLR_Msk   (0x200UL)  /*!< RECCCLR (Bitfield-Mask: 0x01) */
#define R_ICU_NMICLR_RPECLR_Pos    (8UL)      /*!< RPECLR (Bit 8)                */
#define R_ICU_NMICLR_RPECLR_Msk    (0x100UL)  /*!< RPECLR (Bitfield-Mask: 0x01)  */
#define R_ICU_NMICLR_NMICLR_Pos    (7UL)      /*!< NMICLR (Bit 7)                */
#define R_ICU_NMICLR_NMICLR_Msk    (0x80UL)   /*!< NMICLR (Bitfield-Mask: 0x01)  */
#define R_ICU_NMICLR_OSTCLR_Pos    (6UL)      /*!< OSTCLR (Bit 6)                */
#define R_ICU_NMICLR_OSTCLR_Msk    (0x40UL)   /*!< OSTCLR (Bitfield-Mask: 0x01)  */
#define R_ICU_NMICLR_VBATTCLR_Pos  (4UL)      /*!< VBATTCLR (Bit 4)              */
#define R_ICU_NMICLR_VBATTCLR_Msk  (0x10UL)   /*!< VBATTCLR (Bitfield-Mask: 0x01)*/
#define R_ICU_NMICLR_LVD2CLR_Pos   (3UL)      /*!< LVD2CLR (Bit 3)               */
#define R_ICU_NMICLR_LVD2CLR_Msk   (0x8UL)    /*!< LVD2CLR (Bitfield-Mask: 0x01) */
#define R_ICU_NMICLR_LVD1CLR_Pos   (2UL)      /*!< LVD1CLR (Bit 2)               */
#define R_ICU_NMICLR_LVD1CLR_Msk   (0x4UL)    /*!< LVD1CLR (Bitfield-Mask: 0x01) */
#define R_ICU_NMICLR_WDTCLR_Pos    (1UL)      /*!< WDTCLR (Bit 1)                */
#define R_ICU_NMICLR_WDTCLR_Msk    (0x2UL)    /*!< WDTCLR (Bitfield-Mask: 0x01)  */
#define R_ICU_NMICLR_IWDTCLR_Pos   (0UL)      /*!< IWDTCLR (Bit 0)               */
#define R_ICU_NMICLR_IWDTCLR_Msk   (0x1UL)    /*!< IWDTCLR (Bitfield-Mask: 0x01) */
#define R_ICU_NMICLR_TZFCLR_Pos    (13UL)     /*!< TZFCLR (Bit 13)               */
#define R_ICU_NMICLR_TZFCLR_Msk    (0x2000UL) /*!< TZFCLR (Bitfield-Mask: 0x01)  */
#define R_ICU_NMICLR_CPECLR_Pos    (15UL)     /*!< CPECLR (Bit 15)               */
#define R_ICU_NMICLR_CPECLR_Msk    (0x8000UL) /*!< CPECLR (Bitfield-Mask: 0x01)  */

/* ===============================  NMICR  ================================== */
#define R_ICU_NMICR    ((mm_reg_t)(ICU_BASE_ADDR + 0x100))
#define R_ICU_NMICR_NFLTEN_Pos     (7UL)    /*!< NFLTEN (Bit 7)                */
#define R_ICU_NMICR_NFLTEN_Msk     (0x80UL) /*!< NFLTEN (Bitfield-Mask: 0x01)  */
#define R_ICU_NMICR_NFCLKSEL_Pos   (4UL)    /*!< NFCLKSEL (Bit 4)              */
#define R_ICU_NMICR_NFCLKSEL_Msk   (0x30UL) /*!< NFCLKSEL (Bitfield-Mask: 0x03)*/
#define R_ICU_NMICR_NMIMD_Pos      (0UL)    /*!< NMIMD (Bit 0)                 */
#define R_ICU_NMICR_NMIMD_Msk      (0x1UL)  /*!< NMIMD (Bitfield-Mask: 0x01)   */

/* ===============================  IELSR  ================================== */
#define R_ICU_IELSR(n)	((mm_reg_t)(ICU_BASE_ADDR + 0x300 + 4*(n)))
#define R_ICU_IELSR_DTCE_Pos   (24UL)         /*!< DTCE (Bit 24)              */
#define R_ICU_IELSR_DTCE_Msk  GENMASK(24, 24) /*!< DTCE (Bitfield-Mask: 0x01) */
#define R_ICU_IELSR_IR_Pos     (16UL)         /*!< IR (Bit 16)                */
#define R_ICU_IELSR_IR_Msk    GENMASK(16, 16) /*!< IR (Bitfield-Mask: 0x01)   */
#define R_ICU_IELSR_IELS_Pos   (0UL)          /*!< IELS (Bit 0)               */
#define R_ICU_IELSR_IELS_Msk  GENMASK(4, 0)   /*!< IELS (Bitfield-Mask: 0x1ff)*/
#define R_ICU_IELSR_IELS(x)	\
	(((x)<<R_ICU_IELSR_IELS_Pos) & R_ICU_IELSR_IELS_Msk)

/* ===============================  DELSR  ================================== */
#define R_ICU_DELSR_IR_Pos        (16UL)      /*!< IR (Bit 16)                */
#define R_ICU_DELSR_IR_Msk        (0x10000UL) /*!< IR (Bitfield-Mask: 0x01)   */
#define R_ICU_DELSR_DELS_Pos      (0UL)       /*!< DELS (Bit 0)               */
#define R_ICU_DELSR_DELS_Msk      (0x1ffUL)   /*!< DELS (Bitfield-Mask: 0x1ff)*/

/* ==============================  SELSR0  ================================== */
#define R_ICU_SELSR0    ((mm_reg_t) (ICU_BASE_ADDR + 0x200))
#define R_ICU_SELSR0_SELS_Pos     (0UL)      /*!< SELS (Bit 0)                */
#define R_ICU_SELSR0_SELS_Msk     (0x1ffUL)  /*!< SELS (Bitfield-Mask: 0x1ff) */

/* ===============================  WUPEN  ================================== */
#define R_ICU_WUPEN    ((mm_reg_t)(ICU_BASE_ADDR + 0x1A0))
#define R_ICU_WUPEN_IIC0WUPEN_Pos       (31UL)        /*!< IIC0WUPEN (Bit 31) */
#define R_ICU_WUPEN_IIC0WUPEN_Msk       \
	BIT(R_ICU_WUPEN_IIC0WUPEN_Pos) /*!< IIC0WUPEN (Bitfield-Mask: 0x01)  */
#define R_ICU_WUPEN_AGT1CBWUPEN_Pos     (30UL)     /*!< AGT1CBWUPEN (Bit 30) */
#define R_ICU_WUPEN_AGT1CBWUPEN_Msk     \
	BIT(R_ICU_WUPEN_AGT1CBWUPEN_Pos) /*!< AGT1CBWUPEN (Bitfield-Mask: 0x01) */
#define R_ICU_WUPEN_AGT1CAWUPEN_Pos     (29UL)        /*!< AGT1CAWUPEN (Bit 29) */
#define R_ICU_WUPEN_AGT1CAWUPEN_Msk     \
	BIT(R_ICU_WUPEN_AGT1CAWUPEN_Pos) /*!< AGT1CAWUPEN (Bitfield-Mask: 0x01) */
#define R_ICU_WUPEN_AGT1UDWUPEN_Pos     (28UL)        /*!< AGT1UDWUPEN (Bit 28) */
#define R_ICU_WUPEN_AGT1UDWUPEN_Msk     \
	BIT(R_ICU_WUPEN_AGT1UDWUPEN_Pos) /*!< AGT1UDWUPEN (Bitfield-Mask: 0x01) */
#define R_ICU_WUPEN_RTCPRDWUPEN_Pos     (25UL)         /*!< RTCPRDWUPEN (Bit 25) */
#define R_ICU_WUPEN_RTCPRDWUPEN_Msk     \
	BIT(R_ICU_WUPEN_RTCPRDWUPEN_Pos)  /*!< RTCPRDWUPEN (Bitfield-Mask: 0x01) */
#define R_ICU_WUPEN_RTCALMWUPEN_Pos     (24UL)         /*!< RTCALMWUPEN (Bit 24) */
#define R_ICU_WUPEN_RTCALMWUPEN_Msk     \
	BIT(R_ICU_WUPEN_RTCALMWUPEN_Pos)  /*!< RTCALMWUPEN (Bitfield-Mask: 0x01) */
#define R_ICU_WUPEN_ACMPLP0WUPEN_Pos    (23UL)     /*!< ACMPLP0WUPEN (Bit 23) */
#define R_ICU_WUPEN_ACMPLP0WUPEN_Msk    \
	BIT(R_ICU_WUPEN_ACMPLP0WUPEN_Pos)  /*!< ACMPLP0WUPEN (Bitfield-Mask: 0x01) */
#define R_ICU_WUPEN_LVD2WUPEN_Pos       (19UL)       /*!< LVD2WUPEN (Bit 19)  */
#define R_ICU_WUPEN_LVD2WUPEN_Msk       \
	BIT(R_ICU_WUPEN_LVD2WUPEN_Pos)    /*!< LVD2WUPEN (Bitfield-Mask: 0x01) */
#define R_ICU_WUPEN_LVD1WUPEN_Pos       (18UL)         /*!< LVD1WUPEN (Bit 18) */
#define R_ICU_WUPEN_LVD1WUPEN_Msk       \
	BIT(R_ICU_WUPEN_LVD1WUPEN_Pos)    /*!< LVD1WUPEN (Bitfield-Mask: 0x01) */
#define R_ICU_WUPEN_KEYWUPEN_Pos        (17UL)         /*!< KEYWUPEN (Bit 17)  */
#define R_ICU_WUPEN_KEYWUPEN_Msk        \
	BIT(R_ICU_WUPEN_KEYWUPEN_Pos)    /*!< KEYWUPEN (Bitfield-Mask: 0x01)  */
#define R_ICU_WUPEN_IWDTWUPEN_Pos       (16UL)        /*!< IWDTWUPEN (Bit 16) */
#define R_ICU_WUPEN_IWDTWUPEN_Msk       \
	BIT(R_ICU_WUPEN_IWDTWUPEN_Pos)   /*!< IWDTWUPEN (Bitfield-Mask: 0x01) */
#define R_ICU_WUPEN_IRQWUPEN_Pos        (0UL)       /*!< IRQWUPEN (Bit 0)     */
#define R_ICU_WUPEN_IRQWUPEN_Msk        \
	GENMASK(7, 0) /*!< IRQWUPEN (Bitfield-Mask: 0x01)  */

/* ===============================  IELEN  ================================== */
#define R_ICU_IELEN	((mm_reg_t)(ICU_BASE_ADDR + 0x1C0))
#define R_ICU_IELEN_IELEN_Pos     (1UL)   /*!< IELEN (Bit 1)                  */
#define R_ICU_IELEN_IELEN_Msk     (0x2UL) /*!< IELEN (Bitfield-Mask: 0x01)    */
#define R_ICU_IELEN_RTCINTEN_Pos  (0UL)   /*!< RTCINTEN (Bit 0)               */
#define R_ICU_IELEN_RTCINTEN_Msk  (0x1UL) /*!< RTCINTEN (Bitfield-Mask: 0x01) */

struct icu_event {
	event_cb_t callback;
	void *callback_data;
};

struct icu_data {
	struct icu_event entries[CONFIG_NUM_IRQS];
	uint8_t iels_map[CONFIG_NUM_IRQS];
};

/* Instantiation of this driver is done once (with DEVICE_DT_INST_DEFINE)
 * so there is no need to index this by instance
 */
static struct icu_data ra_icu_data;
static struct k_spinlock lock;

static inline
int get_free_irq(unsigned int group)
{
	unsigned int i;

	for (i = group; i < ARRAY_SIZE(ra_icu_data.iels_map); i += 8) {
		if (!ra_icu_data.iels_map[i]) {
			return i;
		}
	}
	return -ENXIO;
}

static int setup_event_irq(int irq, int iels_num)
{
	if (iels_num < CONFIG_NUM_IRQS) {
		uint32_t ieslr =
			sys_read32(R_ICU_IELSR(irq)) & ~R_ICU_IELSR_IELS_Msk;
		sys_write32(ieslr | R_ICU_IELSR_IELS(iels_num),
				R_ICU_IELSR(irq));
		return 0;
	}
	return -EINVAL;
}

void ra_icu_set_callback(struct icu_event *event, event_cb_t callback,
							void *callback_data)
{
	event->callback = callback;
	event->callback_data = callback_data;
}

void ra_icu_release_event_irq(struct icu_event *evt)
{
	K_SPINLOCK(&lock) {
		ra_icu_set_callback(evt, NULL, NULL);
	}
}

struct icu_event *ra_icu_setup_event_irq(int event_number_grouped,
					 event_cb_t callback,
					 void *callback_data)
{
	struct icu_event *event = NULL;

	K_SPINLOCK(&lock) {
		int irq_to_use = get_free_irq(event_number_grouped >> 5);

		if (irq_to_use >= 0) {
			event = &ra_icu_data.entries[irq_to_use];
			ra_icu_set_callback(event, callback, callback_data);
			ra_icu_data.iels_map[irq_to_use] =
				event_number_grouped & R_ICU_IELSR_IELS_Msk;
		}
	}

	return event;
}

int ra_icu_enable_event(struct icu_event *event)
{
	int irq = event - &ra_icu_data.entries[0];

	K_SPINLOCK(&lock) {
		setup_event_irq(irq, ra_icu_data.iels_map[irq]);
		irq_enable(irq);
	}
	return 0;
}

int ra_icu_disable_event(struct icu_event *event)
{
	int irq = event - &ra_icu_data.entries[0];

	K_SPINLOCK(&lock) {
		irq_disable(irq);
		setup_event_irq(irq, 0);
	}
	return 0;
}

int ra_icu_set_dtc_flag(struct icu_event *event, bool dtc)
{
	if (event) {
		int irq = event - &ra_icu_data.entries[0];

		K_SPINLOCK(&lock) {
			uint32_t ieslr = sys_read32(R_ICU_IELSR(irq));

			if (dtc) {
				ieslr |= R_ICU_IELSR_DTCE_Msk;
			} else {
				ieslr &= ~R_ICU_IELSR_DTCE_Msk;

			}
			sys_write32(ieslr, R_ICU_IELSR(irq));
		}
		return 0;
	}
	return -EINVAL;
}

int ra_icu_get_event_irq_num(struct icu_event *event)
{
	if (event) {
		return event - &ra_icu_data.entries[0];
	}

	return -EINVAL;
}

int ra_icu_shutdown_event_irq(struct icu_event *event)
{
	if (event) {
		int irq = event - &ra_icu_data.entries[0];

		K_SPINLOCK(&lock) {
			ra_icu_disable_event(event);
			ra_icu_data.iels_map[irq] = 0;
			ra_icu_set_callback(event, NULL, NULL);
		}

		return 0;
	}
	return -EINVAL;
}

int ra_set_irq_cfg(unsigned int irq, irq_ra_sense_t sense,
		nmi_irq_ra_division_t divisor, int filtered)
{
	uint8_t reg = 0;

	if (irq > 7)
		return -EINVAL;

	reg |= (filtered ? 1 : 0) << 7;
	reg |= (divisor & 0x3) << 4;
	reg |= (sense & 0x3) << 0;

	sys_write8(reg, IRQCR(irq));

	return 0;
}

int ra_get_irq_cfg(unsigned int irq, irq_ra_sense_t *sense,
		nmi_irq_ra_division_t *divisor, int *filtered)
{
	uint8_t reg;

	if (irq > 7)
		return -1;

	reg = sys_read8(IRQCR(irq));

	*filtered = reg >> 7;
	*divisor = (reg >> 4) & 0x3;
	*sense = (reg >> 0) & 0x3;

	return 0;
}

void ra_set_nmi_cfg(nmi_ra_sense_t sense, nmi_irq_ra_division_t divisor,
		int filtered)
{
	uint8_t reg;

	reg  = 0;
	reg |= (filtered ? 1 : 0) << 7;
	reg |= (divisor & 0x3) << 4;
	reg |= sense & 0x1;

	sys_write8(reg, R_ICU_NMICR);
}

void ra_get_nmi_cfg(nmi_ra_sense_t *sense, nmi_irq_ra_division_t *divisor,
		int *filtered)
{
	uint8_t reg = sys_read8(R_ICU_NMICR);

	*filtered = (reg >> 7) & 0x1;
	*divisor = (reg >> 4) & 0x3;
	*sense = (reg >> 0) & 0x1;
}

int ra_activate_wakeup_sources(uint32_t mask)
{
	sys_write32(mask | sys_read32(R_ICU_WUPEN), R_ICU_WUPEN);
	return 0;
}

int ra_deactivate_wakeup_sources(uint32_t mask)
{
	sys_write32(~mask & sys_read32(R_ICU_WUPEN), R_ICU_WUPEN);
	return 0;
}

uint32_t ra_get_active_wakeup(void)
{
	return sys_read32(R_ICU_WUPEN);
}

int ra_activate_nmi_sources(uint16_t mask)
{
	sys_write16(mask | sys_read16(R_ICU_NMIER), R_ICU_NMIER);
	return 0;
}

uint16_t ra_get_active_nmi(void)
{
	return sys_read16(R_ICU_NMIER);
}

void ra_clear_nmi(uint16_t mask)
{
	sys_write16(mask, R_ICU_NMIER);
}

static void icu_isr(void *arg)
{
	struct icu_event *event = (struct icu_event *)arg;

	event->callback(event, event->callback_data);
}

int ra_icu_set_priority(struct icu_event *event, unsigned int priority)
{
	if (event) {
		int irq = event - &ra_icu_data.entries[0];

		z_arm_irq_priority_set(irq, priority, 0);
		return 0;
	}
	return -EINVAL;
}

static int ra_icu_init(const struct device *dev)
{
	/* Start by activating default NMIs */
	ra_activate_nmi_sources(NMI_RECCEN | NMI_SPEEN);

	/* Don't use a loop, these are partially compile-time macros */
	/* FIXME Priorities */
	IRQ_CONNECT(1, 3, icu_isr, &ra_icu_data.entries[1], 0);
	IRQ_CONNECT(2, 3, icu_isr, &ra_icu_data.entries[2], 0);
	IRQ_CONNECT(3, 3, icu_isr, &ra_icu_data.entries[3], 0);
	IRQ_CONNECT(4, 3, icu_isr, &ra_icu_data.entries[4], 0);
	IRQ_CONNECT(5, 3, icu_isr, &ra_icu_data.entries[5], 0);
	IRQ_CONNECT(6, 3, icu_isr, &ra_icu_data.entries[6], 0);
	IRQ_CONNECT(7, 3, icu_isr, &ra_icu_data.entries[7], 0);
	IRQ_CONNECT(8, 3, icu_isr, &ra_icu_data.entries[8], 0);
	IRQ_CONNECT(9, 3, icu_isr, &ra_icu_data.entries[9], 0);
	IRQ_CONNECT(10, 3, icu_isr, &ra_icu_data.entries[10], 0);
	IRQ_CONNECT(11, 3, icu_isr, &ra_icu_data.entries[11], 0);
	IRQ_CONNECT(12, 3, icu_isr, &ra_icu_data.entries[12], 0);
	IRQ_CONNECT(13, 3, icu_isr, &ra_icu_data.entries[13], 0);
	IRQ_CONNECT(14, 3, icu_isr, &ra_icu_data.entries[14], 0);
	IRQ_CONNECT(15, 3, icu_isr, &ra_icu_data.entries[15], 0);
	IRQ_CONNECT(16, 3, icu_isr, &ra_icu_data.entries[16], 0);
	IRQ_CONNECT(17, 3, icu_isr, &ra_icu_data.entries[17], 0);
	IRQ_CONNECT(18, 3, icu_isr, &ra_icu_data.entries[18], 0);
	IRQ_CONNECT(19, 3, icu_isr, &ra_icu_data.entries[19], 0);
	IRQ_CONNECT(20, 3, icu_isr, &ra_icu_data.entries[20], 0);
	IRQ_CONNECT(21, 3, icu_isr, &ra_icu_data.entries[21], 0);
	IRQ_CONNECT(22, 3, icu_isr, &ra_icu_data.entries[22], 0);
	IRQ_CONNECT(23, 3, icu_isr, &ra_icu_data.entries[23], 0);
	IRQ_CONNECT(24, 3, icu_isr, &ra_icu_data.entries[24], 0);
	IRQ_CONNECT(25, 3, icu_isr, &ra_icu_data.entries[25], 0);
	IRQ_CONNECT(26, 3, icu_isr, &ra_icu_data.entries[26], 0);
	IRQ_CONNECT(27, 3, icu_isr, &ra_icu_data.entries[27], 0);
	IRQ_CONNECT(28, 3, icu_isr, &ra_icu_data.entries[28], 0);
	IRQ_CONNECT(29, 3, icu_isr, &ra_icu_data.entries[29], 0);
	IRQ_CONNECT(30, 3, icu_isr, &ra_icu_data.entries[30], 0);
	IRQ_CONNECT(31, 3, icu_isr, &ra_icu_data.entries[31], 0);
	return 0;
}

void ra_icu_clear_event(struct icu_event *event)
{
	if (event) {
		int irq = event - &ra_icu_data.entries[0];

		K_SPINLOCK(&lock) {
			uint32_t ieslr = sys_read32(R_ICU_IELSR(irq));

			sys_write32(ieslr & ~R_ICU_IELSR_IR_Msk,
					R_ICU_IELSR(irq));
		}
	}
}

DEVICE_DT_INST_DEFINE(0,
		&ra_icu_init,
		NULL,
		&ra_icu_data,
		NULL,
		PRE_KERNEL_1,
		CONFIG_INTC_INIT_PRIORITY,
		NULL);
