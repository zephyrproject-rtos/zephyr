.. Zephyr:code-sample:: ZVM
    :name: ZVM (Zephyr-based Virtual Machine)

    Boot virtual machines (VMs) with ZVM.
    This sample support run two VMs
    using ZVM on the QEMU MAX board.

Overview
*********************************

ZVM (Zephyr-based Virtual Machine) is a new generation of Type 1.5 embedded
RTOS (real-time operating system) virtualization solution, jointly developed
with the open-source RTOS Zephyr. ZVM can deploy multiple operating systems
in a secure and isolated manner on a single hardware, providing real-time and
flexible virtualization support for multi-OS and multi-tasking.
Type 1.5 is not a compromise between low latency (Type 1) and high flexibility (Type 2),
but achieves the best of both worlds without sacrificing either:

- ZVM does not run on Zephyr, but directly on the hardware, sharing the driver support and scheduling capabilities of the Zephyr kernel (i.e. Type 1.5 is more flexible than Type 1); meanwhile, ZVM avoids the latency overhead caused by multiple layers of dependency in Type 2 (i.e. Type 1.5 has lower latency than Type 2).

- By combining the real-time scheduling mechanism of Zephyr RTOS and the task isolation mechanism of ZVM, real-time tasks will not be interfered with by low-priority tasks (i.e. Type 1.5 has lower latency than Type 1 and Type 2).

Building and Running
*********************************

Building ZVM for ARMv8.1+ Cores boards
======================================

ZVM requires support for ARMv8.1+ cores, such as Cortex-A55 and A76 processors. The sample can be built as follows:

.. zephyr-app-commands::
    :zephyr-app: samples/subsys/zvm
    :board: qemu_max
    :goals: build
    :compact:

For other ARMv8.1+ compatible boards, you need to add the corresponding overlay files in the samples/subsys/zvm/boards directory.

you can build the ZVM with the following command:

.. code-block:: shell

    west build -b qemu_max/qemu_max/smp samples/subsys/zvm/

Running VM with ZVM
====================================

1. Get the VM image files
--------------------------------------

We provide pre-built images that can be executed directly on the platform.
Use the following method to pull the pre-built images. First, enter the home directory:

.. code-block:: shell

    cd ~
    git clone https://github.com/hnu-esnl/zvm_images.git

Then copy the zvm_host.elf images from the Zephyr repository to ~/zvm_images:

.. code-block:: shell

    cp Zephyr_DIR/build/zephyr/zvm_host.elf ~/zvm_images/qemu_arm64

2. Boot ZVM with pre-written script file
--------------------------------------------

Use auto_zvm.sh to run the ZVM:

.. code-block:: shell

    cd ~/zvm_images/qemu_arm64/
    ./auto_zvm.sh debugserver qemu_max_smp

The following output is printed and you can use commands to create and run the VMs:

Sample output
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: shell

    *** Booting Zephyr OS build 9f824289b28d ***
    Secondary CPU core 1 (MPID:0x1) is up
    Secondary CPU core 2 (MPID:0x2) is up
    Secondary CPU core 3 (MPID:0x3) is up

    █████████╗    ██╗     ██╗    ███╗    ███╗
    ╚════███╔╝    ██║     ██║    ████╗  ████║
       ███╔╝      ╚██╗   ██╔╝    ██╔ ████╔██║
      ██╔╝         ╚██  ██╔╝     ██║ ╚██╔╝██║
    █████████╗      ╚████╔╝      ██║  ╚═╝ ██║
    ╚════════╝        ╚═╝        ╚═╝      ╚═╝

    zvm_host:~#

3. Launching and Connecting to the Corresponding VM:
------------------------------------------------------------

In the ZVM window, enter the following command to view the supported commands on the platform:

.. code-block:: shell

    zvm help

Launching Zephyr Virtual Machine
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1) Create a Zephyr VM:

.. code-block:: shell

    zvm new -t zephyr

2) Run the Zephyr VM:

.. code-block:: shell

    zvm run -n 0

(-n followed by the VM's corresponding ID, assuming the created VM's VM-ID is 0)

3) Enter the Zephyr VM UART console:

.. code-block:: shell

    zvm look 0

4) Exit the Zephyr VM:

Enter the following command in the console:

    Ctrl + x

.. note::

    The ZVM project is created and led by Professor Guoqi Xie at Hunan University, China.
    We would like to express our gratitude to the collaborators for their contributions
    to this project. The main developers are as follows:

    - Guoqi Xie, email: xgqman@hnu.edu.cn
    - Chenglai Xiong (openEuler sig-Zephyr Maintainer), email: xiongcl@hnu.edu.cn
    - Wei Ren (openEuler sig-Zephyr Maintainer), email: dfrd-renw@dfmc.com.cn
    - Xingyu Hu, email: huxingyu@hnu.edu.cn
    - Yuhao Hu, email: ahui@hun.edu.cn

    For more information, see the `ZVM Main page <https://gitee.com/openeuler/zvm>`__.
