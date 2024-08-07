/**
 * intc_ra2_icu.h
 *
 * @brief Renesas RA2 ICU specific definitions and declarations.
 *
 * Copyright (c) 2022-2024 MUNIC Car Data
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __INTC_RA2_ICU_H__
#define __INTC_RA2_ICU_H__

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/dt-bindings/interrupt-controller/renesas-ra2-icu.h>

struct icu_event;

/* It is the user's responsibility to clear the event */
typedef void (*event_cb_t)(struct icu_event *evt, void *callback_data);

typedef enum {
	IRQ_RA_FALL = 0,
	IRQ_RA_RISE = 1,
	IRQ_RA_BOTH = 2,
	IRQ_RA_LOWL = 3,
} irq_ra_sense_t;

typedef enum {
	NMI_RA_FALL = 0,
	NMI_RA_RISE = 1,
} nmi_ra_sense_t;

typedef enum {
	NMI_IRQ_RA_DIV_1  = 0,
	NMI_IRQ_RA_DIV_8  = 1,
	NMI_IRQ_RA_DIV_32 = 2,
	NMI_IRQ_RA_DIV_64 = 3,
} nmi_irq_ra_division_t;

/* Independent watchdog NMI */
#define NMI_IWDTEN BIT(0)
/* Watchdog NMI */
#define NMI_WDTEN  BIT(1)
/* Low voltage detection 1 NMI */
#define NMI_LVD1EN BIT(2)
/* Low voltage detection 2 NMI */
#define NMI_LVD2EN BIT(3)
/* MOSC Oscillation stop detection  NMI */
#define NMI_OSTEN  BIT(6)
/* Pin interrupt  NMI */
#define NMI_NMIEN  BIT(7)
/* SRAM parity error NMI */
#define NMI_RPEEN  BIT(8)
/* SRAM ECC error NMI */
#define NMI_RECCEN BIT(9)
/* MPU Bus slave error NMI */
#define NMI_BUSSEN BIT(10)
/* MPU Bus master error NMI */
#define NMI_BUSMEN BIT(11)
/* CPU Stack pointer monitor NMI */
#define NMI_SPEEN  BIT(12)

/* Wakeup reason definitions */

/* Wakeup on external IRQ (0 through 7) */
#define IRQ_WAKE(x)  BIT(x & 0x7)
/* Wakeup on Independent watchdog */
#define IWDT_WAKE    BIT(16)
/* Wakeup on Key interrupt */
#define KEY_WAKE     BIT(17)
/* Wakeup on Low voltage detection 1 */
#define LVD1_WAKE    BIT(18)
/* Wakeup on Low voltage detection 2 */
#define LVD2_WAKE    BIT(19)
/* Wakeup on Low power analog comparator */
#define ACMPLP0_WAKE BIT(23)
/* Wakeup on RTC alarm */
#define RTCALM_WAKE  BIT(24)
/* Wakeup on RTC period */
#define RTCPRD_WAKE  BIT(25)
/* Wakeup on AGT underflow */
#define AGT1UD_WAKE  BIT(28)
/* Wakeup on AGT compare A */
#define AGT1CA_WAKE  BIT(29)
/* Wakeup on AGT compare B */
#define AGT1CB_WAKE  BIT(30)
/* Wakeup on I2C0 interrupt */
#define IIC0_WAKE    BIT(31)

/* Only use this function when the IRQ is NOT in use */
/* Will fail if irq > 7 */
int ra_set_irq_cfg(unsigned int irq, irq_ra_sense_t sense,
		nmi_irq_ra_division_t div, int filtered);
int ra_get_irq_cfg(unsigned int irq, irq_ra_sense_t *sense,
		nmi_irq_ra_division_t *div, int *filtered);

/* Only use this function when the NMI is NOT in use, i.e before
 * activate_nmi_source()
 */
void ra_set_nmi_cfg(nmi_ra_sense_t sense, nmi_irq_ra_division_t div,
		int filtered);
void ra_get_nmi_cfg(nmi_ra_sense_t *sense, nmi_irq_ra_division_t *div,
		int *filtered);

int ra_activate_wakeup_sources(uint32_t mask);
int ra_deactivate_wakeup_sources(uint32_t mask);
uint32_t ra_get_active_wakeup(void);

/* An NMI can only be activated once after a reset. Some of the sources
 * can be used as event signals, do NOT set them here in that case.
 */
int ra_activate_nmi_sources(uint16_t mask);

/* Result should be &ed with IDX masks to get source */
uint16_t ra_get_active_nmi(void);

/* Input is |ed IDX masks for interrupts to be cleared */
void ra_clear_nmi(uint16_t mask);

struct icu_event *ra_icu_setup_event_irq(int event_number_grouped,
		event_cb_t callback, void *callback_data);

void ra_icu_release_event_irq(struct icu_event *evt);

void ra_icu_set_callback(struct icu_event *event, event_cb_t callback,
			void *callback_data);

int ra_icu_set_priority(struct icu_event *event, unsigned int priority);

int ra_icu_enable_event(struct icu_event *event);
int ra_icu_disable_event(struct icu_event *event);

/* Reroutes the interrupt to the DTC (dma) unit. This API is reserved for the
 * DMA driver, do not use it directly
 */
int ra_icu_set_dtc_flag(struct icu_event *event, bool dtc);

/* Returns the number of the NVIC interrupt corresponding to the given event */
int ra_icu_get_event_irq_num(struct icu_event *event);

int ra_icu_shutdown_event_irq(struct icu_event *event);

void ra_icu_clear_event(struct icu_event *event);

#endif /* __INTC_RA2_ICU_H__ */
