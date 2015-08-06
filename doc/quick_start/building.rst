.. _building_zephyr:

Building the Zephyr Kernel
############################

This section describes the procedure to build the kernel with a sample
application. in a development system and describes how to get access to the
kernel's source code.

Building a Sample Application
*****************************

You can speed up builds by optionally using the :program:`ccache` utility.

.. code-block:: bash

    $ export USE_CCACHE=1

To build an example application follow these steps:

#. Go to the root directory of the |project|.

#. Set the paths properly in the :file:`$ZEPHYR_BASE` directory,
   type:

    .. code-block:: bash

       $ source zephyr-env.sh

#. Build the example project, type:

    .. code-block:: bash

       $ cd $ZEPHYR_BASE/samples/microkernel/apps/hello_world

       $ make pristine && make

.. note::

   You can override the default BSP with the one you want by adding
   :makevar:`BSP=`. The complete options available for the BSP flag
   can be found at :file:`$ZEPHYR_BASE/arch` under the respective
   architecture, for example :file:`$ZEPHYR_BASE/arch/x86/generic_pc`.
   You need to override the ARCH flag with the architecture that
   corresponds to your BSP by adding :makevar:`ARCH=` and the options
   you need to the make command, for example:

   :command:`make BSP=generic_pc ARCH=x86`

   The complete options available for the ARCH flag can be found at
   :file:`$ZEPHYR_BASE`, for example  :file:`$ZEPHYR_BASE/arch/x86`.

The sample projects for the microkernel are found
at :file:`$ZEPHYR_BASE/samples/microkernel/apps`. After building the
application successfully, the results can be found in the :file:`outdir`
directory in the application root directory.
:file:`$SAMPLE_PROJECT/outdir/microkernel.elf`.

For sample projects in the :file:`$ZEPHYR_BASE/samples/nanokernel/apps`
directory, the results can be found at :file:`$SAMPLE_PROJECT/outdir/nanokernel.elf`.