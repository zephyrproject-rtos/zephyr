.. _usermode_sharedmem:

Application Shared Memory
#########################

.. note::

   In this document, we will cover the basic usage of enabling shared
   memory using a template around app_memory subsystem.

Overview
********

The use of subsystem app_memory in userspace allows control of
shared memory between threads.  The foundation of the implementation
consists of memory domains and partitions. Memory partitions are created
and used in the definition of variable to group them into a
common space.  The memory partitions are linked to domains
that are then assigned to a thread.  The process allows selective
access to memory from a thread and sharing of memory between two
threads by assigning a partition to two different domains.  By using
the shared memory template, code to protect memory can be used
on different platform without the application needing to implement
specific handlers for each platform.  Note the developer should understand
the hardware limitations in context to the maximum number of memory
partitions available to a thread.  Specifically processors with MPU's
cannot support the same number of partitions as a MMU.

This specific implementation adds a wrapper to simplify the programmers
task of using the app_memory subsystem through the use of macros and
a python script to generate the linker script. The linker script provides
the proper alignment for processors requiring power of two boundaries.
Without the wrapper, a developer is required to implement custom
linker scripts for each processor in the project.

The general usage is as follows. Define CONFIG_APP_SHARED_MEM=y in the
proj.conf file in the project folder.  Include app_memory/app_memdomain.h
in the userspace source file.  Mark the variable to be placed in
a memory partition.  The two markers are for data and bss respectively:
_app_dmem(id) and _app_bmem(id).  The id is used as the partition name.
The resulting section name can be seen in the linker.map as
"data_smem_id" and "data_smem_idb".

To create a k_mem_partition, call the macro appmem_partition(part0)
where "part0" is the name then used to refer to that partition.
This macro only creates a function and necessary data structures for
the later "initialization".

Once the partition is initialized, the standard memory domain APIs may
be used to add it to domains; the declared name is a k_mem_partition
symbol.

Example:

.. code-block:: c

            /* create partition at top of file outside functions */
            appmem_partition(part0);
            /* create domain */
            struct k_mem_domain dom0;
            /* assign variables to the domain */
            _app_dmem(part0) int var1;
            _app_bmem(part0) static volatile int var2;

            int main()
            {
                    appmem_init_part_part0();
                    appmem_init_app_memory();
                    k_mem_domain_init(&dom0, 0, NULL)
                    k_mem_domain_add_partition(&dom0, part0);
                    k_mem_domain_add_thread(&dom0, k_current_get());
                    ...
            }

If multiple partitions are being created, a variadic
preprocessor macro can be used as provided in
app_macro_support.h:

.. code-block:: c

 FOR_EACH(appmem_partition, part0, part1, part2);

Similarly, the appmem_init_part_* can also be used in the macro:

.. code-block:: c

 FOR_EACH(appmem_init_part, part0, part1, part2);


