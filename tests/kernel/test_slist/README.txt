Title: Test slist

Description:

A simple application verifying slist API and its functionalities

--------------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

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

Starting slist test
 - Initializing the list
 - Appending node 1
 - Finding and removing node 1
 - Prepending node 1
 - Removing node 1
 - Appending node 1
 - Prepending node 2
 - Appending node 3
 - Finding and removing node 1
 - Removing node 3
 - Removing node 2
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
