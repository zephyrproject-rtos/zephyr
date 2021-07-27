/* Copyright (c) 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODULES_CHRE_TARGET_PLATFORM_MUTEX_BASE_IMPL_H_
#define MODULES_CHRE_TARGET_PLATFORM_MUTEX_BASE_IMPL_H_

#include "chre/platform/mutex.h"

namespace chre {
inline Mutex::Mutex()
{
	k_mutex_init(&mutex);
}

inline Mutex::~Mutex()
{
}

inline void Mutex::lock()
{
	k_mutex_lock(&mutex, K_FOREVER);
}

inline bool Mutex::try_lock()
{
	return (k_mutex_lock(&mutex, K_NO_WAIT) == 0);
}

inline void Mutex::unlock()
{
	k_mutex_unlock(&mutex);
}

} /* namespace chre */
#endif /* MODULES_CHRE_TARGET_PLATFORM_MUTEX_BASE_IMPL_H_ */
