/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_NORDIC_COMMON_UICR_UICR_H_
#define SOC_NORDIC_COMMON_UICR_UICR_H_

#include <stdint.h>
#include <nrfx.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

/** Entry in the PERIPHCONF table. */
struct uicr_periphconf_entry {
	/** Register pointer. */
	uint32_t regptr;
	/** Register value. */
	uint32_t value;
} __packed;

/** @brief Add an entry to the PERIPHCONF table section.
 *
 * This should typically not be used directly.
 * Prefer to use one of the higher level macros.
 */
#define UICR_PERIPHCONF_ADD(_regptr, _value)                                                       \
	static STRUCT_SECTION_ITERABLE(uicr_periphconf_entry,                                      \
				       _UICR_PERIPHCONF_ENTRY_NAME(__COUNTER__)) = {               \
		.regptr = (_regptr),                                                               \
		.value = (_value),                                                                 \
	}

#define _UICR_PERIPHCONF_ENTRY_NAME(_id)  __UICR_PERIPHCONF_ENTRY_NAME(_id)
#define __UICR_PERIPHCONF_ENTRY_NAME(_id) _uicr_periphconf_entry_##_id

/** @brief Add a PERIPHCONF entry for a SPU PERIPH[n].PERM register value.
 *
 * @param _spu Global domain SPU instance address.
 * @param _index Peripheral slave index on the bus (PERIPH[n] register index).
 * @param _secattr If true, set SECATTR to secure, otherwise set it to non-secure.
 * @param _dmasec If true, set DMASEC to secure, otherwise set it to non-secure.
 * @param _ownerid OWNERID field value.
 */
#define UICR_SPU_PERIPH_PERM_SET(_spu, _index, _secattr, _dmasec, _ownerid)                        \
	UICR_PERIPHCONF_ADD((uint32_t)&((NRF_SPU_Type *)(_spu))->PERIPH[(_index)].PERM,            \
			    (uint32_t)((((_ownerid) << SPU_PERIPH_PERM_OWNERID_Pos) &              \
					SPU_PERIPH_PERM_OWNERID_Msk) |                             \
				       (((_secattr) ? SPU_PERIPH_PERM_SECATTR_Secure               \
						    : SPU_PERIPH_PERM_SECATTR_NonSecure)           \
					<< SPU_PERIPH_PERM_SECATTR_Pos) |                          \
				       (((_dmasec) ? SPU_PERIPH_PERM_DMASEC_Secure                 \
						   : SPU_PERIPH_PERM_DMASEC_NonSecure)             \
					<< SPU_PERIPH_PERM_DMASEC_Pos) |                           \
				       (SPU_PERIPH_PERM_LOCK_Locked << SPU_PERIPH_PERM_LOCK_Pos)))

/** @brief Add a PERIPHCONF entry for a SPU FEATURE.IPCT.CH[n] register value.
 *
 * @param _spu Global domain SPU instance address.
 * @param _index Feature index.
 * @param _secattr If true, set SECATTR to secure, otherwise set it to non-secure.
 * @param _ownerid OWNERID field value.
 */
#define UICR_SPU_FEATURE_IPCT_CH_SET(_spu, _index, _secattr, _ownerid)                             \
	UICR_PERIPHCONF_ADD((uint32_t)&((NRF_SPU_Type *)(_spu))->FEATURE.IPCT.CH[_index],          \
			    _UICR_SPU_FEATURE_VAL(_secattr, _ownerid))

/** @brief Add a PERIPHCONF entry for a SPU FEATURE.IPCT.INTERRUPT[n] register value.
 *
 * @param _spu Global domain SPU instance address.
 * @param _index Feature index.
 * @param _secattr If true, set SECATTR to secure, otherwise set it to non-secure.
 * @param _ownerid OWNERID field value.
 */
#define UICR_SPU_FEATURE_IPCT_INTERRUPT_SET(_spu, _index, _secattr, _ownerid)                      \
	UICR_PERIPHCONF_ADD((uint32_t)&((NRF_SPU_Type *)(_spu))->FEATURE.IPCT.INTERRUPT[_index],   \
			    _UICR_SPU_FEATURE_VAL(_secattr, _ownerid))

/** @brief Add a PERIPHCONF entry for a SPU FEATURE.DPPIC.CH[n] register value.
 *
 * @param _spu Global domain SPU instance address.
 * @param _index Feature index.
 * @param _secattr If true, set SECATTR to secure, otherwise set it to non-secure.
 * @param _ownerid OWNERID field value.
 */
#define UICR_SPU_FEATURE_DPPIC_CH_SET(_spu, _index, _secattr, _ownerid)                            \
	UICR_PERIPHCONF_ADD((uint32_t)&((NRF_SPU_Type *)(_spu))->FEATURE.DPPIC.CH[_index],         \
			    _UICR_SPU_FEATURE_VAL(_secattr, _ownerid))

/** @brief Add a PERIPHCONF entry for a SPU FEATURE.DPPIC.CHG[n] register value.
 *
 * @param _spu Global domain SPU instance address.
 * @param _index Register index.
 * @param _secattr If true, set SECATTR to secure, otherwise set it to non-secure.
 * @param _ownerid OWNERID field value.
 */
#define UICR_SPU_FEATURE_DPPIC_CHG_SET(_spu, _index, _secattr, _ownerid)                           \
	UICR_PERIPHCONF_ADD((uint32_t)&((NRF_SPU_Type *)(_spu))->FEATURE.DPPIC.CHG[_index],        \
			    _UICR_SPU_FEATURE_VAL(_secattr, _ownerid))

/** @brief Add a PERIPHCONF entry for a SPU FEATURE.GPIOTE[n].CH[m] register value.
 *
 * @param _spu Global domain SPU instance address.
 * @param _index Feature index (GPIOTE[n] register index).
 * @param _subindex Feature subindex (CH[m] register index).
 * @param _secattr If true, set the SECATTR to secure, otherwise set it to non-secure.
 * @param _ownerid OWNERID field value.
 */
#define UICR_SPU_FEATURE_GPIOTE_CH_SET(_spu, _index, _subindex, _secattr, _ownerid)                \
	UICR_PERIPHCONF_ADD(                                                                       \
		(uint32_t)&((NRF_SPU_Type *)(_spu))->FEATURE.GPIOTE[_index].CH[_subindex],         \
		_UICR_SPU_FEATURE_VAL(_secattr, _ownerid))

/** @brief Add a PERIPHCONF entry for a SPU FEATURE.GPIOTE.INTERRUPT[n] register value.
 *
 * @param _spu Global domain SPU instance address.
 * @param _index Feature index.
 * @param _subindex Feature subindex.
 * @param _secattr If true, set SECATTR to secure, otherwise set it to non-secure.
 * @param _ownerid OWNERID field value.
 */
#define UICR_SPU_FEATURE_GPIOTE_INTERRUPT_SET(_spu, _index, _subindex, _secattr, _ownerid)         \
	UICR_PERIPHCONF_ADD(                                                                       \
		(uint32_t)&((NRF_SPU_Type *)(_spu))->FEATURE.GPIOTE[_index].INTERRUPT[_subindex],  \
		_UICR_SPU_FEATURE_VAL(_secattr, _ownerid))

/** @brief Add a PERIPHCONF entry for a SPU FEATURE.GPIO[n].PIN[m] register value.
 *
 * @param _spu Global domain SPU instance address.
 * @param _index Feature index.
 * @param _subindex Feature subindex.
 * @param _secattr If true, set SECATTR to secure, otherwise set it to non-secure.
 * @param _ownerid OWNERID field value.
 */
#define UICR_SPU_FEATURE_GPIO_PIN_SET(_spu, _index, _subindex, _secattr, _ownerid)                 \
	UICR_PERIPHCONF_ADD(                                                                       \
		(uint32_t)&((NRF_SPU_Type *)(_spu))->FEATURE.GPIO[_index].PIN[_subindex],          \
		_UICR_SPU_FEATURE_VAL(_secattr, _ownerid))

/** @brief Add a PERIPHCONF entry for a SPU FEATURE.GRTC.CC[n] register value.
 *
 * @param _spu Global domain SPU instance address.
 * @param _index Feature index.
 * @param _secattr If true, set SECATTR to secure, otherwise set it to non-secure.
 * @param _ownerid OWNERID field value.
 */
#define UICR_SPU_FEATURE_GRTC_CC_SET(_spu, _index, _secattr, _ownerid)                             \
	UICR_PERIPHCONF_ADD((uint32_t)&((NRF_SPU_Type *)(_spu))->FEATURE.GRTC.CC[_index],          \
			    _UICR_SPU_FEATURE_VAL(_secattr, _ownerid))

/* Common macro for encoding a SPU FEATURE.* register value.
 * Note that the MDK SPU_FEATURE_IPCT_CH_ macros are used for all since all the FEATURE registers
 * have the same layout with different naming.
 */
#define _UICR_SPU_FEATURE_VAL(_secattr, _ownerid)                                                  \
	(uint32_t)((((_ownerid) << SPU_FEATURE_IPCT_CH_OWNERID_Pos) &                              \
		    SPU_FEATURE_IPCT_CH_OWNERID_Msk) |                                             \
		   (((_secattr) ? SPU_FEATURE_IPCT_CH_SECATTR_Secure                               \
				: SPU_FEATURE_IPCT_CH_SECATTR_NonSecure)                           \
		    << SPU_FEATURE_IPCT_CH_SECATTR_Pos) |                                          \
		   (SPU_FEATURE_IPCT_CH_LOCK_Locked << SPU_FEATURE_IPCT_CH_LOCK_Pos))

/** @brief Add PERIPHCONF entries for configuring IPCMAP CHANNEL.SOURCE[n] and CHANNEL.SINK[n].
 *
 * @param _index CHANNEL.SOURCE[n]/CHANNEL.SINK[n] register index.
 * @param _source_domain DOMAIN field value in CHANNEL[n].SOURCE.
 * @param _source_ch SOURCE field value in CHANNEL[n].SOURCE.
 * @param _sink_domain DOMAIN field value in CHANNEL[n].SINK.
 * @param _sink_ch SINK field value in CHANNEL[n].SINK.
 */
#define UICR_IPCMAP_CHANNEL_CFG(_index, _source_domain, _source_ch, _sink_domain, _sink_ch)        \
	UICR_IPCMAP_CHANNEL_SOURCE_SET(_index, _source_domain, _source_ch, 1);                     \
	UICR_IPCMAP_CHANNEL_SINK_SET(_index, _sink_domain, _sink_ch)

#define UICR_IPCMAP_CHANNEL_SOURCE_SET(_index, _domain, _ch, _enable)                              \
	UICR_PERIPHCONF_ADD((uint32_t)&NRF_IPCMAP->CHANNEL[(_index)].SOURCE,                       \
			    (uint32_t)((((_domain) << IPCMAP_CHANNEL_SOURCE_DOMAIN_Pos) &          \
					IPCMAP_CHANNEL_SOURCE_DOMAIN_Msk) |                        \
				       (((_ch) << IPCMAP_CHANNEL_SOURCE_SOURCE_Pos) &              \
					IPCMAP_CHANNEL_SOURCE_SOURCE_Msk) |                        \
				       (((_enable) ? IPCMAP_CHANNEL_SOURCE_ENABLE_Enabled          \
						   : IPCMAP_CHANNEL_SOURCE_ENABLE_Disabled)        \
					<< IPCMAP_CHANNEL_SOURCE_ENABLE_Pos)))

#define UICR_IPCMAP_CHANNEL_SINK_SET(_index, _domain, _ch)                                         \
	UICR_PERIPHCONF_ADD((uint32_t)&NRF_IPCMAP->CHANNEL[(_index)].SINK,                         \
			    (uint32_t)((((_domain) << IPCMAP_CHANNEL_SINK_DOMAIN_Pos) &            \
					IPCMAP_CHANNEL_SINK_DOMAIN_Msk) |                          \
				       (((_ch) << IPCMAP_CHANNEL_SINK_SINK_Pos) &                  \
					IPCMAP_CHANNEL_SINK_SINK_Msk)))

/** @brief Add a PERIPHCONF entry for an IRQMAP IRQ[n].SINK register value.
 *
 * @param _irqnum IRQ number (IRQ[n] register index).
 * @param _processor Processor to route the interrupt to (PROCESSORID field value).
 */
#define UICR_IRQMAP_IRQ_SINK_SET(_irqnum, _processor)                                              \
	UICR_PERIPHCONF_ADD((uint32_t)&NRF_IRQMAP->IRQ[(_irqnum)].SINK,                            \
			    (uint32_t)(((_processor) << IRQMAP_IRQ_SINK_PROCESSORID_Pos) &         \
				       IRQMAP_IRQ_SINK_PROCESSORID_Msk))

/** @brief Add a PERIPHCONF entry for configuring a GPIO PIN_CNF[n] CTRLSEL field value.
 *
 * @param _gpio GPIO instance address.
 * @param _pin Pin number (PIN_CNF[n] register index).
 * @param _ctrlsel CTRLSEL field value.
 */
#define UICR_GPIO_PIN_CNF_CTRLSEL_SET(_gpio, _pin, _ctrlsel)                                       \
	UICR_PERIPHCONF_ADD(                                                                       \
		(uint32_t)&((NRF_GPIO_Type *)(_gpio))->PIN_CNF[(_pin)],                            \
		((GPIO_PIN_CNF_ResetValue) |                                                       \
		 (uint32_t)(((_ctrlsel) << GPIO_PIN_CNF_CTRLSEL_Pos) & GPIO_PIN_CNF_CTRLSEL_Msk)))

/** @brief Add a PERIPHCONF entry for a PPIB SUBSCRIBE_SEND[n] register.
 *
 * @param _ppib Global domain PPIB instance address.
 * @param _ppib_ch PPIB channel number.
 */
#define UICR_PPIB_SUBSCRIBE_SEND_ENABLE(_ppib, _ppib_ch)                                           \
	UICR_PERIPHCONF_ADD((uint32_t)&((NRF_PPIB_Type *)(_ppib))->SUBSCRIBE_SEND[(_ppib_ch)],     \
			    (uint32_t)PPIB_SUBSCRIBE_SEND_EN_Msk)

/** @brief Add a PERIPHCONF entry for a PPIB PUBLISH_RECEIVE[n] register.
 *
 * @param _ppib Global domain PPIB instance address.
 * @param _ppib_ch PPIB channel number.
 */
#define UICR_PPIB_PUBLISH_RECEIVE_ENABLE(_ppib, _ppib_ch)                                          \
	UICR_PERIPHCONF_ADD((uint32_t)&((NRF_PPIB_Type *)(_ppib))->PUBLISH_RECEIVE[(_ppib_ch)],    \
			    (uint32_t)PPIB_PUBLISH_RECEIVE_EN_Msk)

/* The definitions below are not currently available in the MDK but are needed for the macros
 * above. When they are, this can be deleted.
 */
#ifndef IPCMAP_CHANNEL_SOURCE_SOURCE_Msk

typedef struct {
	__IOM uint32_t SOURCE;
	__IOM uint32_t SINK;
} NRF_IPCMAP_CHANNEL_Type;

#define IPCMAP_CHANNEL_SOURCE_SOURCE_Pos      (0UL)
#define IPCMAP_CHANNEL_SOURCE_SOURCE_Msk      (0xFUL << IPCMAP_CHANNEL_SOURCE_SOURCE_Pos)
#define IPCMAP_CHANNEL_SOURCE_DOMAIN_Pos      (8UL)
#define IPCMAP_CHANNEL_SOURCE_DOMAIN_Msk      (0xFUL << IPCMAP_CHANNEL_SOURCE_DOMAIN_Pos)
#define IPCMAP_CHANNEL_SOURCE_ENABLE_Pos      (31UL)
#define IPCMAP_CHANNEL_SOURCE_ENABLE_Disabled (0x0UL)
#define IPCMAP_CHANNEL_SOURCE_ENABLE_Enabled  (0x1UL)
#define IPCMAP_CHANNEL_SINK_SINK_Pos          (0UL)
#define IPCMAP_CHANNEL_SINK_SINK_Msk          (0xFUL << IPCMAP_CHANNEL_SINK_SINK_Pos)
#define IPCMAP_CHANNEL_SINK_DOMAIN_Pos        (8UL)
#define IPCMAP_CHANNEL_SINK_DOMAIN_Msk        (0xFUL << IPCMAP_CHANNEL_SINK_DOMAIN_Pos)

typedef struct {
	__IM uint32_t RESERVED[256];
	__IOM NRF_IPCMAP_CHANNEL_Type CHANNEL[16];
} NRF_IPCMAP_Type;

#endif /* IPCMAP_CHANNEL_SOURCE_SOURCE_Msk */

#ifndef NRF_IPCMAP
#define NRF_IPCMAP ((NRF_IPCMAP_Type *)0x5F923000UL)
#endif

#ifndef IRQMAP_IRQ_SINK_PROCESSORID_Msk

typedef struct {
	__IOM uint32_t SINK;
} NRF_IRQMAP_IRQ_Type;

#define IRQMAP_IRQ_SINK_PROCESSORID_Pos (8UL)
#define IRQMAP_IRQ_SINK_PROCESSORID_Msk (0xFUL << IRQMAP_IRQ_SINK_PROCESSORID_Pos)

typedef struct {
	__IM uint32_t RESERVED[256];
	__IOM NRF_IRQMAP_IRQ_Type IRQ[480];
} NRF_IRQMAP_Type;

#endif /* IRQMAP_IRQ_SINK_PROCESSORID_Msk */

#ifndef NRF_IRQMAP
#define NRF_IRQMAP ((NRF_IRQMAP_Type *)0x5F924000UL)
#endif /* NRF_IRQMAP */

#ifndef GPIO_PIN_CNF_CTRLSEL_Pos

#define GPIO_PIN_CNF_CTRLSEL_Pos (28UL)
#define GPIO_PIN_CNF_CTRLSEL_Msk (0x7UL << GPIO_PIN_CNF_CTRLSEL_Pos)
#define GPIO_PIN_CNF_CTRLSEL_Min (0x0UL)
#define GPIO_PIN_CNF_CTRLSEL_Max (0x7UL)
#define GPIO_PIN_CNF_CTRLSEL_GPIO (0x0UL)
#define GPIO_PIN_CNF_CTRLSEL_VPR (0x1UL)
#define GPIO_PIN_CNF_CTRLSEL_GRC (0x1UL)
#define GPIO_PIN_CNF_CTRLSEL_SecureDomain (0x2UL)
#define GPIO_PIN_CNF_CTRLSEL_PWM (0x2UL)
#define GPIO_PIN_CNF_CTRLSEL_I3C (0x2UL)
#define GPIO_PIN_CNF_CTRLSEL_Serial (0x3UL)
#define GPIO_PIN_CNF_CTRLSEL_HSSPI (0x3UL)
#define GPIO_PIN_CNF_CTRLSEL_RadioCore (0x4UL)
#define GPIO_PIN_CNF_CTRLSEL_EXMIF (0x4UL)
#define GPIO_PIN_CNF_CTRLSEL_CELL (0x4UL)
#define GPIO_PIN_CNF_CTRLSEL_DTB (0x6UL)
#define GPIO_PIN_CNF_CTRLSEL_TND (0x7UL)

#endif /* GPIO_PIN_CNF_CTRLSEL_Pos */

#endif /* SOC_NORDIC_COMMON_UICR_UICR_H_ */
