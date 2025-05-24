# iterables.ld
#include <zephyr/linker/iterable_sections.h>
ITERABLE_SECTION_RAM(stepper_event_handler, Z_LINK_ITERABLE_SUBALIGN)
