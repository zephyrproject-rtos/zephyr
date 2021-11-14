# Objective

* Test the sample code for TMP100 sensor.
* Take the ambient temperature readings using temperature sensor having I2C address 0x48.

# Requirements

* Zephyr setup on local system (Linux/macOS/Windows). Kindly follow the [Getting Started](https://docs.zephyrproject.org/latest/getting_started/index.html) guide of [Zephyr Project](https://zephyrproject.org/) documentation for more information.

# Pin connections

| Board pin | Sensor Pin |
| --------- | ---------- |
| 5V        | VCC        |
| GND       | GND        |
| P0.26     | SDA        |
| P0.27     | SCL        |
|           |            |

# Procedure

* Navigate to ZephyrPlayground directory and run the following commands.

  ```
  west build -p auto -b nrf52840dk_nrf52811 tmp100
  ```

* If successfully built then run

  ```
  west flash --erase
  ```

* Use any serial monitor of your choice to view the serial data (Temperature readings in this case).

![tmp100](https://user-images.githubusercontent.com/86110190/133203067-8bbbcf24-fff4-467f-ab9e-8ed4bfe4ba8a.JPG)
