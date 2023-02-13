.. _userspace_protected_memory:

Userspace Protected Memory
##########################

Overview
********
This sample is an example of running multiple threads assigned
unique memory domains with protected partitions.  The
application uses memory partitioning with a sample algorithm
simulating an enigma-like machine, but the implementation of the
machine has not been validated and should not be used for any
actual security purposes.

Requirements
************

The sample is dependent on the subsystem app_memory, and it will
not run on boards that do not support the subsystem.  The sample
was tested on the following boards qemu_x86,frdm_k64, an 96b_carbon.

Building and Running
********************

This example will only cover the qemu_x86 board, since the sample
just prints text to a console.

.. zephyr-app-commands::
   :zephyr-app: samples/userspace/shared_mem
   :board: qemu_x86
   :goals: build run
   :compact:

After starting, the console will display multiple starting messages
followed by two series of repeating messages.  The repeating messages
are the input and output of the enigma-like machine.  The second
message is the output of the first message, and the resulting
output is the first message without spaces.  The two messages are
marked as 1 and 1' respectively.

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Two definitions can be inserted to change the wheel settings and print
the state information.  To enable the definitions uncomment the last
two lines in CMakeLists.txt.

Functionality
*************
The PT thread sends a message followed by the encrypted version of the
message after sleeping.  To send the message the PT thread checks a
flag, and if it is clear, writes the message to a buffer shared with
the encrypt thread.  After writing the buffer, the flag is set. The
encrypt thread copies the memory from the common buffer into the
encrypted thread's private memory when the flag is set and then clears
the flag.  Once the encrypted thread receives the text string, it
performs a simulation of the enigma machine to produce cypher text(CT).
The CT is copied to a shared memory partition connecting to the third
thread. The third thread prints the CT to the console with a banner
denoting the content as CYPHER TEXT.
