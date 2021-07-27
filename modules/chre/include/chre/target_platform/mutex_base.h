/* Copyright (c) 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODULES_CHRE_TARGET_PLATFORM_MUTEX_BASE_H_
#define MODULES_CHRE_TARGET_PLATFORM_MUTEX_BASE_H_

namespace chre {

#include <sys/mutex.h>

/**
 * @brief Platform implementation of a mutex.
 */
class MutexBase {
    protected:
	struct k_mutex mutex;
	friend class ConditionVariable;
};

}  /* namespace chre */

#endif /* MODULES_CHRE_TARGET_PLATFORM_MUTEX_BASE_H_ */
