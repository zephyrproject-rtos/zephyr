UFS Subsystem Test
##################

This test is designed to verify the UFS subsystem stack implementation for UFS
devices. Note that this test will only perform basic reads from the UFS card
after initialization. It requires an UFS card be connected to the board to pass.

* Init test: verify the UFS host controller can detect card presence, and
  test the initialization flow of the UFS subsystem to verify that the stack
  can correctly initialize an UFS card.

* Configuration test: Verify that the UFS stack reports a valid configuration
  for UFS card after initialization.
