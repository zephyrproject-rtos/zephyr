Title: Safe Memory Access

Description:

This test verifies that the safe memory access (_mem_safe) functions as
intended.

---------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make qemu

---------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

---------------------------------------------------------------------------

Sample Output:

testing SUCCESS of READ on RO memory with width 1.......PASS
testing SUCCESS of READ on RO memory with width 2.......PASS
testing SUCCESS of READ on RO memory with width 4.......PASS
testing FAILURE of WRITE on RO memory with width 1.......PASS
testing FAILURE of WRITE on RO memory with width 2.......PASS
testing FAILURE of WRITE on RO memory with width 4.......PASS
testing SUCCESS of READ on RW memory with width 1.......PASS
testing SUCCESS of READ on RW memory with width 2.......PASS
testing SUCCESS of READ on RW memory with width 4.......PASS
testing SUCCESS of WRITE on RW memory with width 1.......PASS
testing SUCCESS of WRITE on RW memory with width 2.......PASS
testing SUCCESS of WRITE on RW memory with width 4.......PASS
testing FAILURE of INVALID ACCESS on RW memory with width 4.......PASS
testing FAILURE of READ on RO memory with width 0.......PASS
testing SUCCESS of READ on RO memory with width 1.......PASS
testing SUCCESS of READ on RO memory with width 2.......PASS
testing FAILURE of READ on RO memory with width 3.......PASS
testing SUCCESS of READ on RO memory with width 4.......PASS
testing FAILURE of READ on RO memory with width 5.......PASS
testing FAILURE of READ on RO memory with width 8.......PASS
testing SUCCESS of READ on RO memory with width 1.......PASS
testing SUCCESS of READ on RO memory with width 1.......PASS
testing SUCCESS of READ on RW memory with width 1.......PASS
testing SUCCESS of READ on RW memory with width 1.......PASS
testing FAILURE of WRITE on RO memory with width 1.......PASS
testing FAILURE of WRITE on RO memory with width 1.......PASS
testing SUCCESS of WRITE on RW memory with width 1.......PASS
testing SUCCESS of WRITE on RW memory with width 1.......PASS
testing FAILURE of READ on out-of-image memory with width 1.......PASS
testing FAILURE of READ on out-of-image memory with width 1.......PASS
testing SUCCESS of adding extra RO region.......PASS
testing SUCCESS of adding extra RW region.......PASS
testing FAILURE of adding extra region that won't fit.......PASS
testing SUCCESS of READ on out-of-image memory with width 1.......PASS
testing SUCCESS of READ on out-of-image memory with width 1.......PASS
testing SUCCESS of READ on out-of-image memory with width 1.......PASS
testing SUCCESS of READ on out-of-image memory with width 1.......PASS
testing FAILURE of WRITE on out-of-image memory with width 1.......PASS
testing FAILURE of WRITE on out-of-image memory with width 1.......PASS
testing SUCCESS of WRITE on out-of-image memory with width 1.......PASS
testing SUCCESS of WRITE on out-of-image memory with width 1.......PASS
testing FAILURE of READ on out-of-image memory with width 1.......PASS
testing FAILURE of READ on out-of-image memory with width 1.......PASS
testing FAILURE of READ on out-of-image memory with width 1.......PASS
testing FAILURE of READ on out-of-image memory with width 1.......PASS
testing FAILURE of WRITE on RO memory with width 1.......PASS
testing SUCCESS of _mem_probe() reading RO values.......PASS
testing SUCCESS of _mem_probe() reading RW values.......PASS
testing SUCCESS of _mem_probe() writing values.......PASS
testing SUCCESS of _mem_safe_read(size: 16, width: 0).......PASS
testing SUCCESS of _mem_safe_read(size: 16, width: 4).......PASS
testing SUCCESS of _mem_safe_read(size: 14, width: 2).......PASS
testing SUCCESS of _mem_safe_read(size: 15, width: 1).......PASS
testing FAILURE of _mem_safe_read() with bad params.......PASS (-22)
testing SUCCESS of _mem_safe_write(size: 16, width: 0).......PASS
testing SUCCESS of _mem_safe_write(size: 16, width: 4).......PASS
testing SUCCESS of _mem_safe_write(size: 14, width: 2).......PASS
testing SUCCESS of _mem_safe_write(size: 15, width: 1).......PASS
testing FAILURE of _mem_safe_write() with bad params.......PASS (-22)
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
