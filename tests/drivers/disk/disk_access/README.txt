Disk Access Test
##################

This test is intended to verify the functionality of disk devices in Zephyr.
It is designed to test the NXP USDHC disk driver, but can be used for other
disk devices as well. The test has the following phases:

* Setup test: Verifies that disk initialization works, as well as testing
  disk_access_ioctl by querying the disk for its sector size and sector count.
  Note that this test also verifies the memory buffers reserved for read/write
  tests are sufficiently large, and will fail if they are not (in which case
  the value of SECTOR_SIZE must be increased)

* Read test: Verifies that the driver can consistently read sectors. This test
  starts by reading sectors from a variety of start locations. Each location is
  read from several times, each time with a different number of desired sectors.
  The test deliberately will read sectors beyond the end of the disk, and if
  the driver does not reject this read request the tests will fail. Following
  these sector reads, the driver will read multiple times from the same memory
  location, to verify that the data being returned is the same.

* Write test: Verifies that the driver can consistently write sectors. This test
  follows the same flow as the read test, but at each step writes data to the
  disk and reads it back to verify correctness. The test first performs writes
  of various length to various sectors (once again, the driver must reject
  writes that would be outside the bounds of the disk), then performs multiple
  writes to the same location.
