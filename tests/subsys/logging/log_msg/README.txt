Title: Log message test

Description:

This test verifies that the log_msg APIs operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    make run

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

***** Booting Zephyr OS v1.11.0-1019-g5dc2ab4f4 *****
Running test suite test_log_message
===================================================================
starting test - test_log_std_msg
PASS - test_log_std_msg.
===================================================================
starting test - test_log_hexdump_msg
PASS - test_log_hexdump_msg.
===================================================================
starting test - test_log_hexdump_data_get_single_chunk
PASS - test_log_hexdump_data_get_single_chunk.
===================================================================
starting test - test_log_hexdump_data_get_two_chunks
PASS - test_log_hexdump_data_get_two_chunks.
===================================================================
starting test - test_log_hexdump_data_get_multiple_chunks
PASS - test_log_hexdump_data_get_multiple_chunks.
===================================================================
===================================================================
PROJECT EXECUTION SUCCESSFUL
