# SPDX-License-Identifier: Apache-2.0

string(REPLACE "nsim_" "" NSIM_ARC_CLASSIC_SER "${SOC_SERIES}")
add_subdirectory(${NSIM_ARC_CLASSIC_SER})

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
