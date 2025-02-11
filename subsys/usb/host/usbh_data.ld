#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_RAM(usbh_contex, Z_LINK_ITERABLE_SUBALIGN)
ITERABLE_SECTION_RAM(usbh_class_data, Z_LINK_ITERABLE_SUBALIGN)
