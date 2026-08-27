/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <nrfx.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_ficr.h>

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#if NRF_GPIO_HAS_SEL
void soc_secure_gpio_pin_mcu_select(uint32_t pin_number, nrf_gpio_pin_sel_t mcu);
#endif

int soc_secure_mem_read(void *dst, void *src, size_t len);

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_tz_secure)

/* Each tagged partition expands to one "|| <overlap>" term in a
 * boolean chain. The comparands are DT constants so no runtime table
 * or loop is emitted.
 */
#define Z_SOC_SECURE_FLASH_OVERLAP(p, a, e)                                   \
	((a) < DT_REG_ADDR(p) + DT_REG_SIZE(p) && (e) > DT_REG_ADDR(p))

#define Z_SOC_SECURE_FLASH_OR_TERM(node_id, prop, idx, a, e)                  \
	|| Z_SOC_SECURE_FLASH_OVERLAP(DT_PHANDLE_BY_IDX(node_id, prop, idx),  \
				      a, e)

#define Z_SOC_SECURE_FLASH_NODE_TERMS(node_id, a, e)                          \
	DT_FOREACH_PROP_ELEM_VARGS(node_id, code_partitions,                  \
				   Z_SOC_SECURE_FLASH_OR_TERM, a, e)          \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, data_partitions),                \
		   (DT_FOREACH_PROP_ELEM_VARGS(node_id, data_partitions,      \
					       Z_SOC_SECURE_FLASH_OR_TERM,    \
					       a, e)))

/**
 * @brief Return true if [addr, addr + len) overlaps any partition
 *        tagged "nordic,tz-secure" in the devicetree.
 *
 * @note Empty ranges are reported as non-secure. The caller must
 *       ensure addr + len does not wrap.
 */
static inline bool soc_secure_flash_range_is_secure(uintptr_t addr, size_t len)
{
	if (len == 0) {
		return false;
	}

	uintptr_t end = addr + len;

	return (false
		DT_FOREACH_STATUS_OKAY_VARGS(nordic_tz_secure,
					     Z_SOC_SECURE_FLASH_NODE_TERMS,
					     addr, end));
}

/**
 * @brief Read flash, routing through soc_secure_mem_read() when
 *        [src, src + len) overlaps a "nordic,tz-secure" partition.
 *
 * @note @p src is non-const to match soc_secure_mem_read()'s
 *       signature; the source bytes are not modified.
 */
static inline int soc_secure_flash_read(void *dst, void *src, size_t len)
{
	if (soc_secure_flash_range_is_secure((uintptr_t)src, len)) {
		return soc_secure_mem_read(dst, src, len);
	}
	memcpy(dst, src, len);
	return 0;
}

#else /* no okay "nordic,tz-secure" descriptor */

static inline bool soc_secure_flash_range_is_secure(uintptr_t addr, size_t len)
{
	(void)addr;
	(void)len;
	return false;
}

static inline int soc_secure_flash_read(void *dst, void *src, size_t len)
{
	memcpy(dst, src, len);
	return 0;
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_tz_secure) */

#else /* !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) */
#if NRF_GPIO_HAS_SEL
static inline void soc_secure_gpio_pin_mcu_select(uint32_t pin_number, nrf_gpio_pin_sel_t mcu)
{
	nrf_gpio_pin_control_select(pin_number, mcu);
}
#endif /* NRF_GPIO_HAS_SEL */

static inline int soc_secure_mem_read(void *dst, void *src, size_t len)
{
	(void)memcpy(dst, src, len);
	return 0;
}

static inline bool soc_secure_flash_range_is_secure(uintptr_t addr, size_t len)
{
	(void)addr;
	(void)len;
	return false;
}

static inline int soc_secure_flash_read(void *dst, void *src, size_t len)
{
	memcpy(dst, src, len);
	return 0;
}

#endif /* defined CONFIG_TRUSTED_EXECUTION_NONSECURE */

/* Include these soc_secure_* functions only when the FICR is mapped as secure only */
#if defined(NRF_FICR_S)
#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#if defined(CONFIG_SOC_HFXO_CAP_INTERNAL) || \
	DT_ENUM_HAS_VALUE(DT_NODELABEL(hfxo), load_capacitors, internal)
static inline uint32_t soc_secure_read_xosc32mtrim(void)
{
	uint32_t xosc32mtrim;
	int err;

	err = soc_secure_mem_read(&xosc32mtrim,
				  (void *)&NRF_FICR_S->XOSC32MTRIM,
				  sizeof(xosc32mtrim));
	__ASSERT(err == 0, "Secure read error (%d)", err);

	return xosc32mtrim;
}
#endif /* defined(CONFIG_SOC_HFXO_CAP_INTERNAL) */

static inline void soc_secure_read_deviceid(uint32_t deviceid[2])
{
	int err;

	err = soc_secure_mem_read(deviceid,
				 (void *)&NRF_FICR_S->INFO.DEVICEID,
				 2 * sizeof(uint32_t));
	__ASSERT(err == 0, "Secure read error (%d)", err);
}

#else /* defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) */
#if defined(CONFIG_SOC_HFXO_CAP_INTERNAL) || \
	DT_ENUM_HAS_VALUE(DT_NODELABEL(hfxo), load_capacitors, internal)
static inline uint32_t soc_secure_read_xosc32mtrim(void)
{
	return NRF_FICR_S->XOSC32MTRIM;
}
#endif /* defined(CONFIG_SOC_HFXO_CAP_INTERNAL) */

static inline void soc_secure_read_deviceid(uint32_t deviceid[2])
{
	deviceid[0] = nrf_ficr_deviceid_get(NRF_FICR_S, 0);
	deviceid[1] = nrf_ficr_deviceid_get(NRF_FICR_S, 1);
}

#endif /* defined CONFIG_TRUSTED_EXECUTION_NONSECURE */
#endif /* defined(NRF_FICR_S) */
