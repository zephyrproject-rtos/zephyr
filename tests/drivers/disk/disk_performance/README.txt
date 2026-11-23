Disk Performance Test
##################

This test is intended to test the performance of disk devices under Zephyr. It
was tested with SD cards, but can be used for other disk devices as well.
The test has the following phases:

* Setup test: simply sets up the disk, and reads data such as the sector count
  and sector size

* Sequential read test: This test performs sequential reads, first only over one
  sector, than over multiple sequential sectors.

* Random read test: This test performs random reads across the disk, each one
  sector in length.

* Sequential write test: This test performs sequential writes, first only over
  one sector, than over multiple sequential sectors.

* Random write test: This test performs random writes across the disk, each one
  sector in length
