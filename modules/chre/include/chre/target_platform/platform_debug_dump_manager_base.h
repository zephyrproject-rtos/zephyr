/* Copyright (c) 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODULES_CHRE_TARGET_PLATFORM_PLATFORM_DEBUG_DUMP_MANAGER_BASE_H_
#define MODULES_CHRE_TARGET_PLATFORM_PLATFORM_DEBUG_DUMP_MANAGER_BASE_H_

namespace chre {

/**
 * @brief Platform specific debug dump manager.
 */
class PlatformDebugDumpManagerBase {
    protected:
	static constexpr size_t kDebugDumpStrMaxSize = CHRE_MESSAGE_TO_HOST_MAX_SIZE;
};

}  /* namespace chre */

#endif /* MODULES_CHRE_TARGET_PLATFORM_PLATFORM_DEBUG_DUMP_MANAGER_BASE_H_ */
