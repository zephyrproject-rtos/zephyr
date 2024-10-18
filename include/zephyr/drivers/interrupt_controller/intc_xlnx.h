/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. (AMD)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTC_XLNX_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTC_XLNX_H_

#ifndef _ASMLANGUAGE
extern void xlnx_intc_irq_enable(uint32_t irq);
extern void xlnx_intc_irq_disable(uint32_t irq);
extern void xlnx_intc_irq_acknowledge(uint32_t mask);
extern uint32_t xlnx_intc_irq_pending(void);
extern uint32_t xlnx_intc_irq_get_enabled(void);
extern uint32_t xlnx_intc_irq_pending_vector(void);
#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTC_XLNX_H_ */
