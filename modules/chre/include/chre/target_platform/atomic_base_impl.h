/* Copyright (c) 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODULES_CHRE_TARGET_PLATFORM_ATOMIC_BASE_IMPL_H_
#define MODULES_CHRE_TARGET_PLATFORM_ATOMIC_BASE_IMPL_H_

#include <sys/atomic.h>

#include "chre/platform/atomic.h"

namespace chre {

inline AtomicBool::AtomicBool(bool starting_value)
{
	value = ATOMIC_INIT(starting_value);
}

inline bool AtomicBool::operator=(bool desired) {
  return atomic_set(&value, desired);
}

inline bool AtomicBool::load() const {
  return atomic_get(&value);
}

inline void AtomicBool::store(bool desired) {
  atomic_set(&value, desired);
}

inline bool AtomicBool::exchange(bool desired) {
  return atomic_set(&value, desired);
}

inline AtomicUint32::AtomicUint32(uint32_t starting_value) {
  value = ATOMIC_INIT(starting_value);
}

inline uint32_t AtomicUint32::operator=(uint32_t desired) {
  return atomic_set(&value, desired);
}

inline uint32_t AtomicUint32::load() const {
  return atomic_get(&value);
}

inline void AtomicUint32::store(uint32_t desired) {
  atomic_set(&value, desired);
}

inline uint32_t AtomicUint32::exchange(uint32_t desired) {
  return atomic_set(&value, desired);
}

inline uint32_t AtomicUint32::fetch_add(uint32_t arg) {
  return atomic_add(&value, arg);
}

inline uint32_t AtomicUint32::fetch_increment() {
  return atomic_inc(&value);
}

inline uint32_t AtomicUint32::fetch_sub(uint32_t arg) {
  return atomic_sub(&value, arg);
}

inline uint32_t AtomicUint32::fetch_decrement() {
  return atomic_dec(&value);
}

}  /* namespace chre */

#endif /* MODULES_CHRE_TARGET_PLATFORM_ATOMIC_BASE_IMPL_H_ */
