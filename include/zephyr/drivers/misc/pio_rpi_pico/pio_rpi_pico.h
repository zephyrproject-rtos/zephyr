/*
 * Copyright (c) 2023 Ionut Pavel <iocapa@iocapa.com>
 * Copyright (c) 2023 Tokita, Hiroshi <tokita.hiroshi@fujitsu.com>
 * Copyright (c) 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_PIO_RPI_PICO_H_
#define ZEPHYR_DRIVERS_MISC_PIO_RPI_PICO_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/devicetree/gpio.h>
#include <zephyr/sys/slist.h>

/**
 * @brief Get the clock frequency of the associated PIO instance
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	pio {
 *		status = "okay";
 *		clocks = <&system_clk>; // 125000000
 *		c: child {};
 *	};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	// child.c
 *	DT_PIO_RPI_PICO_CLOCK_FREQ_HZ(node, 0) // 125000000
 * @endcode
 *
 * @param node_id node identifier
 * @return the parent's clock frequency
 */
#define DT_PIO_RPI_PICO_CLOCK_FREQ_HZ(node_id)							\
	DT_PROP(DT_CLOCKS_CTLR(DT_PARENT(node_id)), clock_frequency)

/**
 * @brief Get the clock frequency of the associated PIO instance
 *
 * @param inst instance number
 * @return the parent's clock frequency
 *
 * @see DT_PIO_RPI_PICO_CLOCK_FREQ_HZ
 */
#define DT_INST_PIO_RPI_PICO_CLOCK_FREQ_HZ(inst)						\
	DT_PIO_RPI_PICO_CLOCK_FREQ_HZ(DT_DRV_INST(inst))

/**
 * @brief Get a pin number from a pinctrl / group name and index
 *
 * Example devicetree fragment(s):
 *
 * @code{.dts}
 *	pinctrl {
 *		pio_child_default: pio_child_default {
 *			tx_gpio {
 *				pinmux = <PIO0_P0>, <PIO0_P2>;
 *			};
 *
 *			rx_gpio {
 *				pinmux = <PIO0_P1>;
 *				input-enable;
 *			};
 *		};
 *	};
 * @endcode
 *
 * @code{.dts}
 *	pio {
 *		status = "okay";
 *		c: child {
 *			pinctrl-0 = <&pio_child_default>;
 *			pinctrl-names = "default";
 *		};
 *	};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	DT_PIO_RPI_PICO_PIN_BY_NAME(node, default, 0, tx_gpio, 0) // 0
 *	DT_PIO_RPI_PICO_PIN_BY_NAME(node, default, 0, tx_gpio, 1) // 2
 *	DT_PIO_RPI_PICO_PIN_BY_NAME(node, default, 0, rx_gpio, 0) // 1
 * @endcode
 *
 * @param node_id node identifier
 * @param p_name pinctrl name
 * @param p_idx pinctrl index
 * @param g_name group name
 * @param g_idx group index
 * @return pin number
 */
#define DT_PIO_RPI_PICO_PIN_BY_NAME(node_id, p_name, p_idx, g_name, g_idx)			\
	RP2_GET_PIN_NUM(DT_PROP_BY_IDX(								\
		DT_CHILD(DT_PINCTRL_BY_NAME(node_id, p_name, p_idx), g_name), pinmux, g_idx))

/**
 * @brief Get a pin number from a pinctrl / group name and index
 *
 * @param inst instance number
 * @param p_name pinctrl name
 * @param p_idx pinctrl index
 * @param g_name group name
 * @param g_idx group index
 * @return pin number
 *
 * @see DT_PIO_RPI_PICO_PIN_BY_NAME
 */
#define DT_INST_PIO_RPI_PICO_PIN_BY_NAME(inst, p_name, p_idx, g_name, g_idx)			\
	DT_PIO_RPI_PICO_PIN_BY_NAME(DT_DRV_INST(inst), p_name, p_idx, g_name, g_idx)

/**
 * @brief Get assigned PIO base address
 *
 * @param node_id node identifier
 * @return base address
 */
#define DT_PIO_RPI_PICO_REG_ADDR(node_id)							\
	DT_REG_ADDR(DT_PARENT(node_id))

/**
 * @brief Get assigned PIO base address
 *
 * @param inst instance number
 * @return base address
 *
 * @see DT_PIO_RPI_PICO_REG_ADDR
 */
#define DT_INST_PIO_RPI_PICO_REG_ADDR(inst)							\
	DT_PIO_RPI_PICO_REG_ADDR(DT_DRV_INST(inst))

/**
 * @brief PIO shared IRQ handler type
 *
 * @param dev Pointer to registered device
 */
typedef void (*pio_rpi_pico_irq_t)(const struct device *dev);

/**
 * @brief PIO shared IRQ configuration data
 */
struct pio_rpi_pico_irq_cfg {
	/* Device node */
	sys_snode_t node;

	/* IRQ callback */
	pio_rpi_pico_irq_t irq_func;

	/* Callback parameter */
	const void *irq_param;

	/* Shared IRQ index */
	uint8_t irq_idx;

	/* IRQ status */
	bool enabled;
};

/**
 * @brief Register a shared IRQ
 *
 * @param dev Pointer to the parent device
 * @param cfg Pointer to the IRQ configuration
 */
void pio_rpi_pico_irq_register(const struct device *dev, struct pio_rpi_pico_irq_cfg *cfg);

/**
 * @brief Enable a shared IRQ
 *
 * @param dev Pointer to the parent device
 * @param cfg Pointer to the IRQ configuration
 */
void pio_rpi_pico_irq_enable(const struct device *dev, struct pio_rpi_pico_irq_cfg *cfg);

/**
 * @brief Disable a shared IRQ
 *
 * @param dev Pointer to the parent device
 * @param cfg Pointer to the IRQ configuration
 */
void pio_rpi_pico_irq_disable(const struct device *dev, struct pio_rpi_pico_irq_cfg *cfg);

/**
 * @brief Dynamic state machine allocator
 *
 * @param dev Pointer to the PIO device structure
 * @param count State machines to allocate
 * @param sm Output pointer for the first allocated state machine
 * @return -EIO on error
 */
int pio_rpi_pico_alloc_sm(const struct device *dev, size_t count, uint8_t *sm);

/**
 * @brief Dynamic instruction allocator
 *
 * @param dev Pointer to the PIO device structure
 * @param count Instructions to allocate
 * @param instr Output pointer for the first allocated instruction
 * @return -ENOMEM on error
 */
int pio_rpi_pico_alloc_instr(const struct device *dev, size_t count, uint8_t *instr);

/**
 * @brief Dynamic shared instruction allocator
 *
 * @param dev Pointer to the PIO device structure
 * @param key String key to identify the shared section
 * @param count Instructions to allocate
 * @param instr Output pointer for the first allocated instruction
 * @return -ENOMEM on error
 * @return -EALREADY if the section was already allocated
 *
 * @note in case of -EALREADY, instr will contain the previous allocated instruction
 */
int pio_rpi_pico_alloc_shared_instr(const struct device *dev, const char *key,
				    size_t count, uint8_t *instr);

#endif /* ZEPHYR_DRIVERS_MISC_PIO_RPI_PICO_H_ */
