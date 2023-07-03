#include <zephyr/linker/iterable_sections.h>

ITERABLE_SECTION_ROM(test_rom, 4)
ITERABLE_SECTION_ROM(test_rom2, 4)
ITERABLE_SECTION_ROM(test_rom_named, 4)
ITERABLE_SECTION_ROM_NUMERIC(test_rom_numeric, 4)
