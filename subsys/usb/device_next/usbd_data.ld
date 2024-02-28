#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_RAM(usbd_contex, Z_LINK_ITERABLE_SUBALIGN)
ITERABLE_SECTION_RAM(usbd_class_iter, Z_LINK_ITERABLE_SUBALIGN)
