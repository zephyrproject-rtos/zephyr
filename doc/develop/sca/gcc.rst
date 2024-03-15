.. _gcc:

GCC static analysis support
###########################

Static analysis was introduced in `GCC <https://gcc.gnu.org/>`__ 10 and it is enabled
with the option ``-fanalyzer``. This option performs a much more expensive and thorough
analysis of the code than traditional warnings.

Run GCC static analysis
***********************

To run GCC static analysis, :ref:`west build <west-building>` should be
called with a ``-DZEPHYR_SCA_VARIANT=gcc`` parameter, e.g.

.. code-block:: shell

    west build -b qemu_x86 samples/userspace/hello_world_user -- -DZEPHYR_SCA_VARIANT=gcc
