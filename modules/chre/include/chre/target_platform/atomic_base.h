/* Copyright (c) 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODULES_CHRE_TARGET_PLATFORM_ATOMIC_BASE_H_
#define MODULES_CHRE_TARGET_PLATFORM_ATOMIC_BASE_H_

namespace chre {

class AtomicBase {
    protected:
	atomic_t value;
};

typedef AtomicBase AtomicBoolBase;
typedef AtomicBase AtomicUint32Base;

}  /* namespace chre */

#endif /* MODULES_CHRE_TARGET_PLATFORM_ATOMIC_BASE_H_ */
