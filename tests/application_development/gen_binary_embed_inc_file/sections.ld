/*
 * Copyright (c) 2025 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

test_section :
{
    __test_section_start = .;

    KEEP (*(.test_section*))

    __test_section_end = .;

} GROUP_LINK_IN(ROMABLE_REGION)

test_rw_section :
{
    __test_rw_section_start = .;

    KEEP (*(.test_rw_section*))

    __test_rw_section_end = .;

} GROUP_LINK_IN(RAMABLE_REGION)
