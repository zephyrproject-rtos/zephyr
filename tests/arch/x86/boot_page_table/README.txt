Title: x86 boot time page table validation

Description:

This test is for x86 to validate boot time page table faults.
Testcase outputs to console, for corresponding result, on validating
boot page table.

The checks are for param of the interested memory region
that are value->p, value->us and value->rw. These are permissoin associated
with the interested memory regions.

--------------------------------------------------------------------------------

Build and Run:

This testcase will output to console.  It can be built and executed
on QEMU as follows:

    mkdir build && cd build
    cmake -DBOARD=qemu_x86 ..
    make run

--------------------------------------------------------------------------------

Troubleshooting/Problem Solving:

Issues will occur if the object files and other related files generated from
previous build are not discarded. In that scenario below step can be followed

   make pristine	# remove old outdir directory
			# discard results/files generated from previous build

--------------------------------------------------------------------------------

Sample Output:

Running test suite boot_page_table_validate
===================================================================
starting test - test_boot_page_table
PASS - test_boot_page_table.
===================================================================
===================================================================
PROJECT EXECUTION SUCCESSFUL
