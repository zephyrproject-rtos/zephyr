# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2025, Siemens AG

# This script creates a FAT-formatted test disk image of the specified
# size, in kilo bytes, given as the argument.

import sys

from pyfatfs.PyFat import PyFat
from pyfatfs.PyFatFS import PyFatFS

disk_image_path = sys.argv[1]
disk_size = int(sys.argv[2]) * 1024

# Allocate the disk space for the disk image file.
with open(disk_image_path, 'wb') as f:
    f.seek(disk_size - 1)
    f.write(b'\x00')

# Format the disk image with the FAT file system.
pf = PyFat()
pf.mkfs(disk_image_path, fat_type=PyFat.FAT_TYPE_FAT12, size=disk_size)
pf.close()

# Populate the disk image with a test file.
pfs = PyFatFS(disk_image_path)
f = pfs.openbin('test.txt', 'wb')
f.write(b'This is an existing test file.')
f.close()
pfs.close()
