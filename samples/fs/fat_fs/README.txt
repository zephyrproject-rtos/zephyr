Title: Zephyr File System Demo

Description:

Demonstrates basic file and dir operations using the Zephyr file system.
--------------------------------------------------------------------------------

Building and Running Project:

While this demo uses RAM to emulate storage, it can be tested using QEMU.

Following command will build it for running on QEMU:

    make qemu

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

Closed file testfile.txt
File (testfile.txt) deleted successfully!
Created dir sub1!
Created dir sub2!
Created dir sub1/sub3!
Creating new file testfile.txt
Opened file testfile.txt
Creating new file sub1/testfile.txt
Opened file sub1/testfile.txt
Creating new file sub2/testfile.txt
Opened file sub2/testfile.txt
Creating new file sub1/sub3/file1.txt
Opened file sub1/sub3/file1.txt
Creating new file sub1/sub3/file2.txt
Opened file sub1/sub3/file2.txt
Data successfully written!
Data written:"1"

Data successfully written!
Data written:"12"

Data successfully written!
Data written:"123"

Data successfully written!
Data written:"1234"

Data successfully written!
Data written:"12345"

Closed file testfile.txt
Closed file sub1/testfile.txt
Closed file sub2/testfile.txt
Closed file sub1/sub3/file1.txt
Closed file sub1/sub3/file2.txt

Listing dir /:
[DIR ] SUB1
[DIR ] SUB2
[FILE] TESTFILE.TXT (size = 1)

Listing dir sub1:
[DIR ] SUB3
[FILE] TESTFILE.TXT (size = 2)

Listing dir sub2:
[FILE] TESTFILE.TXT (size = 3)

Listing dir sub1/sub3:
[FILE] FILE1.TXT (size = 4)
[FILE] FILE2.TXT (size = 5)
