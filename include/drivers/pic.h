/* pic.h - public Intel 8259 PIC APIs */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __INCpich
#define __INCpich

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_PIC_DISABLE)

/* programmable interrupt controller info (pair of cascaded 8259A devices) */

#define PIC_MASTER_BASE_ADRS 0x20
#define PIC_SLAVE_BASE_ADRS 0xa0
#define PIC_REG_ADDR_INTERVAL 1

/* register definitions */
#define PIC_ADRS(baseAdrs, reg) (baseAdrs + (reg * PIC_REG_ADDR_INTERVAL))

#define PIC_PORT2(base) PIC_ADRS(base, 0x01) /* port 2 */

#ifndef _ASMLANGUAGE

extern int _i8259_init(struct device *unused);

#endif /* _ASMLANGUAGE */

#endif /* CONFIG_PIC_DISABLE */

#ifdef __cplusplus
}
#endif

#endif /* __INCpich */
