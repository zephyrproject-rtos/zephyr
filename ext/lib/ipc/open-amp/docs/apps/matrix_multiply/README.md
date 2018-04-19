
# matrix_multiply 
This readme is about the OpenAMP matrix_multiply demo.
The matrix_multiply is about one processor generates two matrices, and send them to the one, and the other one calcuate the matrix multiplicaiton and return the result matrix.

For now, it implements Linux generates the matrices, and the baremetal calculate the matrix mulitplication and send back the result.

## Compilation

### Baremetal Compilation
Option `WITH_MATRIX_MULTIPLY` is to control if the application will be built.
By default this option is `ON` when `WITH_APPS` is on.

Here is an example:

```
$ cmake ../open-amp -DCMAKE_TOOLCHAIN_FILE=zynq7_generic -DWITH_OBSOLETE=on -DWITH_APPS=ON
```

### Linux Compilation

#### Linux Kernel Compilation
You will need to manually compile the following kernel modules with your Linux kernel (Please refer to Linux kernel documents for how to add kernel module):

* Your machine's remoteproc kernel driver
* `obsolete/system/linux/kernelspace/rpmsg_user_dev_driver` if you want to run the matrix_multiply app in Linux user space.
* `obsolete/apps/matrix_multiply/system/linux/kernelspace/rpmsg_mat_mul_kern_app` if you want to run the matrix_multiply app in Linux kernel space.

#### Linux Userspace Compliation
* Compile `obsolete/apps/matrix_multiply/system/linux/userspace/mat_mul_demo` into your Linux OS.
* If you are running generic(baremetal) system as remoteproc slave, and Linux as remoteproc master, please also add the built generic `matrix_multiply` executable to the firmware of your Linux OS.

## Run the Demo

### Load the Demo
After Linux boots,
* Load the machine remoteproc. If Linux runs as remoteproc master, you will need to pass the other processor's matrix_multiply binary as firmware arguement to the remoteproc module.
* If you run the Linux kernel application demo, load the `rpmsg_mat_mul_kern_app` module, you will see the kernel app will generate two matrices to the other processor, and output the result matrix returned by the other processor.
* If you run the userspace application demo, load the `rpmsg_user_dev_driver` module.
* If you run the userspace application demo `mat_mul_demo`, you will see the similar output on the console:
```
****************************************
Please enter command and press enter key
****************************************
1 - Generates random 6x6 matrices and transmits them to remote core over rpmsg
..
2 - Quit this application ..
CMD>
```
* Input `1` to run the matrix multiplication.
* Input `2` to exit the application.

After you run the demo, you will need to unload the kernel modules.

### Unload the Demo
* If you run the userspace application demo, unload the `rpmsg_user_dev_driver` module.
* If you run the kernelspace application demo, unload the `rpmsg_mat_mul_kern_app` module.
* Unload the machine remoteproc driver.

