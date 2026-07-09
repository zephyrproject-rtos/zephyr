Flash Preload testing
#####################

This test is designed to verify that data is preloaded in the flash, at a configurable
offset. It is primarily intended to verify that semihosted flash can be preloaded
with data, but can also be used for physical hardware.

The test expects to find the ascii string "flash_data" at the start of the
``storage`` partition.
