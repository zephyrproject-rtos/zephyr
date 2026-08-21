# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Build the Secure image (secure_s) before the NS main image so that
# libentryveneers.a is available when the NS image links.
#
# In sysbuild context BOARD is the base board name and BOARD_QUALIFIERS holds
# the remaining path elements (e.g. "pse846gps2dbzc4a/m33/ns").  Strip the
# trailing "/ns" from BOARD_QUALIFIERS to derive the Secure board.
# E.g. BOARD=kit_pse84_eval, BOARD_QUALIFIERS=pse846gps2dbzc4a/m33/ns
#   → SECURE_BOARD=kit_pse84_eval/pse846gps2dbzc4a/m33

if(DEFINED BOARD_QUALIFIERS AND "${BOARD_QUALIFIERS}" MATCHES "/ns$")
    string(REGEX REPLACE "/ns$" "" _secure_qualifiers "${BOARD_QUALIFIERS}")
    if(_secure_qualifiers)
        set(SECURE_BOARD "${BOARD}/${_secure_qualifiers}")
    else()
        set(SECURE_BOARD "${BOARD}")
    endif()
else()
    # Board has no /ns qualifier — fall back to the AN521 Secure CPU (QEMU).
    set(SECURE_BOARD "mps2/an521/cpu0")
endif()

ExternalZephyrProject_Add(
    APPLICATION secure_s
    SOURCE_DIR  ${APP_DIR}/../tz_s
    BOARD       ${SECURE_BOARD}
)

# NS image depends on S image for the veneer library.
add_dependencies(${DEFAULT_IMAGE} secure_s)
sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} secure_s)
