# This section must be loaded first of all the
# custom sections because we want it to be placed
# at the top address of RAM.
SECTION_PROLOGUE(NRF_KMU_RESERVED_PUSH_SECTION,(NOLOAD) ,)
{
    __nrf_kmu_reserved_push_area = .;
    *(.nrf_kmu_reserved_push_area)
    __nrf_kmu_reserved_push_area_end = .;
} GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

# It doesn't seem to be possible to enforce placing a section
# at a specific address in memory using the Zephyr SECTION macros.
# So this assert is necessary to avoid accidentatly moving this
# section to a different address.
ASSERT(__nrf_kmu_reserved_push_area == RAM_ADDR, "Error:  \
 The section NRF_KMU_RESERVED_PUSH_SECTION needs to be    \
 placed on the top RAM address but it is not, please edit \
 your linker scripts to make sure that it is placed on    \
 the to RAM address.")
