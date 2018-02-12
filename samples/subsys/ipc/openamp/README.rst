.. _openAMP_example:

OpenAMP Example Application
###########################

Overview
********

This application demostrates how to use OpenAMP with Zephyr. It is designed to
be an OpenAMP counterpart to the RPMsg-Lite example for the LPCXpresso5411x
chip, to be used for comparison and benchmarking of the two RPC libraries.

Building the application
*************************

Building and running this application requires that libmetal and OpenAMP are
compiled first. Clone the libmetal and OpenAMP repositories in an appropriate
location (outside the Zephyr code tree):

.. code-block:: bash

   $ git clone https://github.com/OpenAMP/libmetal.git
   $ git clone https://github.com/OpenAMP/open-amp.git

Compile and build libmetal for both cores:

.. code-block:: bash

   $ cd libmetal
   $ mkdir build-master build-remote
   $ cd build-master
   $ cmake -DWITH_ZEPHYR=ON -DBOARD=lpcxpresso54114 ..
   $ make
   $ cd ../build-remote
   $ cmake -DWITH_ZEPHYR=ON -DBOARD=lpcxpresso54114_m0 ..
   $ make

Compile and build OpenAMP for both cores:

.. code-block:: bash

   $ cd ../../open-amp
   $ mkdir build-master build-remote
   $ cd build-master
   $ cmake -DWITH_ZEPHYR=ON -DWITH_PROXY=OFF -DBOARD=lpcxpresso54114 \
       -DLIBMETAL_INCLUDE_DIR=<path to libmetal>/build-master/lib/include \
       -DLIBMETAL_LIB=<path to libmetal>/build-master/lib/libmetal.a ..
   $ make
   $ cd ../build-remote
   $ cmake -DWITH_ZEPHYR=ON -DWITH_PROXY=OFF -DBOARD=lpcxpresso54114_m0 \
       -DLIBMETAL_INCLUDE_DIR=<path to libmetal>/build-remote/lib/include \
       -DLIBMETAL_LIB=<path to libmetal>/build-remote/lib/libmetal.a ..
   $ make

Compile the remote application, by running the following commands:

.. code-block:: bash

   $ cd $ZEPHYR_BASE/samples/subsys/ipc/openamp/remote
   $ mkdir build
   $ cmake -DBOARD=lpcxpresso54114_m0 \
       -DLIBMETAL_INCLUDE_DIR=<path to libmetal>/build-remote/lib/include \
       -DLIBMETAL_LIBRARY=<path to libmetal>/build-remote/lib/libmetal.a \
       -DOPENAMP_INCLUDE_DIR=<path to openamp>/lib/include \
       -DOPENAMP_LIBRARY=<path to openamp>/build-remote/lib/libopen_amp.a ..
   $ make

Compile the master application, by running the following commands:

.. code-block:: bash

   $ cd $ZEPHYR_BASE/samples/subsys/ipc/openamp/master
   $ mkdir build
   $ cmake -DBOARD=lpcxpresso54114 \
       -DLIBMETAL_INCLUDE_DIR=<path to libmetal>/build-master/lib/include \
       -DLIBMETAL_LIBRARY=<path to libmetal>/build-master/lib/libmetal.a \
       -DOPENAMP_INCLUDE_DIR=<path to openamp>/lib/include \
       -DOPENAMP_LIBRARY=<path to openamp>/build-master/lib/libopen_amp.a ..
   $ make

Flash the project to the board from the master build directory:

.. code-block:: bash

   $ make flash

