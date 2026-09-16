/*
 * SPDX-FileCopyrightText: 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _VECTOR_HIJACK_H_
#define _VECTOR_HIJACK_H_

/* Makes a copy of the vector table in writable RAM (it's generally in
 * a ROM section), redirects it, and hooks the SVC interrupt with our
 * own code above so we can catch direct interrupts.
 */
void *vector_hijack(void (*my_svc_handler)(void));

#endif /* _VECTOR_HIJACK_H_ */
