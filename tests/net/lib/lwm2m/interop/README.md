# LwM2M Interoperability tests using Leshan demo server

This directory contains list of testcases that use
the Twister's Pytest integration to run testcases against Leshan demo server.
These tests use emulated hardware (native_posix).

These tests require setup that is not done in Twister run, so follow this documentation to set
up the test environment.

## Network setup

As with typical network samples, host machine uses IP address `192.0.2.2` and the emulated device
running Zephyr is using address `192.0.2.1`.

Follow [Networking with the host system](https://docs.zephyrproject.org/latest/connectivity/networking/networking_with_host.html#networking-with-the-host-system)
from Zephyr's documentation how to set it up, or follow [Create NAT and routing for Zephyr native network on Linux](https://github.com/zephyrproject-rtos/net-tools/blob/master/README%20NAT.md).

### Leshan server setup

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
twister -p native_posix -vv --enable-slow -T tests/net/lib/lwm2m/interop
```

Or use the Docker based testing

```
./scripts/net/run-sample-tests.sh tests/net/lib/lwm2m/interop
```

## Test specification

Tests are written from test spec;
[OMA Enabler Test Specification (Interoperability) for Lightweight M2M](https://www.openmobilealliance.org/release/LightweightM2M/ETS/OMA-ETS-LightweightM2M-V1_1-20190912-D.pdf)

Following tests are implemented:
* LightweightM2M-1.1-int-0 – Client Initiated Bootstrap
* LightweightM2M-1.1-int-1 – Client Initiated Bootstrap Full (PSK)
* LightweightM2M-1.1-int-101 – Initial Registration
* LightweightM2M-1.1-int-102 – Registration Update
* LightweightM2M-1.1-int-104 – Registration Update Trigge
* LightweightM2M-1.1-int-105 - Discarded Register Update
* LightweightM2M-1.1-int-107 – Extending the lifetime of a registration
* LightweightM2M-1.1-int-108 – Turn on Queue Mode
* LightweightM2M-1.1-int-109 – Behavior in Queue Mode
* LightweightM2M-1.1-int-201 – Querying basic information in Plain Text
* LightweightM2M-1.1-int-203 – Querying basic information in TLV format
* LightweightM2M-1.1-int-204 – Querying basic information in JSON format
* LightweightM2M-1.1-int-205 – Setting basic information in Plain Text
* LightweightM2M-1.1-int-211 – Querying basic information in CBOR format
* LightweightM2M-1.1-int-212 – Setting basic information in CBOR format
* LightweightM2M-1.1-int-215 – Setting basic information in TLV format
* LightweightM2M-1.1-int-220 – Setting basic information in JSON format
* LightweightM2M-1.1-int-221 – Attempt to perform operations on Security
* LightweightM2M-1.1-int-401 – UDP Channel Security – PSK Mode
