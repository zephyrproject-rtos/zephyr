SCSI-UFS Subsystem Test
#######################

The Test is designed to verify the SCSI subsystem stack implementation for UFS
devices. It requires a UFS Device be connected to the board to pass.

* Init test: Initialize the SCSI Device handle through UFS host controller.

* IOCTL test: Verify the SCSI IOCTL operations based on SG_IO.

Warning: Ensure that you have configured the test to allow writes to the UFS
device. By default, test prevents write to avoid data corruption. Please refer
to the configuration options in testcase to enable write operations if needed.

* R/W Test: Write data to the SCSI Device and verify that it can be read back
without errors.
