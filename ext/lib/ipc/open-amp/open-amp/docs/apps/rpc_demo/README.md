
# rpc_demo 
This readme is about the OpenAMP rpc_demo demo.
The rpc_demo is about one processor uses the UART on the other processor and create file on the other processor's filesystem with file operations.

For now, It implements the processor running generic(baremetal) applicaiton access the devices on the Linux.

## Compilation

### Baremetal Compilation
Option `WITH_RPC_DEMO` is to control if the application will be built.
By default this option is `ON` when `WITH_APPS` is on.

Here is an example:

```
$ cmake ../open-amp -DCMAKE_TOOLCHAIN_FILE=zynq7_generic -DWITH_OBSOLETE=on -DWITH_APPS=ON
```

### Linux Compilation

#### Linux Kernel Compilation
You will need to manually compile the following kernel modules with your Linux kernel (Please refer to Linux kernel documents for how to add kernel module):

* Your machine's remoteproc kernel driver
* `obsolete/apps/rpc_demo/system/linux/kernelspace/rpmsg_proxy_dev_driver`

#### Linux Userspace Compliation
* Compile `obsolete/apps/rpc_demo/system/linux/userspace/proxy_app` into your Linux OS.
* Add the built generic `rpc_demo` executable to the firmware of your Linux OS.

## Run the Demo
After Linux boots, run `proxy_app` as follows:
```
# proxy_app [-m REMOTEPROC_MODULE] [-f PATH_OF_THE_RPC_DEMO_FIRMWARE]
```

The demo application will load the remoteproc module, then the proxy rpmsg module, will output message sent from the other processor, send the console input back to the other processor. When the demo application exits, it will unload the kernel modules.
