Title: Zephyr File System Demo

Description:

Demonstrates basic file and dir operations using the Zephyr file system.
--------------------------------------------------------------------------------

Building and Running Project:

The demo will run on native_posix and will use the on-board SPI flash.

       mkdir build; cd build
       cmake -DBOARD=native_posix ..
       make run

To test fatfs on MMC, add this cmake option to your build command:
	-DCONF_FILE=prj_mmc.conf
--------------------------------------------------------------------------------

Sample Output:

***** BOOTING ZEPHYR OS v1.6.99 - BUILD: Feb  8 2017 07:33:07 *****
Running test suite fat_fs_basic_test
tc_start() - test_fat_file

Open tests:
Creating new file testfile.txt
Opened file testfile.txt

Write tests:
Data written:"hello world!"

Data successfully written!

Sync tests:

Read tests:
Data read:"hello world!"

Data read matches data written

Truncate tests:

Testing shrink to 0 size
Testing write after truncating

Write tests:
Data written:"hello world!"

Data successfully written!
Original size of file = 12

Testing shrinking
File size after shrinking by 5 bytes = 7

Testing expanding
File size after expanding by 10 bytes = 17
Testing for zeroes in expanded region

Close tests:
Closed file testfile.txt

Delete tests:
File (testfile.txt) deleted successfully!
===================================================================
PASS - test_fat_file.
tc_start() - test_fat_dir

mkdir tests:
Creating new dir testdir

Write tests:
Data written:"hello world!"

Data successfully written!
Created dir testdir!

lsdir tests:

Listing dir /:
[DIR ] FATFILE
[FILE] TEST.C (size = 10)
[DIR ] TESTDIR
[DIR ] 1

lsdir tests:

Listing dir testdir:
[FILE] TESTFILE.TXT (size = 12)

rmdir tests:

Removing testdir/TESTFILE.TXT
Removed dir testdir!

lsdir tests:

Listing dir /:
[DIR ] FATFILE
[FILE] TEST.C (size = 10)
[DIR ] 1

===================================================================
PASS - test_fat_dir.
tc_start() - test_fat_fs

Optimal transfer block size   = 512
Allocation unit size          = 1024
Volume size in f_frsize units = 2028
Free space in f_frsize units  = 2025
===================================================================
PASS - test_fat_fs.
===================================================================
PROJECT EXECUTION SUCCESSFUL
