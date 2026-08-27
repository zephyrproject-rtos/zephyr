# LwM2M Interoperability tests using Leshan demo server

This directory contains list of testcases that use
the Twister's Pytest integration to run testcases against Leshan demo server.
These tests use emulated hardware (native_sim).

These tests require setup that is not done in Twister run, so follow this documentation to set
up the test environment.

## Network setup

As with typical network samples, host machine uses IP address `192.0.2.2` and the emulated device
running Zephyr is using address `192.0.2.1`.

Follow [Networking with the host system](https://docs.zephyrproject.org/latest/connectivity/networking/networking_with_host.html#networking-with-the-host-system)
from Zephyr's documentation how to set it up, or follow [Create NAT and routing for Zephyr native network on Linux](https://github.com/zephyrproject-rtos/net-tools/blob/master/README%20NAT.md).


### Run Lehan server from net-tools Docker container

Zephyr's net-tools Docker container already contains Leshan, so if you don't want to set the environment up manually,
use the pre-made docker.

First, build the docker container. You only need to do this one, or when you update the container.
```
cd tools/net-tools/docker
docker build -t net-tools .
```

Start the docker networking
```
cd tools/net-tools/
sudo ./net-setup.sh --config docker.conf start
```

Start the docker container and run leshan
```
docker run --hostname=net-tools --name=net-tools --ip=192.0.2.2 --ip6=2001:db8::2 -p 8080:8080 -p 8081:8081 -p 5683:5683/udp --rm -dit --network=net-tools0 net-tools
docker container exec net-tools /net-tools/start-leshan.sh
```

### Stop Leshan, docker and networking

```
cd tools/net-tools/
docker kill net-tools
sudo ./net-setup.sh --config docker.conf stop
docker network rm net-tools0
```
### Leshan server setup (manual)

* Leshan server must be reachable from the device using IP address `192.0.2.2`.
  Configure the port forwarding, if you use Docker to run Leshan.
* Leshan demo server REST API must be reachable from localhost.
* tcp/8080 Leshan web interface and REST API
* tcp/8081 Leshan bootstrap server REST API
* udp/5683 Leshan non-secure CoAP
* udp/5684 Leshan DTLS CoAP
* udp/5783 non-secure Bootstrap CoAP
* udp/5684 DTLS Bootstrap CoAP
* Download Leshan JAR file from https://ci.eclipse.org/leshan/job/leshan/lastSuccessfulBuild/artifact/leshan-server-demo.jar
* Download Leshan Bootstrap server JAR file from https://ci.eclipse.org/leshan/job/leshan/lastSuccessfulBuild/artifact/leshan-bsserver-demo.jar

Both server can be started like this:
```
java -jar ./leshan-server-demo.jar -wp 8080 -vv
java -jar ./leshan-bsserver-demo.jar -lp=5783 -slp=5784 -wp 8081
```

Or create a helper script that does everything, including download:
```
#!/bin/sh -eu

# Download Leshan if needed
if [ ! -f leshan-server-demo.jar ]; then
        wget https://ci.eclipse.org/leshan/job/leshan/lastSuccessfulBuild/artifact/leshan-server-demo.jar
fi

if [ ! -f leshan-bsserver-demo.jar ]; then
	wget 'https://ci.eclipse.org/leshan/job/leshan/lastSuccessfulBuild/artifact/leshan-bsserver-demo.jar'
fi

mkdir -p log

start-stop-daemon --make-pidfile --pidfile log/leshan.pid --chdir $(pwd) --background --start \
        --startas /bin/bash -- -c "exec java -jar ./leshan-server-demo.jar -wp 8080 -vv --models-folder objects >log/leshan.log 2>&1"

start-stop-daemon --make-pidfile --pidfile log/leshan_bs.pid --chdir $(pwd) --background --start \
        --startas /bin/bash -- -c "exec java -jar ./leshan-bsserver-demo.jar -lp=5783 -slp=5784 -wp 8081 -vv >log/leshan_bs.log 2>&1"
```

Then stopping would require similar script:
```
#!/bin/sh -eu

start-stop-daemon --remove-pidfile --pidfile log/leshan.pid --stop
start-stop-daemon --remove-pidfile --pidfile log/leshan_bs.pid --stop
```

## Python package requirements

These tests require extra package that is not installed when you follow the Zephyr's setup.
Install with `pip install CoAPthon3`

## Running tests

```
twister -p native_sim -vv --enable-slow -T tests/net/lib/lwm2m/interop
```

Or use the Docker based testing

```
./scripts/net/run-sample-tests.sh tests/net/lib/lwm2m/interop
```

## Test specification

Tests are written from test spec;
[OMA Enabler Test Specification (Interoperability) for Lightweight M2M](https://www.openmobilealliance.org/release/LightweightM2M/ETS/OMA-ETS-LightweightM2M-V1_1-20190912-D.pdf)

## Current status

|Test case|Status|Notes|
|---------|------|-----|
|LightweightM2M-1.1-int-0 - Client Initiated Bootstrap                  |:white_check_mark:| |
|LightweightM2M-1.1-int-1 - Client Initiated Bootstrap Full (PSK)       |:white_check_mark:| |
|LightweightM2M-1.1-int-2 - Client Initiated Bootstrap Full (Cert)      | |testcase not implemented |
|LightweightM2M-1.1-int-3 – Simple Bootstrap from Smartcard             |:large_orange_diamond:|We don't have any smartcard support.|
|LightweightM2M-1.1-int-4 - Bootstrap Delete                            |:white_check_mark:| |
|LightweightM2M-1.1-int-5 - Server Initiated Bootstrap                  |:white_check_mark:| |
|LightweightM2M-1.1-int-6 - Bootstrap Sequence                          |:white_check_mark:| |
|LightweightM2M-1.1-int-7 - Fallback to bootstrap                       |:white_check_mark:| |
|LightweightM2M-1.1-int-8 - Bootstrap Read | |Test cannot be implemented from client side.|
|LightweightM2M-1.1-int-9 - Bootstrap and Configuration Consistency     | |testcase not implemented |
|LightweightM2M-1.1-int-101 - Initial Registration                      |:white_check_mark:| |
|LightweightM2M-1.1-int-102 - Registration Update                       |:white_check_mark:| |
|LightweightM2M-1.1-int-103 - Deregistration                            |:white_check_mark:| |
|LightweightM2M-1.1-int-104 - Registration Update Trigge                |:white_check_mark:| |
|LightweightM2M-1.1-int-105 - Discarded Register Update                 |:white_check_mark:| |
|LightweightM2M-1.1-int-107 - Extending the lifetime of a registration  |:white_check_mark:| |
|LightweightM2M-1.1-int-108 - Turn on Queue Mode                        |:white_check_mark:| |
|LightweightM2M-1.1-int-109 - Behavior in Queue Mode                    |:white_check_mark:| |
|LightweightM2M-1.1-int-201 - Querying basic information in Plain Text  |:white_check_mark:| |
|LightweightM2M-1.1-int-203 - Querying basic information in TLV format  |:white_check_mark:| |
|LightweightM2M-1.1-int-204 - Querying basic information in JSON format |:white_check_mark:| |
|LightweightM2M-1.1-int-205 - Setting basic information in Plain Text   |:white_check_mark:| |
|LightweightM2M-1.1-int-211 - Querying basic information in CBOR format |:white_check_mark:| |
|LightweightM2M-1.1-int-212 - Setting basic information in CBOR format  |:white_check_mark:| |
|LightweightM2M-1.1-int-215 - Setting basic information in TLV format   |:white_check_mark:| |
|LightweightM2M-1.1-int-220 - Setting basic information in JSON format  |:white_check_mark:| |
|LightweightM2M-1.1-int-221 - Attempt to perform operations on Security |:white_check_mark:| |
|LightweightM2M-1.1-int-222 - Read on Object                            |:white_check_mark:| |
|LightweightM2M-1.1-int-223 - Read on Object Instance                   |:white_check_mark:| |
|LightweightM2M-1.1-int-224 - Read on Resource                          |:white_check_mark:| |
|LightweightM2M-1.1-int-225 - Read on Resource Instance                 |:white_check_mark:| |
|LightweightM2M-1.1-int-226 - Write (Partial Update) on Object Instance |:white_check_mark:| |
|LightweightM2M-1.1-int-222 - Read on Object                            |:white_check_mark:| |
|LightweightM2M-1.1-int-223 - Read on Object Instance                   |:white_check_mark:| |
|LightweightM2M-1.1-int-224 - Read on Resource                          |:white_check_mark:| |
|LightweightM2M-1.1-int-225 - Read on Resource Instance                 |:white_check_mark:| |
|LightweightM2M-1.1-int-226 - Write (Partial Update) on Object Instance |:white_check_mark:| |
|LightweightM2M-1.1-int-227 - Write (replace) on Resource               |:white_check_mark:| |
|LightweightM2M-1.1-int-228 - Write on Resource Instance                |:white_check_mark:|[~~#64011~~](https://github.com/zephyrproject-rtos/zephyr/issues/64011) |
|LightweightM2M-1.1-int-229 - Read-Composite Operation                  |:white_check_mark:|[~~#64012~~](https://github.com/zephyrproject-rtos/zephyr/issues/64012) [~~#64189~~](https://github.com/zephyrproject-rtos/zephyr/issues/64189) |
|LightweightM2M-1.1-int-230 - Write-Composite Operation                 |:white_check_mark:| |
|LightweightM2M-1.1-int-231 - Querying basic information in SenML JSON format|:white_check_mark:| |
|LightweightM2M-1.1-int-232 - Querying basic information in SenML CBOR format|:white_check_mark:| |
|LightweightM2M-1.1-int-233 - Setting basic information in SenML CBOR format|:white_check_mark:| |
|LightweightM2M-1.1-int-234 - Setting basic information in SenML JSON format|:white_check_mark:| |
|LightweightM2M-1.1-int-235 - Read-Composite Operation on root path     |:white_check_mark:| |
|LightweightM2M-1.1-int-236 - Read-Composite - Partial Presence|:white_check_mark:| |
|LightweightM2M-1.1-int-237 - Read on Object without specifying Content-Type|:white_check_mark:| |
|LightweightM2M-1.1-int-241 - Executable Resource: Rebooting the device |:white_check_mark:| |
|LightweightM2M-1.1-int-256 - Write Operation Failure                   |:white_check_mark:| |
|LightweightM2M-1.1-int-257 - Write-Composite Operation                 |:white_check_mark:| |
|LightweightM2M-1.1-int-260 - Discover Command                          |:white_check_mark:| |
|LightweightM2M-1.1-int-261 - Write-Attribute Operation on a multiple resource|:white_check_mark:| |
|LightweightM2M-1.1-int-280 - Successful Read-Composite Operation       |:white_check_mark:| |
|LightweightM2M-1.1-int-281 - Partially Successful Read-Composite Operation|:white_check_mark:| |
|LightweightM2M-1.1-int-301 - Observation and Notification of parameter values|:white_check_mark:| |
|LightweightM2M-1.1-int-302 - Cancel Observations using Reset Operation |:white_check_mark:| |
|LightweightM2M-1.1-int-303 - Cancel observations using Observe with Cancel parameter|:white_check_mark:||
|LightweightM2M-1.1-int-304 - Observe-Composite Operation               |:white_check_mark:| |
|LightweightM2M-1.1-int-305 - Cancel Observation-Composite Operation    |:white_check_mark:| |
|LightweightM2M-1.1-int-306 – Send Operation                            |:white_check_mark:|[~~#64290~~](https://github.com/zephyrproject-rtos/zephyr/issues/64290)|
|LightweightM2M-1.1-int-307 – Muting Send                               |:white_check_mark:| |
|LightweightM2M-1.1-int-308 - Observe-Composite and Creating Object Instance|:white_check_mark:|[~~#64634~~](https://github.com/zephyrproject-rtos/zephyr/issues/64634)|
|LightweightM2M-1.1-int-309 - Observe-Composite and Deleting Object Instance|:white_check_mark:|[~~#64634~~](https://github.com/zephyrproject-rtos/zephyr/issues/64634)|
|LightweightM2M-1.1-int-310 - Observe-Composite and modification of parameter values|:white_check_mark:| |
|LightweightM2M-1.1-int-311 - Send command                              |:white_check_mark:| |
|LightweightM2M-1.1-int-401 - UDP Channel Security - PSK Mode           |:white_check_mark:| |
|LightweightM2M-1.1-int-1630 - Create Portfolio Object Instance         |:white_check_mark:| |
|LightweightM2M-1.1-int-1635 - Delete Portfolio Object Instance         |:white_check_mark:| |

* :white_check_mark: Working OK.
* :large_orange_diamond: Feature or operation not implemented.
* :red_circle: Broken
