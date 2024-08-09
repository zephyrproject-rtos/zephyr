/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef __SOC_H_
#define __SOC_H_

#include <stdint.h>
#include "layout.h"

#ifdef CONFIG_XLNX_INTC

extern void xlnx_intc_irq_enable(uint32_t irq);
extern void xlnx_intc_irq_disable(uint32_t irq);
extern void xlnx_intc_irq_acknowledge(uint32_t mask);
extern uint32_t xlnx_intc_irq_pending(void);
extern uint32_t xlnx_intc_irq_get_enabled(void);
extern uint32_t xlnx_intc_irq_pending_vector(void);

#endif /* CONFIG_XLNX_INTC */

#endif /* __SOC_H_ */
