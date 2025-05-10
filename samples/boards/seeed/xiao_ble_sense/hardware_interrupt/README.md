## Description
This is showing how to use and set up the double_tap interrupt on the xiao_ble_sense.

For a detailed description for how to first configure the interrupt 1 pin in the .overlay file and generally how to use callbacks in tandem with the int 1 pin for the XIAO BLE Sense, please refer to the README.md file found in the interrupts folder (applications/example_apps/interrupts/README.md).

Each interrupt requires different values being written to the registers on the LSM. For double tap to work, these registers need to be written to:
```
void setupDoubleTapInterrupt(){

    struct lsm6dsl_data *lsm_data = lsm6ds3_dev->data;
    /*
     * For more detailed descriptions of each register please refer to the LSM6DSL
     * datasheet found at http://www.st.com/resource/en/datasheet/lsm6dsl.pdf
     */

    //CTRL1_XL(10h): Linear acceleration sensor control register, sets ODR
    lsm_data->hw_tf->update_reg(lsm6ds3_dev, 0x10, 0xFF, 0x60);

    //TAP_CFG(58h): Enables interrupts and inactivity functions, configuration of filtering and tap recognition functions
    lsm_data->hw_tf->update_reg(lsm6ds3_dev, 0x58, 0xFF, 0x8E);

    //TAP_THS_6D(59h): Portrait/landscape position and tap function threshold register
    lsm_data->hw_tf->update_reg(lsm6ds3_dev, 0x59, 0xFF, 0x85);

    //INT_DUR2(5Ah): Tap recognition function setting, set "Duration", "Quiet" and "Shock."
    lsm_data->hw_tf->update_reg(lsm6ds3_dev, 0x5A, 0xFF, 0x7F);

    //WAKE_UP_THS(5Bh): Single/double-tap event enable, leaves threshold for wakeup at default
    lsm_data->hw_tf->update_reg(lsm6ds3_dev, 0x5B, 0xFF, 0x80);

    //MD1_CFG(5Eh): Enables INT1_DOUBLE_TAP, routing of 6D event on INT1 pin
    lsm_data->hw_tf->update_reg(lsm6ds3_dev, 0x5E, 0xFF, 0x08);

}
```

**CTRL1_XL** is the linear acceleration sensor control register 1 which allows you to change the output data rate (ODR) and power mode selection. This defaults to 0000 or power-down. As well as a few other configurations such as the accelerometer full-scale selection, digital LPF bandwidth selection and analog chain bandwidth selection which do not change in our applications, only the ODR.

**TAP_CFG** enables interrupt and inactivity functions, configurations of filering and tap recognition functions. For this we need to enable the most significant bit which enables basic interrupt generation. We can leave the INACT_EN function at default for this example (that is for the inactivity example). We can leave SLOPE_FDS alone for this examplesince that applies a filter for the wake-up and activity/inactivity interrupt examples. Then we want to enable the TAP_X_EN, TAP_Y_EN, and TAP_Z_EN which allows tap recognition to be enabled in all axis. You can turn off any of these registers to have a tap detected on only a singular axis if you wish. Then finally we want to leave the LIR bit at default so that the interrupts are not latched, if you want the interrupts to stay latched until reset this is where you would change that.

**TAP_THS_6D** is very important for the funcitonality of the single or double tap interrupts. This controls the portrait/landscape position and tap function thresholds.
For this we can change the threshold at which the taps are detected making a slight tap able to trigger a interrupt or a much harder tap needed. 

**INT_DUR2** is also extremely important for the single and double tap intterupts since this is the register that is the tap recognition function setting register. The DUR bits can be used to increase or decrease the amount of time between taps that trigger the double tap interrupt. You can also set the QUIET and SHOCK bits here as well that are not adjusted in this example, you can read more about those in the register description found in the references section in the README.md that is inside the general interrupts folder.

**WAKE_UP_THS**is the single and double tap function threshold register. The most significant bit in this register controls whether a single/double-tap event is enabled or not, this needs to be set to 1 for these interrupts to work. This register also contains bits for the threshold for wakeup.

**MD1_CFG** is the register that handles the routing of interrupts to the INT1 pin. For this example all we need to do is write a 1 to the INT1_DOUBLE_TAP bit to enable that. This register also contains all the other possible interrupts that can be generated on the LSM.

## How to build:
Open a terminal as if building any other example, such as blinky, and build for the XIAO BLE Sense specifically:
```
    west build -p -b xiao_ble/nrf52840/sense \zephy\samples\boards\seeed\xiao_ble_sense\hardware_interrupt\
```

And monitor the output through a serial monitor, I utilize the Arduino IDE serial monitor setup for XIAO BLE Sense.
There should be a message: "Double Tap detected at (timestamp in ms)" each time a double tap is detected.