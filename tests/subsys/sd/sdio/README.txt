SDIO Subsystem Test
##################

This test is designed to verify the SD subsystem stack implementation for SDIO
devices. Note that this test will only perform basic reads from the SDIO card
after initialization, as most registers present on the card will be vendor
specific. It requires an SDIO card be connected to the board to pass.
The test has the following phases:

* Init test: verify the SD host controller can detect card presence, and
  test the initialization flow of the SDIO subsystem to verify that the stack
  can correctly initialize an SDIO card.

* Read test: verify that a read from the SDIO common card register area returns
  valid data.

* Configuration test: verify that the SD stack reports a valid configuration
  for this card after initialization.
