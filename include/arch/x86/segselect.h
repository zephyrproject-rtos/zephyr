/* segselect.h - IA-32 segment selector header */

/*
 * Copyright (c) 2012-2014, Wind River Systems, Inc.
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

/*
DESCRIPTION
This header contains the IA-32 segment selector defintions. These are extracted
into their own file so they can be shared with the host tools.
 */

#ifndef _SEGSELECT_H
#define _SEGSELECT_H

/*
 * Segment selector macros for the various entries in the GDT.  These are
 * actually byte offsets from the start of the entry table (tGdtHeader->pEntries)
 * that are loaded into the appropriate CPU segment register (along with the
 * appropriate T bit and privilege level bits).
 *
 * When referencing a specific GDT descriptor via tGdtHeader->pEntries, use
 * XXX_SEG_SELECTOR/sizeof(tGdtDesc)
 */

#define NULL_SEG_SELECTOR		0
#define KERNEL_CODE_SEG_SELECTOR	0x0008
#define KERNEL_DATA_SEG_SELECTOR	0x0010

#endif /* !_SEGSELECT_H */
