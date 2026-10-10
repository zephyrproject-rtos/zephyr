/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/drivers/mfd/bflb-system-control.h>

#include <bflb_soc.h>
#include <aon_reg.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#include <pds_reg.h>

/*
 * Most BFLB SoCs (BL602, BL702, BL702L, BL616) store a factory-programmed
 * MAC address in efuse at offset 0x14 (low 32 bits) and 0x18. The MAC is stored
 * in little-endian order and must be byte-swapped to network (big-endian)
 * order so the OUI occupies the first three bytes.
 */
#if defined(CONFIG_SOC_SERIES_BL616CL)
#define EFUSE_WIFI_MAC_LOW_OFFSET	0x04
#define EFUSE_WIFI_MAC_HIGH_OFFSET	0x08
#else
#define EFUSE_WIFI_MAC_LOW_OFFSET	0x14
#define EFUSE_WIFI_MAC_HIGH_OFFSET	0x18
#endif

/* BL70x/L parity slot */
#define EFUSE_DATA_0_EF_KEY_SLOT_5_W2	0x74

#define DEVICE_ID_LENGTH		6
#define DEVICE_ID_LENGTH_EFUSE		8

#define EFUSE_WIFI_MAC_PARITY_MSK	0x3f
#define EFUSE_WIFI_MAC_PARITY_POS	16

/* The reset tracking on BFLB is cumulative and not cleared automatically */
#define RECORDER_RESET_ANALOG		BIT(0)
#define RECORDER_RESET_EXTERNAL		BIT(1)
#define RECORDER_RESET_PDS		BIT(2)
#define RECORDER_RESET_WDT		BIT(3)
#define RECORDER_RESET_SOFT_CPU		BIT(4)
#define RECORDER_RESET_SOFT_SYS		BIT(5)
#define RECORDER_RESET_SOFT_CPUSYS	BIT(6)

#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL70XL)
#define HBN_EVENT_POR			BIT(0)
#define HBN_EVENT_EXTRST		BIT(1)
#define HBN_EVENT_SWRST			BIT(2)
#define HBN_EVENT_PWRRST		BIT(3)
#define HBN_EVENT_BOR			BIT(4)
#else
#define HBN_EVENT_WDT			BIT(0)
#define HBN_EVENT_POR			BIT(1)
#define HBN_EVENT_SWRST			BIT(2)
#define HBN_EVENT_PWRRST		BIT(3)
#define HBN_EVENT_BOR			BIT(4)
#endif

#define PDS_EVENT_BUSRST		BIT(0)
#define PDS_EVENT_HBNPWRRST		BIT(1)
#define PDS_EVENT_PDSRST		BIT(2)

/* PU_CHIP pulled down */
#define RESET_PU_CHIP_RECORDER		(RECORDER_RESET_ANALOG | RECORDER_RESET_PDS		\
	| RECORDER_RESET_WDT | RECORDER_RESET_SOFT_CPU | RECORDER_RESET_SOFT_SYS		\
	| RECORDER_RESET_SOFT_CPUSYS)
#define RESET_PU_CHIP_HBN		(HBN_EVENT_POR | HBN_EVENT_PWRRST)
#define RESET_PU_CHIP_PDS		(PDS_EVENT_BUSRST | PDS_EVENT_HBNPWRRST | PDS_EVENT_PDSRST)

#define RESET_SW_HBN			0
#define RESET_SW_PDS			(PDS_EVENT_BUSRST)

/* Warm software reset */
#define RESET_SW_WARM_RECORDER		(RECORDER_RESET_SOFT_SYS | RECORDER_RESET_SOFT_CPU)

/* Cold software reset */
#define RESET_SW_COLD_RECORDER		0

/* Missing on BL61x */
#ifndef HBN_CLR_RESET_EVENT_MSK
#define HBN_CLR_RESET_EVENT_MSK		(1U << 13U)
#endif

/* We store and clear the software reboot flag as soon as possible */
static bool marked_sw_reboot;
/* This one indicates this is not the first power up for hwinfo */
static bool marked_npor;

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	const struct device *efuse = DEVICE_DT_GET(DT_NODELABEL(efuse));
	uint32_t mac_low, mac_high;
	uint8_t id[DEVICE_ID_LENGTH_EFUSE];
	uint8_t parity_cl, parity_ef;
	int err;

	if (!device_is_ready(efuse)) {
		return -ENODEV;
	}

	err = otp_read(efuse, EFUSE_WIFI_MAC_LOW_OFFSET, &mac_low, sizeof(uint32_t));
	if (err != 0) {
		return err;
	}

	err = otp_read(efuse, EFUSE_WIFI_MAC_HIGH_OFFSET, &mac_high, sizeof(uint32_t));
	if (err != 0) {
		return err;
	}

	/* Copy and convert from efuse little-endian to network byte order */
#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL70XL)
	/* Keep whole length: 0s are counted */
	sys_put_be32(mac_high, &id[0]);
	sys_put_be32(mac_low, &id[4]);
#else
	sys_put_be16((uint16_t)mac_high, &id[0]);
	sys_put_be32(mac_low, &id[2]);
#endif

	/* Check parity which uses 0 count */
#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL70XL)
	parity_cl =  (DEVICE_ID_LENGTH_EFUSE * BITS_PER_BYTE)
		- (sys_count_bits(id, DEVICE_ID_LENGTH_EFUSE) & EFUSE_WIFI_MAC_PARITY_MSK);
	/* BL70x/L stores parity elsewhere */
	err = otp_read(efuse, EFUSE_DATA_0_EF_KEY_SLOT_5_W2, &mac_high, sizeof(uint32_t));
	if (err != 0) {
		return err;
	}
	parity_ef = mac_high & EFUSE_WIFI_MAC_PARITY_MSK;
	/* Keep only the bits we want after parity check */
	sys_put_be16((uint16_t)(mac_low >> 16), &id[4]);
#else
	parity_cl =  (DEVICE_ID_LENGTH * BITS_PER_BYTE)
		- (sys_count_bits(id, DEVICE_ID_LENGTH) & EFUSE_WIFI_MAC_PARITY_MSK);
	parity_ef = (mac_high >> EFUSE_WIFI_MAC_PARITY_POS) & EFUSE_WIFI_MAC_PARITY_MSK;
#endif

	if (parity_cl != parity_ef) {
		return -EIO;
	}

	length = MIN(length, DEVICE_ID_LENGTH);
	memcpy(buffer, id, length);

	return length;
}

static int retained_reset_init_func(void)
{
	const struct device *aon_scratch0_dev = DEVICE_DT_GET(DT_NODELABEL(aon_scratch0));
	uint32_t aon_scratch0 = 0;

	(void)retained_mem_read(aon_scratch0_dev, 0, (uint8_t *)&aon_scratch0, sizeof(uint32_t));

	if ((aon_scratch0 & AON_SCRATCH0_FLAG_SW_REBOOT) != 0) {
		marked_sw_reboot = true;
	}

	if ((aon_scratch0 & AON_SCRATCH0_FLAG_NPOR) != 0) {
		marked_npor = true;
	}

	aon_scratch0 |= AON_SCRATCH0_FLAG_NPOR;
	aon_scratch0 &= ~AON_SCRATCH0_FLAG_SW_REBOOT;
	(void)retained_mem_write(aon_scratch0_dev, 0, (uint8_t *)&aon_scratch0, sizeof(uint32_t));

	return 0;
}

#if defined(CONFIG_SOC_SERIES_BL808)

static uint32_t bflb_get_reset_reason(void)
{
	uint32_t reason = 0;
	uint32_t reason_recorder = sys_read32(GLB_BASE + GLB_RESET_STS0_OFFSET)
		& GLB_TOP_RESET_RECORDER_MSK;
	uint32_t reason_pds = (sys_read32(PDS_BASE + PDS_STAT_OFFSET) & PDS_RESET_EVENT_MSK)
		>> PDS_RESET_EVENT_POS;
	uint32_t reason_hbn = (sys_read32(HBN_BASE + HBN_GLB_OFFSET) & HBN_RESET_EVENT_MSK)
		>> HBN_RESET_EVENT_POS;

	if ((reason_hbn & HBN_EVENT_POR) != 0) {
		reason |= RESET_POR;
	}

	if (reason_recorder != RESET_PU_CHIP_RECORDER) {
		if ((reason_hbn & HBN_EVENT_BOR) != 0) {
			reason |= RESET_BROWNOUT;
		}

		if ((reason_recorder & RECORDER_RESET_WDT) != 0) {
			reason |= RESET_WATCHDOG;
		}
	}

	if (reason_recorder == RESET_SW_WARM_RECORDER) {
		reason |= RESET_SOFTWARE;
	} else if (reason_recorder == RESET_SW_COLD_RECORDER) {
		reason |= RESET_SOFTWARE;
		reason |= RESET_POR;
	}

	return reason;
}

#endif

#if defined(CONFIG_SOC_SERIES_BL616CL) || defined(CONFIG_SOC_SERIES_BL61X)

static uint32_t bflb_get_reset_reason(void)
{
	uint32_t reason = 0;
	uint32_t reason_recorder = sys_read32(GLB_BASE + GLB_RESET_STS0_OFFSET)
		& GLB_TOP_RESET_RECORDER_MSK;
	uint32_t reason_pds = (sys_read32(PDS_BASE + PDS_STAT_OFFSET) & PDS_RESET_EVENT_MSK)
		>> PDS_RESET_EVENT_POS;
	uint32_t reason_hbn = (sys_read32(HBN_BASE + HBN_GLB_OFFSET) & HBN_RESET_EVENT_MSK)
		>> HBN_RESET_EVENT_POS;

	if ((reason_hbn & HBN_EVENT_POR) != 0) {
		reason |= RESET_POR;
	}

	if (reason_recorder != RESET_PU_CHIP_RECORDER) {
		if ((reason_hbn & HBN_EVENT_BOR) != 0) {
			reason |= RESET_BROWNOUT;
		}

		if ((reason_recorder & RECORDER_RESET_WDT) != 0) {
			reason |= RESET_WATCHDOG;
		}
	}

	/* Not a HBN reset (main memory content survivable) */
	if (reason_hbn == RESET_SW_HBN && reason_pds == RESET_SW_PDS) {
		reason |= RESET_SOFTWARE;
		if (reason_recorder == RESET_SW_COLD_RECORDER) {
			reason |= RESET_POR;
		}
		if ((reason_recorder & RECORDER_RESET_PDS) != 0) {
			reason |= RESET_LOW_POWER_WAKE;
		}
	}

	return reason;
}

#endif

#if defined(CONFIG_SOC_SERIES_BL616CL) || defined(CONFIG_SOC_SERIES_BL61X) \
	|| defined(CONFIG_SOC_SERIES_BL808)

static void bflb_clear_reset_reason(void)
{
	uint32_t tmp;

	tmp = sys_read32(GLB_BASE + GLB_RESET_STS0_OFFSET);
	tmp &= GLB_CLR_TOP_RESET_RECORDER_UMSK;
	sys_write32(tmp, GLB_BASE + GLB_RESET_STS0_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_RESET_STS0_OFFSET);
	tmp |= GLB_CLR_TOP_RESET_RECORDER_MSK;
	sys_write32(tmp, GLB_BASE + GLB_RESET_STS0_OFFSET);

	tmp = sys_read32(GLB_BASE + GLB_RESET_STS0_OFFSET);
	tmp &= GLB_CLR_TOP_RESET_RECORDER_UMSK;
	sys_write32(tmp, GLB_BASE + GLB_RESET_STS0_OFFSET);

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= ~HBN_CLR_RESET_EVENT_MSK;
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp |= HBN_CLR_RESET_EVENT_MSK;
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= ~HBN_CLR_RESET_EVENT_MSK;
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_STAT_OFFSET);
	tmp &= PDS_CLR_RESET_EVENT_UMSK;
	sys_write32(tmp, PDS_BASE + PDS_STAT_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_STAT_OFFSET);
	tmp |= PDS_CLR_RESET_EVENT_MSK;
	sys_write32(tmp, PDS_BASE + PDS_STAT_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_STAT_OFFSET);
	tmp &= PDS_CLR_RESET_EVENT_UMSK;
	sys_write32(tmp, PDS_BASE + PDS_STAT_OFFSET);
}

#endif

#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL70XL)

static uint32_t bflb_get_reset_reason(void)
{
	uint32_t reason = 0;
	uint32_t reason_hbn = (sys_read32(HBN_BASE + HBN_GLB_OFFSET) & HBN_RESET_EVENT_MSK)
		>> HBN_RESET_EVENT_POS;
	uint32_t reason_pds = (sys_read32(PDS_BASE + PDS_INT_OFFSET) & PDS_RESET_EVENT_MSK)
		>> PDS_RESET_EVENT_POS;

	if ((reason_hbn & HBN_EVENT_POR) != 0) {
		reason |= RESET_POR;
	}

	if ((reason_hbn & HBN_EVENT_BOR) != 0) {
		reason |= RESET_BROWNOUT;
	}

	if (reason_hbn == RESET_SW_HBN && reason_pds == RESET_SW_PDS) {
		if (marked_sw_reboot) {
			reason |= RESET_SOFTWARE;
			reason |= RESET_POR;
		} else {
			reason |= RESET_WATCHDOG;
		}
	}

	return reason;
}

static void bflb_clear_reset_reason(void)
{
	uint32_t tmp;

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= HBN_CLEAR_RESET_EVENT_UMSK;
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp |= HBN_CLEAR_RESET_EVENT_MSK;
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp &= HBN_CLEAR_RESET_EVENT_UMSK;
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_INT_OFFSET);
	tmp &= PDS_CLR_RESET_EVENT_UMSK;
	sys_write32(tmp, PDS_BASE + PDS_INT_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_INT_OFFSET);
	tmp |= PDS_CLR_RESET_EVENT_MSK;
	sys_write32(tmp, PDS_BASE + PDS_INT_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_INT_OFFSET);
	tmp &= PDS_CLR_RESET_EVENT_UMSK;
	sys_write32(tmp, PDS_BASE + PDS_INT_OFFSET);
}

#elif defined(CONFIG_SOC_SERIES_BL60X)

static uint32_t bflb_get_reset_reason(void)
{
	uint32_t reason = 0;

	if (marked_sw_reboot && marked_npor) {
		reason |= RESET_SOFTWARE;
		reason |= RESET_POR;
	} else if (!marked_npor) {
		reason |= RESET_POR;
	} else {
		/* We don't really know... */
		reason |= RESET_WATCHDOG;
		reason |= RESET_BROWNOUT;
	}

	return reason;
}

static void bflb_clear_reset_reason(void)
{
	/* Nothing to clear */
}

#endif

int z_impl_hwinfo_clear_reset_cause(void)
{
	bflb_clear_reset_reason();

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_POR
		      | RESET_SOFTWARE
		      | RESET_WATCHDOG
		      | RESET_LOW_POWER_WAKE
		      | RESET_BROWNOUT);

	return 0;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t reason = bflb_get_reset_reason();

	*cause = reason;

	return 0;
}

SYS_INIT_NAMED(retained_reset_init, retained_reset_init_func,
	       POST_KERNEL, UTIL_INC(CONFIG_RETAINED_MEM_INIT_PRIORITY));
