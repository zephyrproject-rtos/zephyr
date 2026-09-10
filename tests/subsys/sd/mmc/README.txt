MMC Subsystem Test
##################

This test is designed to verify the MMC protocol stack implementation,
and run stress tests to verify large data transfers succeed using the
subsystem. Due to the differences between underlying SD host controller drivers,
this test also serves as a complete test for the SDHC driver implementation in
use. It requires an SD card be connected to the board to pass, and will
perform destructive I/O on the card, wiping any data present. The test has
the following phases:

* Init test: verify the SD host controller can detect card presence, and
  test the initialization flow of the MMC subsystem to verify that the stack
  can correctly initialize an SD card.

* IOCTL test: verify the SD subsystem correctly implements IOCTL calls required
  for block devices in Zephyr.

* Read test: verify that single block reads work, followed by multiple
  block reads. Ensure the subsystem will reject reads beyond the end of
  the card's stated size.

* Write test: verify that single block writes work, followed by multiple
  block writes. Ensure the subsystem will reject writes beyond the end of
  the card's stated size.

* R/W test: write data to the MMC card, and verify that it is able
  to be read back without error. Perform this R/W combination at several
  sector locations across the MMC card.
