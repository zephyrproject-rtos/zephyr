 /*
  * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
  * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Link ".first_section" into the first partition */
GROUP_START(first_region)

SECTION_DATA_PROLOGUE(first_section,,)
{
	KEEP(*(".first_section"));
	KEEP(*(".first_section.*"));
} GROUP_LINK_IN(first_region)

GROUP_END(first_region)
