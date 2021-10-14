/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ESP_INTR_ALLOC_H__
#define ZEPHYR_INCLUDE_DRIVERS_ESP_INTR_ALLOC_H__

#include <stdint.h>
#include <stdbool.h>

/* number of possible interrupts per core */
#define ESP_INTC_INTS_NUM		(32)

/*
 * Interrupt allocation flags - These flags can be used to specify
 * which interrupt qualities the code calling esp_intr_alloc* needs.
 *
 */

/* Keep the LEVELx values as they are here; they match up with (1<<level) */
#define ESP_INTR_FLAG_LEVEL1		(1<<1)	/* Accept a Level 1 int vector, lowest priority */
#define ESP_INTR_FLAG_LEVEL2		(1<<2)	/* Accept a Level 2 int vector */
#define ESP_INTR_FLAG_LEVEL3		(1<<3)	/* Accept a Level 3 int vector */
#define ESP_INTR_FLAG_LEVEL4		(1<<4)	/* Accept a Level 4 int vector */
#define ESP_INTR_FLAG_LEVEL5		(1<<5)	/* Accept a Level 5 int vector */
#define ESP_INTR_FLAG_LEVEL6		(1<<6)	/* Accept a Level 6 int vector */
#define ESP_INTR_FLAG_NMI		(1<<7)	/* Accept a Level 7 int vector, highest priority */
#define ESP_INTR_FLAG_SHARED		(1<<8)	/* Interrupt can be shared between ISRs */
#define ESP_INTR_FLAG_EDGE		(1<<9)	/* Edge-triggered interrupt */
#define ESP_INTR_FLAG_IRAM		(1<<10)	/* ISR can be called if cache is disabled */
#define ESP_INTR_FLAG_INTRDISABLED	(1<<11)	/* Return with this interrupt disabled */

/* Low and medium prio interrupts. These can be handled in C. */
#define ESP_INTR_FLAG_LOWMED	(ESP_INTR_FLAG_LEVEL1|ESP_INTR_FLAG_LEVEL2|ESP_INTR_FLAG_LEVEL3)

/* High level interrupts. Need to be handled in assembly. */
#define ESP_INTR_FLAG_HIGH	(ESP_INTR_FLAG_LEVEL4|ESP_INTR_FLAG_LEVEL5|ESP_INTR_FLAG_LEVEL6| \
				 ESP_INTR_FLAG_NMI)

/* Mask for all level flags */
#define ESP_INTR_FLAG_LEVELMASK	(ESP_INTR_FLAG_LEVEL1|ESP_INTR_FLAG_LEVEL2|ESP_INTR_FLAG_LEVEL3| \
				 ESP_INTR_FLAG_LEVEL4|ESP_INTR_FLAG_LEVEL5|ESP_INTR_FLAG_LEVEL6| \
				 ESP_INTR_FLAG_NMI)

/*
 * The esp_intr_alloc* functions can allocate an int for all *_INTR_SOURCE int sources that
 * are routed through the interrupt mux. Apart from these sources, each core also has some internal
 * sources that do not pass through the interrupt mux. To allocate an interrupt for these sources,
 * pass these pseudo-sources to the functions.
 */
#define ETS_INTERNAL_TIMER0_INTR_SOURCE         -1 /* Xtensa timer 0 interrupt source */
#define ETS_INTERNAL_TIMER1_INTR_SOURCE         -2 /* Xtensa timer 1 interrupt source */
#define ETS_INTERNAL_TIMER2_INTR_SOURCE         -3 /* Xtensa timer 2 interrupt source */
#define ETS_INTERNAL_SW0_INTR_SOURCE            -4 /* Software int source 1 */
#define ETS_INTERNAL_SW1_INTR_SOURCE            -5 /* Software int source 2 */
#define ETS_INTERNAL_PROFILING_INTR_SOURCE      -6 /* Int source for profiling */

/* Function prototype for interrupt handler function */
typedef void (*intr_handler_t)(void *arg);

struct shared_vector_desc_t {
	int disabled : 1;
	int source : 8;
	volatile uint32_t *statusreg;
	uint32_t statusmask;
	intr_handler_t isr;
	void *arg;
	struct shared_vector_desc_t *next;
};

/* Pack using bitfields for better memory use */
struct vector_desc_t {
	int flags : 16;                                 /* OR of VECDESC_FLAG_* defines */
	unsigned int cpu : 1;
	unsigned int intno : 5;
	int source : 8;                                 /* Int mux flags, used when not shared */
	struct shared_vector_desc_t *shared_vec_info;   /* used when VECDESC_FL_SHARED */
	struct vector_desc_t *next;
};

/** Interrupt handler associated data structure */
struct intr_handle_data_t {
	struct vector_desc_t *vector_desc;
	struct shared_vector_desc_t *shared_vector_desc;
};

/**
 * @brief Initializes interrupt table to its defaults
 */
void esp_intr_initialize(void);

/**
 * @brief Mark an interrupt as a shared interrupt
 *
 * This will mark a certain interrupt on the specified CPU as
 * an interrupt that can be used to hook shared interrupt handlers
 * to.
 *
 * @param intno The number of the interrupt (0-31)
 * @param cpu CPU on which the interrupt should be marked as shared (0 or 1)
 * @param is_in_iram Shared interrupt is for handlers that reside in IRAM and
 *                   the int can be left enabled while the flash cache is disabled.
 *
 * @return -EINVAL if cpu or intno is invalid
 *         0 otherwise
 */
int esp_intr_mark_shared(int intno, int cpu, bool is_in_iram);

/**
 * @brief Reserve an interrupt to be used outside of this framework
 *
 * This will mark a certain interrupt on the specified CPU as
 * reserved, not to be allocated for any reason.
 *
 * @param intno The number of the interrupt (0-31)
 * @param cpu CPU on which the interrupt should be marked as shared (0 or 1)
 *
 * @return -EINVAL if cpu or intno is invalid
 *         0 otherwise
 */
int esp_intr_reserve(int intno, int cpu);

/**
 * @brief Allocate an interrupt with the given parameters.
 *
 * This finds an interrupt that matches the restrictions as given in the flags
 * parameter, maps the given interrupt source to it and hooks up the given
 * interrupt handler (with optional argument) as well. If needed, it can return
 * a handle for the interrupt as well.
 *
 * The interrupt will always be allocated on the core that runs this function.
 *
 * If ESP_INTR_FLAG_IRAM flag is used, and handler address is not in IRAM or
 * RTC_FAST_MEM, then ESP_ERR_INVALID_ARG is returned.
 *
 * @param source The interrupt source. One of the *_INTR_SOURCE interrupt mux
 *               sources, as defined in esp-xtensa-intmux.h, or one of the internal
 *               ETS_INTERNAL_*_INTR_SOURCE sources as defined in this header.
 * @param flags An ORred mask of the ESP_INTR_FLAG_* defines. These restrict the
 *               choice of interrupts that this routine can choose from. If this value
 *               is 0, it will default to allocating a non-shared interrupt of level
 *               1, 2 or 3. If this is ESP_INTR_FLAG_SHARED, it will allocate a shared
 *               interrupt of level 1. Setting ESP_INTR_FLAG_INTRDISABLED will return
 *               from this function with the interrupt disabled.
 * @param handler The interrupt handler. Must be NULL when an interrupt of level >3
 *               is requested, because these types of interrupts aren't C-callable.
 * @param arg    Optional argument for passed to the interrupt handler
 * @param ret_handle Pointer to a struct intr_handle_data_t pointer to store a handle that can
 *               later be used to request details or free the interrupt. Can be NULL if no handle
 *               is required.
 *
 * @return -EINVAL if the combination of arguments is invalid.
 *         -ENODEV No free interrupt found with the specified flags
 *         0 otherwise
 */
int esp_intr_alloc(int source,
		int flags,
		intr_handler_t handler,
		void *arg,
		struct intr_handle_data_t **ret_handle);


/**
 * @brief Allocate an interrupt with the given parameters.
 *
 *
 * This essentially does the same as esp_intr_alloc, but allows specifying a register and mask
 * combo. For shared interrupts, the handler is only called if a read from the specified
 * register, ANDed with the mask, returns non-zero. By passing an interrupt status register
 * address and a fitting mask, this can be used to accelerate interrupt handling in the case
 * a shared interrupt is triggered; by checking the interrupt statuses first, the code can
 * decide which ISRs can be skipped
 *
 * @param source The interrupt source. One of the *_INTR_SOURCE interrupt mux
 *               sources, as defined in esp-xtensa-intmux.h, or one of the internal
 *               ETS_INTERNAL_*_INTR_SOURCE sources as defined in this header.
 * @param flags An ORred mask of the ESP_INTR_FLAG_* defines. These restrict the
 *               choice of interrupts that this routine can choose from. If this value
 *               is 0, it will default to allocating a non-shared interrupt of level
 *               1, 2 or 3. If this is ESP_INTR_FLAG_SHARED, it will allocate a shared
 *               interrupt of level 1. Setting ESP_INTR_FLAG_INTRDISABLED will return
 *               from this function with the interrupt disabled.
 * @param intrstatusreg The address of an interrupt status register
 * @param intrstatusmask A mask. If a read of address intrstatusreg has any of the bits
 *               that are 1 in the mask set, the ISR will be called. If not, it will be
 *               skipped.
 * @param handler The interrupt handler. Must be NULL when an interrupt of level >3
 *               is requested, because these types of interrupts aren't C-callable.
 * @param arg    Optional argument for passed to the interrupt handler
 * @param ret_handle Pointer to a struct intr_handle_data_t pointer to store a handle that can
 *               later be used to request details or free the interrupt. Can be NULL if no handle
 *               is required.
 *
 * @return -EINVAL if the combination of arguments is invalid.
 *         -ENODEV No free interrupt found with the specified flags
 *         0 otherwise
 */
int esp_intr_alloc_intrstatus(int source,
		int flags,
		uint32_t intrstatusreg,
		uint32_t intrstatusmask,
		intr_handler_t handler,
		void *arg,
		struct intr_handle_data_t **ret_handle);


/**
 * @brief Disable and free an interrupt.
 *
 * Use an interrupt handle to disable the interrupt and release the resources associated with it.
 * If the current core is not the core that registered this interrupt, this routine will be
 * assigned to the core that allocated this interrupt, blocking and waiting until the resource
 * is successfully released.
 *
 * @note
 * When the handler shares its source with other handlers, the interrupt status bits
 * it's responsible for should be managed properly before freeing it. See ``esp_intr_disable``
 * for more details. Please do not call this function in ``esp_ipc_call_blocking``.
 *
 * @param handle The handle, as obtained by esp_intr_alloc or esp_intr_alloc_intrstatus
 *
 * @return -EINVAL the handle is NULL
 *         0 otherwise
 */
int esp_intr_free(struct intr_handle_data_t *handle);


/**
 * @brief Get CPU number an interrupt is tied to
 *
 * @param handle The handle, as obtained by esp_intr_alloc or esp_intr_alloc_intrstatus
 *
 * @return The core number where the interrupt is allocated
 */
int esp_intr_get_cpu(struct intr_handle_data_t *handle);

/**
 * @brief Get the allocated interrupt for a certain handle
 *
 * @param handle The handle, as obtained by esp_intr_alloc or esp_intr_alloc_intrstatus
 *
 * @return The interrupt number
 */
int esp_intr_get_intno(struct intr_handle_data_t *handle);

/**
 * @brief Disable the interrupt associated with the handle
 *
 * @note
 * 1. For local interrupts (ESP_INTERNAL_* sources), this function has to be called on the
 * CPU the interrupt is allocated on. Other interrupts have no such restriction.
 * 2. When several handlers sharing a same interrupt source, interrupt status bits, which are
 * handled in the handler to be disabled, should be masked before the disabling, or handled
 * in other enabled interrupts properly. Miss of interrupt status handling will cause infinite
 * interrupt calls and finally system crash.
 *
 * @param handle The handle, as obtained by esp_intr_alloc or esp_intr_alloc_intrstatus
 *
 * @return -EINVAL if the combination of arguments is invalid.
 *         0 otherwise
 */
int esp_intr_disable(struct intr_handle_data_t *handle);

/**
 * @brief Enable the interrupt associated with the handle
 *
 * @note For local interrupts (ESP_INTERNAL_* sources), this function has to be called on the
 *       CPU the interrupt is allocated on. Other interrupts have no such restriction.
 *
 * @param handle The handle, as obtained by esp_intr_alloc or esp_intr_alloc_intrstatus
 *
 * @return -EINVAL if the combination of arguments is invalid.
 *         0 otherwise
 */
int esp_intr_enable(struct intr_handle_data_t *handle);

/**
 * @brief Set the "in IRAM" status of the handler.
 *
 * @note Does not work on shared interrupts.
 *
 * @param handle The handle, as obtained by esp_intr_alloc or esp_intr_alloc_intrstatus
 * @param is_in_iram Whether the handler associated with this handle resides in IRAM.
 *                   Handlers residing in IRAM can be called when cache is disabled.
 *
 * @return -EINVAL if the combination of arguments is invalid.
 *         0 otherwise
 */
int esp_intr_set_in_iram(struct intr_handle_data_t *handle, bool is_in_iram);

/**
 * @brief Disable interrupts that aren't specifically marked as running from IRAM
 */
void esp_intr_noniram_disable(void);


/**
 * @brief Re-enable interrupts disabled by esp_intr_noniram_disable
 */
void esp_intr_noniram_enable(void);

#endif
