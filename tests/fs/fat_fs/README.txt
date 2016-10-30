Title: Zephyr File System Demo

Description:

Demonstrates basic file and dir operations using the Zephyr file system.
--------------------------------------------------------------------------------

Building and Running Project:

The demo will run on Arduino 101 and will use the on-board SPI flash.

    make BOARD=arduino_101

--------------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

--------------------------------------------------------------------------------

Sample Output:

File System Demo!

Creating new file testfile.txt
Opened file testfile.txt
Data successfully written!
Data written:"hello world!"

Data successfully read!
Data read:"hello world!"

Data read matches data written!

Truncate tests:
Testing shrink to 0 size
Testing write after truncating
Data successfully written!
Data written:"hello world!"

Original size of file = 12
File size after shrinking by 5 bytes = 7
Check original contents after shrinking file
Data successfully read!
Data read:"hello w"

File size after expanding by 10 bytes = 17
Check original contents after expanding file
Data successfully read!
Data read:"hello w"

Testing for zeroes in expanded region
Closed file testfile.txt
File (testfile.txt) deleted successfully!
Created dir sub1!
Creating new file testfile.txt
Opened file testfile.txt
Creating new file sub1/testfile.txt
Opened file sub1/testfile.txt
Data successfully written!
Data written:"1"

Data successfully written!
Data written:"12"

Closed file testfile.txt
Closed file sub1/testfile.txt

Listing dir /:
[DIR ] SUB1
[FILE] TESTFILE.TXT (size = 1)

Listing dir sub1:
[FILE] TESTFILE.TXT (size = 2)

Removing files and sub directories in sub1
Removing sub1/TESTFILE.TXT
Removed dir sub1!

Optimal transfer block size   = 512
Allocation unit size          = 512
Volume size in f_frsize units = 152
Free space in f_frsize units  = 151
