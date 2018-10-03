# The nrf52810_pca10040 board mirrors the nRF52832 DK hardware. This
# needs to be considered by certain system initialization functionality
# residing in system_nrf52810.c.
zephyr_compile_definitions(DEVELOP_IN_NRF52832)
