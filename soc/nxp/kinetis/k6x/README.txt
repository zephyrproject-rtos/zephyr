Notes on the FSL FRDM K64F SRAM base address and size

Although the K64F CPU has 64 kB of SRAM at 0x1FFF0000 (code space), it is not
used by the FSL FRDM K64F platform.  Only the 192 kB region based at the
standard ARMv7-M SRAM base address of 0x20000000 is supported.

As such the following values are used:

CONFIG_SRAM_BASE_ADDRESS=0x20000000
CONFIG_SRAM_SIZE=64      # Measured in kB
