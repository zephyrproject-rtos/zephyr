Title: test_mail

Description:

This test verifies that the microkernel mailbox APIs operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
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

Starting mailbox tests
===================================================================
MsgSenderTask: task_mbox_put to non-waiting task is OK
MsgRcvrTask: task_mbox_get when no message is OK
MsgSenderTask: task_mbox_put_wait_timeout to non-waiting task is OK
MsgRcvrTask: task_mbox_get_wait_timeout when no message is OK
MsgRcvrTask: task_mbox_get_wait from specified task is OK
MsgSenderTask: task_mbox_put to specified waiting task is OK
MsgRcvrTask: task_mbox_get from anonymous task is OK
MsgSenderTask: task_mbox_put_wait_timeout to anonymous non-waiting task is OK
MsgSenderTask: task_mbox_put_wait of empty message is OK
MsgRcvrTask: task_mbox_get_wait of empty message is OK
MsgRcvrTask: task_mbox_get_wait of message header #3 is OK
MsgRcvrTask: task_mbox_data_get of message data #3 is OK
MsgSenderTask: task_mbox_put_wait_timeout for 2 part receive test is OK
MsgRcvrTask: task_mbox_get_wait_timeout of message header #4 is OK
MsgRcvrTask: task_mbox_data_get cancellation of message #4 is OK
MsgSenderTask: task_mbox_put_wait for cancelled receive test is OK
MsgRcvrTask: task_mbox_get_wait of message header #1 is OK
MsgRcvrTask: task_mbox_data_get_async_block of message data #1 is OK
MsgSenderTask: task_mbox_put_wait for block-based receive test is OK
MsgRcvrTask: task_mbox_get_wait of message header #2 is OK
MsgRcvrTask: task_mbox_data_get_async_block of message data #2 is OK
MsgSenderTask: task_mbox_put_wait for block-exhaustion receive test is OK
MsgRcvrTask: task_mbox_get_wait of message header #3 is OK
MsgRcvrTask: task_mbox_data_get of message data #3 is OK
MsgSenderTask: task_mbox_put_wait_timeout for long-duration receive test is OK
===================================================================
PROJECT EXECUTION SUCCESSFUL
