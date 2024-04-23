/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/linker/iterable_sections.h>

#if defined(CONFIG_ADC)
	ITERABLE_SECTION_ROM(adc_driver_api, Z_LINK_ITERABLE_SUBALIGN)
#endif

#if defined(CONFIG_RTC)
	ITERABLE_SECTION_ROM(rtc_driver_api, Z_LINK_ITERABLE_SUBALIGN)
#endif
