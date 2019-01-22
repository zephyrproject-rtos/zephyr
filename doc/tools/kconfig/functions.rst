.. _kconfig-functions:

Kconfig Functions
#################

Kconfiglib provides user-defined preprocessor functions that
we use in Zephyr to expose Device Tree information to Kconfig.
For example, we can get the default value for a Kconfig symbol
from the device tree.

Device Tree Related Functions
*****************************

.. code-block:: none

  dt_int_val(kconf, _, name, unit):
       This function looks up 'name' in the DTS generated "conf" style database
       and if its found it will return the value as an decimal integer.  The
       function will divide the value based on 'unit':
           None        No division
           'k' or 'K'  divide by 1024 (1 << 10)
           'm' or 'M'  divide by 1,048,576 (1 << 20)
           'g' or 'G'  divide by 1,073,741,824 (1 << 30)

  dt_hex_val(kconf, _, name, unit):
       This function looks up 'name' in the DTS generated "conf" style database
       and if its found it will return the value as an hex integer.  The
       function will divide the value based on 'unit':
           None        No division
           'k' or 'K'  divide by 1024 (1 << 10)
           'm' or 'M'  divide by 1,048,576 (1 << 20)
           'g' or 'G'  divide by 1,073,741,824 (1 << 30)

Example Usage
*************

The following example shows the usage of the ``dt_int_val`` function:

.. code-block:: none

   boards/arm/mimxrt1020_evk/Kconfig.defconfig

   config FLASH_SIZE
      default $(dt_int_val,DT_NXP_IMX_FLEXSPI_402A8000_SIZE_1,K)

In this example if we examine the generated generated_dts_board.conf file
as part of the Zephyr build we'd find the following entry:

.. code-block:: none

   DT_NXP_IMX_FLEXSPI_402A8000_SIZE_1=8388608

The ``dt_int_val`` will search the generated_dts_board.conf that is derived from
the dts for the board and match the ``DT_NXP_IMX_FLEXSPI_402A8000_SIZE_1`` entry.
The function than will than scale the value by ``1024``.  This effective causes
the above to look like:

.. code-block:: none

   config FLASH_SIZE
      default 8192
