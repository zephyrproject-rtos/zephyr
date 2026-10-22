DMIC API Test
##################

This test is designed to verify that DMIC peripherals implement the API
correctly. It performs the following checks:

* Verify the DMIC will not start sampling before it is configured

* Verify the DMIC can sample from one left channel

* Verify the DMIC can sample from a stereo L/R pair

* Verify that the DMIC works with the maximum number of channels possible
  (defined based on the DTS compatible present in the build)

* Verify that the DMIC can restart sampling after being paused and resumed

* Verify that invalid channel maps (R/R pair, non-adjacent channels) are
  rejected by the DMIC driver.
