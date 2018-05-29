
# echo_test
This readme is about the OpenAMP echo_test demo.
The echo_test is about one processor sends message to the other one, and the other one echo back the message. The processor which sends the message will verify the echo message.

For now, it implements Linux sends the message, and the baremetal echos back.

## Compilation

### Baremetal Compilation
Option `WITH_ECHO_TEST` is to control if the application will be built.
By default this option is `ON` when `WITH_APPS` is on.

Here is an example:

```
$ cmake ../open-amp -DCMAKE_TOOLCHAIN_FILE=zynq7_generic -DWITH_OBSOLETE=on -DWITH_APPS=ON
```

### Linux Compilation

#### Linux Kernel Compilation
You will need to manually compile the following kernel modules with your Linux kernel (Please refer to Linux kernel documents for how to add kernel module):

* Your machine's remoteproc kernel driver
* `obsolete/apps/echo_test/system/linux/kernelspace/rpmsg_user_dev_driver` if you want to run the echo_test app in Linux user space.
* `obsolete/system/linux/kernelspace/rpmsg_echo_test_kern_app` if you want to run the echo_test app in Linux kernel space.

#### Linux Userspace Compliation
* Compile `obsolete/apps/echo_test/system/linux/userspace/echo_test` into your Linux OS.
* If you are running generic(baremetal) system as remoteproc slave, and Linux as remoteproc master, please also add the built generic `echo_test` executable to the firmware of your Linux OS.

## Run the Demo

### Load the Demo
After Linux boots,
* Load the machine remoteproc. If Linux runs as remoteproc master, you will need to pass the other processor's echo_test binary as firmware arguement to the remoteproc module.
* If you run the Linux kernel application demo, load the `rpmsg_echo_test_kern_app` module. You will see the kernel application send the message to remote and the remote reply back and the kernel application will verify the result.
* If you run the userspace application demo, load the `rpmsg_user_dev_driver` module.
* If you run the userspace application demo, you will see the similar output on the console:
```
****************************************
 Please enter command and press enter key
 ****************************************
 1 - Send data to remote core, retrieve the echo and validate its integrity ..
 2 - Quit this application ..
 CMD>
```
* Input `1` to send packages.
* Input `2` to exit the application.

After you run the demo, you will need to unload the kernel modules.

### Unload the Demo
* If you run the userspace application demo, unload the `rpmsg_user_dev_driver` module.
* If you run the kernelspace application demo, unload the `rpmsg_echo_test_kern_app` module.
* Unload the machine remoteproc driver.

