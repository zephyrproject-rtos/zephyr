 /*
  * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
  * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Link ".second_section" into the second partition */
GROUP_START(second_region)

SECTION_DATA_PROLOGUE(second_section,,)
{
	KEEP(*(".second_section"));
	KEEP(*(".second_section.*"));
} GROUP_LINK_IN(second_region)

GROUP_END(second_region)
