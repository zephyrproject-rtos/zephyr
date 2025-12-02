MMC Subsystem Test
##################

This test requires 2 SDIO instances on mcu and 2 eMMCs on board. The test is
designed to verify the simultaneous transmission of large amounts of data across
2 MMC devices on 2 SDIO instances. 2 SD host controller drivers must be implemented
in device tree, this test also serves as a complete test for the SDHC driver
implementation in use. The test has the following phases:

* MMC multiple test: Created two MMC threads. One thread tested writing large amounts of data
  from SDHC0 to the eMMC card, verifying that the data could be read back without errors.
  The other thread ran the same test on SDHC1. This test can not only test the two SDIO
  instances with different block counts, addresses, and widths, but also ensure that
  the two threads can synchronize during the read and write process.
