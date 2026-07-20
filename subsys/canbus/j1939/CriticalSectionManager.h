/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CRITICAL_SECTION_MANAGER_H_
#define CRITICAL_SECTION_MANAGER_H_

/*
 * The imported stack expects global critical section hooks.
 * This compatibility layer keeps them as no-ops by default.
 */
void CriticalSection_Lock(void);
void CriticalSection_Unlock(void);

#endif /* CRITICAL_SECTION_MANAGER_H_ */
