.. zephyr:code-sample:: fingerprint-sensor
   :name: Fingerprint sensor
   :relevant-api: biometrics_interface

   Demonstrate fingerprint sensor enrollment, verification, identification, and template
   management using the biometric driver API.

Overview
********

This sample demonstrates how to use the fingerprint sensor biometric driver API, showing
enrollment, verification, identification, and template management.

Requirements
************

- Fingerprint sensor module (e.g., ZFM-20, ZFM-60, ZFM-70, R30x, JM-101, DY-50, FPM10A, or compatible)
- Appropriate connection to the sensor (UART, I2C, or SPI depending on sensor)

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/biometrics/fingerprint
   :board: <your_board>
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

    [00:00:00.177,000] <inf> main: Fingerprint Sensor Test
    [00:00:00.177,000] <inf> main: Sensor capabilities:
    [00:00:00.177,000] <inf> main:   Max templates: 150
    [00:00:00.177,000] <inf> main:   Template size: 512 bytes
    [00:00:00.177,000] <inf> main:   Enrollment samples: 2
    [00:00:00.177,000] <inf> main: Enroll two fingers
    [00:00:00.177,000] <inf> main: Enrolling finger ID 1
    [00:00:00.177,000] <inf> zfm_x0: Enrollment started for ID 1
    [00:00:00.177,000] <inf> main: Place finger...
    [00:00:01.938,000] <inf> zfm_x0: Enrollment capture completed (sample 1/2)
    [00:00:01.938,000] <inf> main: Remove finger
    [00:00:03.938,000] <inf> main: Place same finger again...
    [00:00:08.546,000] <inf> zfm_x0: Enrollment capture completed (sample 2/2)
    [00:00:08.681,000] <inf> zfm_x0: Enrollment completed for ID 1
    [00:00:08.681,000] <inf> main: Enrollment complete
    [00:00:08.681,000] <inf> main: Enrolling finger ID 2
    [00:00:08.681,000] <inf> zfm_x0: Enrollment started for ID 2
    [00:00:08.681,000] <inf> main: Place finger...
    [00:00:23.722,000] <inf> main: Found 2 stored template(s):
    [00:00:23.723,000] <inf> main:   - Template ID: 2
    [00:00:23.723,000] <inf> main:   - Template ID: 3

Features Demonstrated
*********************

- Query sensor capabilities (max templates, sample requirements)
- Enroll two fingerprints (2-sample process each)
- List stored templates
- Verify fingerprint against specific ID (1:1 match)
- Identify any enrolled finger (1:N search)
- Delete a template

Device Tree Configuration
**************************

Add your fingerprint sensor to the device tree with the ``fingerprint`` alias.

Example for ZFM-x0 series (UART-based):

.. code-block:: devicetree

   / {
       aliases {
           fingerprint = &fingerprint_sensor;
       };
   };

   &uart1 {
       status = "okay";
       current-speed = <57600>;

       fingerprint_sensor: fingerprint0 {
           compatible = "zhiantec,zfm-x0";
           comm-addr = <0xffffffff>;
       };
   };

For other sensors, use the appropriate compatible string and bus configuration.
