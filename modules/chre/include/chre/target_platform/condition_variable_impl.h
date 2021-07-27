/* Copyright (c) 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODULES_CHRE_TARGET_PLATFORM_CONDITION_VARIABLE_BASE_H_
#define MODULES_CHRE_TARGET_PLATFORM_CONDITION_VARIABLE_BASE_H_

#include "chre/platform/condition_variable.h"

namespace chre
{
inline ConditionVariable::ConditionVariable()
{
	k_condvar_init(&condvar);
}

inline ConditionVariable::~ConditionVariable()
{
}

inline void ConditionVariable::notify_one()
{
	k_condvar_signal(&condvar);
}

inline void ConditionVariable::wait(Mutex &mutex)
{
	k_condvar_wait(&condvar, mutex.mutex, K_FOREVER);
}

inline bool ConditionVariable::wait_for(Mutex &mutex, Nanoseconds timeout)
{
	return (k_condvar_wait(&condvar, mutex.mutex, K_NSEC(timeout)) == 0);
}

} /* namespace chre */

#endif /* MODULES_CHRE_TARGET_PLATFORM_CONDITION_VARIABLE_BASE_H_ */
