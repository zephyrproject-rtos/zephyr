# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/toolchain/zephyr/${SDK_VERSION}/target.cmake)

set(DETERMINISTIC_AR_OPTION "-D" CACHE STRING
  "Makes static libraries deterministic, see 'ar' documentation")
