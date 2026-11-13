/*
 * Copyright (c) 2025 Linumiz GmbH
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/sys_io.h>
#include <string.h>

/*
 * FACTORYREGION - read-only OTP region containing factory-programmed device
 * identity data. Base address differs per SoC family; resolved from DT at
 * compile time via DT_NODELABEL(factoryregion).
 *
 *   MSPM0:   0x41C40000
 *   MSPM33C: 0x80111000
 *   AM13E:   0x60111000
 */
#define MSPM0_FACTORYREGION_BASE	DT_REG_ADDR(DT_NODELABEL(factoryregion))

#define MSPM0_FACTORYREGION_TRACEID	MSPM0_FACTORYREGION_BASE
#define MSPM0_FACTORYREGION_DEVICEID	(MSPM0_FACTORYREGION_BASE + 0x04U)
#define MSPM0_FACTORYREGION_USERID	(MSPM0_FACTORYREGION_BASE + 0x08U)

/* DEVICEID[31:0] packed fields */
#define MSPM0_DEVICEID_MANUFACTURER_MASK	0x00000FFEU
#define MSPM0_DEVICEID_MANUFACTURER_OFS		1U
#define MSPM0_DEVICEID_PARTNUM_MASK		0x0FFFF000U
#define MSPM0_DEVICEID_PARTNUM_OFS		12U
#define MSPM0_DEVICEID_VERSION_MASK		0xF0000000U
#define MSPM0_DEVICEID_VERSION_OFS		28U

/* USERID[31:0] packed fields */
#define MSPM0_USERID_PART_MASK		0x0000FFFFU
#define MSPM0_USERID_PART_OFS		0U
#define MSPM0_USERID_VARIANT_MASK	0x00FF0000U
#define MSPM0_USERID_VARIANT_OFS	16U

/*
 * SYSCTL RSTCAUSE - records the cause of the last reset.
 * IMPORTANT: the ID field [4:0] is READ-TO-CLEAR. Reading it clears the
 * register. The value must be cached after the first read.
 *
 * SYSCTL base and RSTCAUSE offset are identical across MSPM0, MSPM33, AM13.
 */
#define MSPM0_SYSCTL_BASE	DT_REG_ADDR(DT_NODELABEL(sysctl))
#define MSPM0_SYSCTL_RSTCAUSE	(MSPM0_SYSCTL_BASE + 0x1220U)
#define MSPM0_RSTCAUSE_ID_MASK	0x0000001FU

/* RSTCAUSE.ID field values - TRM SYSCTL chapter */
#define MSPM0_RSTCAUSE_PORHWFAIL	0x01U /* POR hardware failure          */
#define MSPM0_RSTCAUSE_POREXNRST	0x02U /* NRST > 1s triggered POR       */
#define MSPM0_RSTCAUSE_PORSW		0x03U /* Software triggered POR        */
#define MSPM0_RSTCAUSE_BORSUPPLY	0x04U /* BOR0 supply violation         */
#define MSPM0_RSTCAUSE_BORWAKESHUTDN	0x05U /* Wake from SHUTDOWN mode       */
#define MSPM0_RSTCAUSE_BOOTPARITY	0x08U /* Non-PMU trim parity fault     */
#define MSPM0_RSTCAUSE_BOOTCLKFAIL	0x09U /* Fatal clock failure           */
#define MSPM0_RSTCAUSE_BOOTEXNRST	0x0CU /* NRST < 1s triggered BOOTRST   */
#define MSPM0_RSTCAUSE_BOOTSW		0x0DU /* Software triggered BOOTRST    */
#define MSPM0_RSTCAUSE_SYSWWDT0		0x0EU /* WWDT0 window violation        */
#define MSPM0_RSTCAUSE_SYSBSLEXIT	0x10U /* BSL exit                      */
#define MSPM0_RSTCAUSE_SYSBSLENTRY	0x11U /* BSL entry                     */
#define MSPM0_RSTCAUSE_SYSWWDT1		0x13U /* WWDT1 window violation        */
#define MSPM0_RSTCAUSE_SYSFLASHECC	0x14U /* Flash uncorrectable ECC error */
#define MSPM0_RSTCAUSE_SYSCPULOCK	0x15U /* CPU lockup                    */
#define MSPM0_RSTCAUSE_SYSDBG		0x1AU /* Debug triggered SYSRST        */
#define MSPM0_RSTCAUSE_SYSSW		0x1BU /* Software triggered SYSRST     */
#define MSPM0_RSTCAUSE_CPUDBG		0x1CU /* Debug triggered CPURST        */
#define MSPM0_RSTCAUSE_CPUSW		0x1DU /* Software triggered CPURST     */

/* Cached reset cause - RSTCAUSE register is read-to-clear */
static uint32_t reset_cause;

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	struct mspm0_device_id {
		uint16_t manufacturer;
		uint16_t partnum;
		uint8_t version;
		uint32_t traceid;
		uint16_t user_partnum;
		uint8_t user_var;
	};

	struct mspm0_device_id info = {0};
	uint32_t deviceid;
	uint32_t userid;

	info.traceid = sys_read32(MSPM0_FACTORYREGION_TRACEID);

	deviceid = sys_read32(MSPM0_FACTORYREGION_DEVICEID);
	info.manufacturer = (uint16_t)((deviceid & MSPM0_DEVICEID_MANUFACTURER_MASK) >>
				       MSPM0_DEVICEID_MANUFACTURER_OFS);
	info.partnum      = (uint16_t)((deviceid & MSPM0_DEVICEID_PARTNUM_MASK) >>
				       MSPM0_DEVICEID_PARTNUM_OFS);
	info.version      = (uint8_t)((deviceid & MSPM0_DEVICEID_VERSION_MASK) >>
				      MSPM0_DEVICEID_VERSION_OFS);

	userid = sys_read32(MSPM0_FACTORYREGION_USERID);
	info.user_partnum = (uint16_t)((userid & MSPM0_USERID_PART_MASK) >>
				       MSPM0_USERID_PART_OFS);
	info.user_var     = (uint8_t)((userid & MSPM0_USERID_VARIANT_MASK) >>
				      MSPM0_USERID_VARIANT_OFS);

	if (length > sizeof(info)) {
		length = sizeof(info);
	}
	memcpy(buffer, &info, length);

	return (ssize_t)length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t reason;

	if (reset_cause != 0) {
		*cause = reset_cause;
		return 0;
	}

	reason = sys_read32(MSPM0_SYSCTL_RSTCAUSE) & MSPM0_RSTCAUSE_ID_MASK;
	switch (reason) {
	case MSPM0_RSTCAUSE_PORHWFAIL:
		*cause = RESET_POR;
		break;
	case MSPM0_RSTCAUSE_POREXNRST:
		__fallthrough;
	case MSPM0_RSTCAUSE_BOOTEXNRST:
		*cause = RESET_PIN;
		break;
	case MSPM0_RSTCAUSE_PORSW:
		__fallthrough;
	case MSPM0_RSTCAUSE_BOOTSW:
		__fallthrough;
	case MSPM0_RSTCAUSE_SYSSW:
		__fallthrough;
	case MSPM0_RSTCAUSE_CPUSW:
		*cause = RESET_SOFTWARE;
		break;
	case MSPM0_RSTCAUSE_BORSUPPLY:
		*cause = RESET_BROWNOUT;
		break;
	case MSPM0_RSTCAUSE_BORWAKESHUTDN:
		*cause = RESET_LOW_POWER_WAKE;
		break;
	case MSPM0_RSTCAUSE_BOOTPARITY:
		*cause = RESET_PARITY;
		break;
	case MSPM0_RSTCAUSE_BOOTCLKFAIL:
		*cause = RESET_CLOCK;
		break;
	case MSPM0_RSTCAUSE_SYSBSLEXIT:
		__fallthrough;
	case MSPM0_RSTCAUSE_SYSBSLENTRY:
		*cause = RESET_BOOTLOADER;
		break;
	case MSPM0_RSTCAUSE_SYSWWDT0:
		__fallthrough;
	case MSPM0_RSTCAUSE_SYSWWDT1:
		*cause = RESET_WATCHDOG;
		break;
	case MSPM0_RSTCAUSE_SYSFLASHECC:
		*cause = RESET_FLASH;
		break;
	case MSPM0_RSTCAUSE_SYSCPULOCK:
		*cause = RESET_CPU_LOCKUP;
		break;
	case MSPM0_RSTCAUSE_SYSDBG:
		__fallthrough;
	case MSPM0_RSTCAUSE_CPUDBG:
		*cause = RESET_DEBUG;
		break;
	default:
		*cause = 0;
		break;
	}

	reset_cause = *cause;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	reset_cause = 0;

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = RESET_POR
		   | RESET_PIN
		   | RESET_SOFTWARE
		   | RESET_BROWNOUT
		   | RESET_LOW_POWER_WAKE
		   | RESET_PARITY
		   | RESET_CLOCK
		   | RESET_BOOTLOADER
		   | RESET_WATCHDOG
		   | RESET_FLASH
		   | RESET_CPU_LOCKUP
		   | RESET_DEBUG;

	return 0;
}
