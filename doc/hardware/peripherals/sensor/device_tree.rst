Device Tree
###########

In the context of sensors device tree provides the initial hardware configuration
for sensors on a per device level. Each device must specify a device tree binding
in Zephyr, and ideally, a set of hardware configuration options for things such
as channel power modes, data rates, filters, decimation, and scales. These can
then be used in a boards devicetree to configure a sensor to its initial state.

.. code-block:: dts

   #include <zephyr/dt-bindings/icm42688.h>

   &spi0 {
       /* SPI bus options here, not shown */

       accel_gyro0: icm42688p@0 {
           compatible = "invensense,icm42688";
           reg = <0>;
           int-gpios = <&pioc 6 GPIO_ACTIVE_HIGH>; /* SoC specific pin to select for interrupt line */
           spi-max-frequency = <DT_FREQ_M(24)>; /* Maximum SPI bus frequency */
           accel-pwr-mode = <ICM42688_ACCEL_LN>; /* Low noise mode */
           accel-odr = <ICM42688_ACCEL_ODR_2000>; /* 2000 Hz sampling */
           accel-fs = <ICM42688_ACCEL_FS_16>; /* 16G scale */
           gyro-pwr-mode = <ICM42688_GYRO_LN>; /* Low noise mode */
           gyro-odr = <ICM42688_GYRO_ODR_2000>; /* 2000 Hz sampling */
           gyro-fs = <ICM42688_GYRO_FS_16>; /* 16G scale */
       };
    };
