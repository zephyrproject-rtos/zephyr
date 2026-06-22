******************************
Bluetooth co-existence drivers
******************************

Co-existence Ticker
###################

Implementation :file:`coex_ticker.c` is designed to utilize co-existence with another transmitter. Chips such as nordic nRF9160 provide a 1-wire co-existence interface, which allows the Bluetooth controller to suspend its activity until the other transceiver suspends its operation.

Nordic connect SDK provides detailed description of the 1-wire and 3-wire co-existence interface for the `SoftDevice Bluetooth controller <https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrfxlib/mpsl/doc/bluetooth_coex.html>`_

Similarly, as in the nordic implementation of the 1-wire interface, the coexistence ticker utilizes a single pin called BLE_GRANT, which active level (high or low) is programmable by the device tree definition.

.. code-block:: DTS

    coex_gpio: coex {
        compatible = "radio-gpio-coex";
        grant-gpios = <&gpio0 0 (GPIO_PULL_DOWN | GPIO_ACTIVE_HIGH)>;
        grant-delay-us = <150>;
    };

Whenever the grant pin transitions into non-active (such as 1 for the nRF9160). state the implementation starts a ticker job, which in predefined intervals cancels any radio events. This way all advertisements and other radioactivities are suspended until the grant pin transitions into an active state.
