.. _usermode_api:

User Mode
#########

Zephyr offers the capability to run threads at a reduced privilege level
which we call user mode. The current implementation is designed for devices
with MPU hardware.

For details on creating threads that run in user mode, please see
:ref:`lifecycle_v2`.

.. toctree::
    :maxdepth: 2

    overview.rst
    memory_domain.rst
    kernelobjects.rst
    syscalls.rst
    mpu_stack_objects.rst
    mpu_userspace.rst
