.. _usermode_overview:

Overview
########

Threat Model
************

User mode threads are considered to be untrusted by Zephyr and are therefore
isolated from other user mode threads and from the kernel. A flawed or
malicious user mode thread cannot leak or modify the private data/resources
of another thread or the kernel, and cannot interfere with or
control another user mode thread or the kernel.

Example use-cases of Zephyr's user mode features:

- The kernel can protect against many unintentional programming errors which
  could otherwise silently or spectacularly corrupt the system.

- The kernel can sandbox complex data parsers such as interpreters, network
  protocols, and filesystems such that malicious third-party code or data
  cannot compromise the kernel or other threads.

- The kernel can support the notion of multiple logical "applications", each
  with their own group of threads and private data structures, which are
  isolated from each other if one crashes or is otherwise compromised.

Design Goals
============

For threads running in a non-privileged CPU state (hereafter referred to as
'user mode') we aim to protect against the following:

- We prevent access to memory not specifically granted, or incorrect access to
  memory that has an incompatible policy, such as attempting to write to a
  read-only area.

  - Access to thread stack buffers will be controlled with a policy which
    partially depends on the underlying memory protection hardware.

    - A user thread will by default have read/write access to its own stack
      buffer.

    - A user thread will never by default have access to user thread stacks
      that are not members of the same memory domain.

    - A user thread will never by default have access to thread stacks owned
      by a supervisor thread, or thread stacks used to handle system call
      privilege elevations, interrupts, or CPU exceptions.

    - A user thread may have read/write access to the stacks of other user
      threads in the same memory domain, depending on hardware.

       - On MPU systems, threads may only access their own stack buffer.

       - On MMU systems, threads may access any user thread stack in the same
         memory domain. Portable code should not assume this.

  - By default, program text and read-only data are accessible to all threads
    on read-only basis, kernel-wide. This policy may be adjusted.

  - User threads by default are not granted default access to any memory
    except what is noted above.

- We prevent use of device drivers or kernel objects not specifically granted,
  with the permission granularity on a per object or per driver instance
  basis.

- We validate kernel or driver API calls with incorrect parameters that would
  otherwise cause a crash or corruption of data structures private to the
  kernel. This includes:

  - Using the wrong kernel object type.

  - Using parameters outside of proper bounds or with nonsensical values.

  - Passing memory buffers that the calling thread does not have sufficient
    access to read or write, depending on the semantics of the API.

  - Use of kernel objects that are not in a proper initialization state.

- We ensure the detection and safe handling of user mode stack overflows.

- We prevent invoking system calls to functions excluded by the kernel
  configuration.

- We prevent disabling of or tampering with kernel-defined and hardware-
  enforced memory protections.

- We prevent re-entry from user to supervisor mode except through the kernel-
  defined system calls and interrupt handlers.

- We prevent the introduction of new executable code by user mode threads,
  except to the extent to which this is supported by kernel system calls.

We are specifically not protecting against the following attacks:

- The kernel itself, and any threads that are executing in supervisor mode,
  are assumed to be trusted.

- The toolchain and any supplemental programs used by the build system are
  assumed to be trusted.

- The kernel build is assumed to be trusted. There is considerable build-time
  logic for creating the tables of valid kernel objects, defining system calls,
  and configuring interrupts. The .elf binary files that are worked with
  during this process are all assumed to be trusted code.

- We can't protect against mistakes made in memory domain configuration done in
  kernel mode that exposes private kernel data structures to a user thread. RAM
  for kernel objects should always be configured as supervisor-only.

- It is possible to make top-level declarations of user mode threads and
  assign them permissions to kernel objects. In general, all C and header
  files that are part of the kernel build producing zephyr.elf are assumed to
  be trusted.

- We do not protect against denial of service attacks through thread CPU
  starvation. Zephyr has no thread priority aging and a user thread of a
  particular priority can starve all threads of lower priority, and also other
  threads of the same priority if time-slicing is not enabled.

- There are build-time defined limits on how many threads can be active
  simultaneously, after which creation of new user threads will fail.

- Stack overflows for threads running in supervisor mode may be caught,
  but the integrity of the system cannot be guaranteed.

High-level Policy Details
*************************

Broadly speaking, we accomplish these thread-level memory protection goals
through the following mechanisms:

- Any user thread will only have access to a subset of memory:
  typically its stack, program text, read-only data, and any partitions
  configured in the :ref:`memory_domain` it belongs to. Access to any other RAM
  must be done on the thread's behalf through system calls, or specifically
  granted by a supervisor thread using the memory domain APIs. Newly created
  threads inherit the memory domain configuration of the parent. Threads may
  communicate with each other by having shared membership of the same memory
  domains, or via kernel objects such as semaphores and pipes.

- User threads cannot directly access memory belonging to kernel objects.
  Although pointers to kernel objects are used to reference them, actual
  manipulation of kernel objects is done through system call interfaces. Device
  drivers and threads stacks are also considered kernel objects. This ensures
  that any data inside a kernel object that is private to the kernel cannot be
  tampered with.

- User threads by default have no permission to access any kernel object or
  driver other than their own thread object. Such access must be granted by
  another thread that is either in supervisor mode or has permission on both
  the receiving thread object and the kernel object being granted access to.
  The creation of new threads has an option to automatically inherit
  permissions of all kernel objects granted to the parent, except the parent
  thread itself.

- For performance and footprint reasons Zephyr normally does little or no
  parameter error checking for kernel object or device driver APIs. Access from
  user mode through system calls involves an extra layer of handler functions,
  which are expected to rigorously validate access permissions and type of
  the object, check the validity of other parameters through bounds checking or
  other means, and verify proper read/write access to any memory buffers
  involved.

- Thread stacks are defined in such a way that exceeding the specified stack
  space will generate a hardware fault. The way this is done specifically
  varies per architecture.

Constraints
***********

All kernel objects, thread stacks, and device driver instances must be defined
at build time if they are to be used from user mode. Dynamic use-cases for
kernel objects will need to go through pre-defined pools of available objects.

There are some constraints if additional application binary data is loaded
for execution after the kernel starts:

- Loaded object code will not be able to define any kernel objects that will be
  recognized by the kernel. This code will instead need to use APIs for
  requesting kernel objects from pools.

- Similarly, since the loaded object code will not be part of the kernel build
  process, this code will not be able to install interrupt handlers,
  instantiate device drivers, or define system calls, regardless of what
  mode it runs in.

- Loaded object code that does not come from a verified source should always
  be entered with the CPU already in user mode.
