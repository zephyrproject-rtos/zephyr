#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_RAM(test_ram, 4)
ITERABLE_SECTION_RAM(test_ram2, 4)
ITERABLE_SECTION_RAM(test_ram_named, 4)
ITERABLE_SECTION_RAM_NUMERIC(test_ram_numeric, 4)
