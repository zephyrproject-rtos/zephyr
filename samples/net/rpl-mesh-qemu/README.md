# RPL Mesh Network over QEMU

Overview
********

This sample uses virtual-hub tool from zephyrproject-rtos/net-tools
to setup a multi-node RPL network on top of QEMU. This way, is possible
to validate different network topologies in virtualized environment.

In this sample, we will build a three node graph connected like a row:

root <-> node1 <-> node2

Requirements
************

First of all, we must clone net-tools repository, and build virtual-hub:
```
cd virtual-hub
mkdir build && cd build
cmake ..
make
```

Building and Running
********************

1. Build and run the RPL root application:
```
cd root
mkdir build && cd build
cmake -DQEMU_PIPE_ID=1 ..
make
make node
```

2. Build and run the first RPL node application:
```
cd node
mkdir build-1 && cd build-1
cmake -DQEMU_PIPE_ID=2 ..
make
make node
```

3. Build and run the second RPL node application:
```
cd node
mkdir build-2 && cd build-2
cmake -DQEMU_PIPE_ID=3 ..
make
make node
```

4. Now we should run the virtual-hub, which will connect
all the pipes according to the csv file:
```
cd virtual-hub/build
./hub ../input.csv
```

5. Wait until the network is fine, you can check it
using net shell on root application:
```
select net
route
```
You should see two routes pointing to both clients application.

Virtual-Hub Notes
*****************

When trying to reboot/drop a node from network you must
also reboot the virtual-hub to keep it working properly.

For more details about how to customize the network, follow the
README instructions from virtual-hub repository.