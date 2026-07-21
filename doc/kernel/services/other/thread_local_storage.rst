.. _thread_local_storage:

Thread Local Storage (TLS)
##########################

Thread Local Storage (TLS) allows variables to be allocated on a per-thread
basis. These variables are stored in the thread stack which means every
thread has its own copy of these variables.

Zephyr currently requires toolchain support for TLS.


Configuration
*************

To enable thread local storage in Zephyr, :kconfig:option:`CONFIG_THREAD_LOCAL_STORAGE`
needs to be enabled. Note that this option may not be available if
the architecture or the SoC does not have the hidden option
:kconfig:option:`CONFIG_ARCH_HAS_THREAD_LOCAL_STORAGE` enabled, which means
the architecture or the SoC does not have the necessary code to support
thread local storage and/or the toolchain does not support TLS.

:kconfig:option:`CONFIG_ERRNO_IN_TLS` can be enabled together with
:kconfig:option:`CONFIG_ERRNO` to let the variable ``errno`` be a thread local
variable. This allows user threads to access the value of ``errno`` without
making a system call.


Declaring and Using Thread Local Variables
******************************************

The macro ``Z_THREAD_LOCAL`` can be used to declare thread local variables.

For example, to declare a thread local variable in header files:

.. code-block:: c

   extern Z_THREAD_LOCAL int i;

And to declare the actual variable in source files:

.. code-block:: c

   Z_THREAD_LOCAL int i;

Keyword ``static`` can also be used to limit the variable within a source file:

.. code-block:: c

   static Z_THREAD_LOCAL int j;

Using the thread local variable is the same as using other variable, for example:

.. code-block:: c

   void testing(void) {
       i = 10;
   }
