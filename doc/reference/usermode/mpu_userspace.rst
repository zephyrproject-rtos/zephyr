.. _mpu_userspace:

MPU Backed Userspace
####################

The MPU backed userspace implementation requires the creation of a secondary
set of stacks.  These stacks exist in a 1:1 relationship with each thread stack
defined in the system.  The privileged stacks are created as a part of the
build process.

A post-build script ``gen_priv_stacks.py`` scans the generated
ELF file and finds all of the thread stack objects.  A set of privileged
stacks, a lookup table, and a set of helper functions are created and added
to the image.

During the process of dropping a thread to user mode, the privileged stack
information is filled in and later used by the swap and system call
infrastructure to configure the MPU regions properly for the thread stack and
guard (if applicable).

During system calls, the user mode thread's access to the system call and the
passed-in parameters are all validated.  The user mode thread is then elevated
to privileged mode, the stack is switched to use the privileged stack, and the
call is made to the specified kernel API.  On return from the kernel API,  the
thread is set back to user mode and the stack is restored to the user stack.

