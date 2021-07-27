/* Copyright (c) 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODULES_CHRE_TARGET_PLATFORM_CONDITION_VARIABLE_BASE_H_
#define MODULES_CHRE_TARGET_PLATFORM_CONDITION_VARIABLE_BASE_H_

#include <kernel.h>

namespace chre {

class ConditionVariableBase {
    protected:
	struct k_condvar condvar;
};

}  /* namespace chre */

#endif /* MODULES_CHRE_TARGET_PLATFORM_CONDITION_VARIABLE_BASE_H_ */
