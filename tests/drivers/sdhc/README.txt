SDHC API Test
##################

This test is designed to verify the functionality of a device implementing the
SD host controller API. It requires that an SD card be present on the SD bus
to pass. The test has the following phases:

* Reset test: Verify the SDHC can successfully reset the host controller state.
  This primarily tests that the driver returns zero for this call, although if
  the reset left the SDHC in a bad state subsequent tests may fail.

* Host props: Get host properties structure from SDHC. This verifies that
  the API returns a valid host property structure (at a minimum, the driver
  must initialize all fields of the structure to zero.)

* Set_IO test: Verify that the SDHC will reject clock frequencies outside of
  the frequency range it claims to support via sdhc_get_host_props.

* Card presence test. Verify that the SDHC detects card presence.

* Request test: Make a request to read the card interface condition,
  and verify that valid data is returned.

Note that this test does not verify the tuning or card busy api, as the SD
specification is state based, and testing these portions of the SDHC would
require implementing a large portion of the SD subsystem in this test.
