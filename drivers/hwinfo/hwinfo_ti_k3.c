/*
 * SPDX-FileCopyrightText: 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(hwinfo_ti_k3, CONFIG_HWINFO_LOG_LEVEL);

#define JTAGID_NODE  DT_NODELABEL(jtagid)
#define RST_SRC_NODE DT_NODELABEL(reset_status)

static const struct device *const jtagid_syscon = DEVICE_DT_GET(DT_PARENT(JTAGID_NODE));
static const struct device *const rst_src_syscon = DEVICE_DT_GET(DT_PARENT(RST_SRC_NODE));

#define JTAGID_OFFSET  DT_REG_ADDR_RAW(JTAGID_NODE)
#define RST_SRC_OFFSET DT_REG_ADDR_RAW(RST_SRC_NODE)

#define RST_SRC_DDRSS_TO BIT(11) /* DDRSS timeout */
#define RST_SRC_DEBUG    BIT(10) /* Debug subsystem */
#define RST_SRC_THERMAL  BIT(9)  /* Thermal */
#define RST_SRC_MPU_WDT1 BIT(5)  /* MPU watchdog 1 */
#define RST_SRC_MPU_WDT0 BIT(4)  /* MPU watchdog 0 */
#define RST_SRC_SMS_WARM BIT(3)  /* Security management subsystem, warm reset */
#define RST_SRC_SMS_COLD BIT(2)  /* Security management subsystem, cold reset */
#define RST_SRC_SW_WARM  BIT(1)  /* SW_WARMRESET request */
#define RST_SRC_PIN      BIT(0)  /* Device warm reset (RESETz pin) */

#define RST_SRC_ALL                                                                                \
	(RST_SRC_DDRSS_TO | RST_SRC_DEBUG | RST_SRC_THERMAL | RST_SRC_MPU_WDT1 |                   \
	 RST_SRC_MPU_WDT0 | RST_SRC_SMS_WARM | RST_SRC_SMS_COLD | RST_SRC_SW_WARM | RST_SRC_PIN)

#define JTAGID_VARIANT GENMASK(31, 28)
#define JTAGID_PARTNO  GENMASK(27, 12)
#define JTAGID_MFG     GENMASK(11, 1)
#define JTAGID_MFG_TI  0x17U

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	struct __packed {
		uint16_t partno;
		uint8_t variant;
	} info;
	uint32_t jtagid;
	uint32_t mfg;
	int ret;

	if (!device_is_ready(jtagid_syscon)) {
		return -ENODEV;
	}

	ret = syscon_read_reg(jtagid_syscon, JTAGID_OFFSET, &jtagid);
	if (ret != 0) {
		LOG_ERR("Failed to read JTAGID (%d)", ret);
		return ret;
	}

	mfg = FIELD_GET(JTAGID_MFG, jtagid);
	if (mfg != JTAGID_MFG_TI) {
		LOG_ERR("Invalid manufacturer ID: 0x%x", mfg);
		return -ENODEV;
	}

	info.partno = (uint16_t)FIELD_GET(JTAGID_PARTNO, jtagid);
	info.variant = (uint8_t)FIELD_GET(JTAGID_VARIANT, jtagid);

	length = MIN(length, sizeof(info));
	memcpy(buffer, &info, length);

	return length;
}

/* hwinfo_get_reset_cause() must return 0 after hwinfo_clear_reset_cause() */
static bool reset_cleared;

static uint32_t rst_src_to_cause(uint32_t src)
{
	uint32_t cause = 0;

	if ((src & RST_SRC_PIN) != 0U) {
		cause |= RESET_PIN;
	}
	if ((src & RST_SRC_SW_WARM) != 0U) {
		cause |= RESET_SOFTWARE;
	}
	if ((src & (RST_SRC_SMS_COLD | RST_SRC_SMS_WARM)) != 0U) {
		cause |= RESET_SECURITY;
	}
	if ((src & (RST_SRC_MPU_WDT0 | RST_SRC_MPU_WDT1)) != 0U) {
		cause |= RESET_WATCHDOG;
	}
	if ((src & RST_SRC_THERMAL) != 0U) {
		cause |= RESET_TEMPERATURE;
	}
	if ((src & RST_SRC_DEBUG) != 0U) {
		cause |= RESET_DEBUG;
	}
	if ((src & RST_SRC_DDRSS_TO) != 0U) {
		cause |= RESET_HARDWARE;
	}

	return cause;
}

static int rst_src_read(uint32_t *src)
{
	int ret;

	if (!device_is_ready(rst_src_syscon)) {
		return -ENODEV;
	}

	ret = syscon_read_reg(rst_src_syscon, RST_SRC_OFFSET, src);
	if (ret != 0) {
		LOG_ERR("Failed to read RST_SRC (%d)", ret);
		return ret;
	}

	*src &= RST_SRC_ALL;

	return 0;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t src;
	int ret;

	if (reset_cleared) {
		*cause = 0;
		return 0;
	}

	ret = rst_src_read(&src);
	if (ret != 0) {
		return ret;
	}

	*cause = rst_src_to_cause(src);
	if (*cause == 0U) {
		/* RST_SRC is 0 after a POR */
		*cause = RESET_POR;
	}

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	uint32_t src;
	int ret;

	ret = rst_src_read(&src);
	if (ret != 0) {
		return ret;
	}

	ret = syscon_write_reg(rst_src_syscon, RST_SRC_OFFSET, src);
	if (ret != 0) {
		LOG_ERR("Failed to clear RST_SRC (%d)", ret);
		return ret;
	}

	reset_cleared = true;

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = rst_src_to_cause(RST_SRC_ALL) | RESET_POR;

	return 0;
}
