This folder contains an example MQTT-SN Gateway made for use with a native_sim board application utilizing MQTT-SN.

## Usage
These steps will create all the necessary containers along with the network bridge between the Zephyr network interface and the containers.
1. Start the net-tools configuration from [here](https://github.com/zephyrproject-rtos/net-tools) with `./net-setup.sh --config docker.conf`.
2. Start the containers from the `./compose` directory with `docker compose up`.
