i2c demo
######

Overview
********

This Arduino i2c sample gets data from an ADXL345 sensor connected over ``i2c0``.

Requirements
************

Your board must:

#. Have at least 1 i2c port.
#. Have the ADXL345 sensor connected to the I2C port.

Building and Running
********************

Build and flash Blinky as follows,

```sh
$> west build -p -b arduino_nano_33_ble samples/i2cdemo

$> west flash --bossac=/home/$USER/.arduino15/packages/arduino/tools/bossac/1.9.1-arduino2/bossac
```

After flashing, probe the UART pin (TX) and you should be able to see X, Y and Z values if everything goes well.
