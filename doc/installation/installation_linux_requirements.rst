.. _Requirements:

Requirements
############

Install the following requirements on the build host system using either
apt-get or yum.

.. note::
   Minor version updates of the listed required packages might also
   work.

.. _GeneralDevelopment:

Packages Required for General Development
#########################################

Install the required packages in a Ubuntu host system, type:

.. code-block:: bash

   $ sudo apt-get install git make gcc gcc-multilib g++ \
     libc6-dev-i386 g++-multilib

Install the required packages in a Fedora host system, type:

.. code-block:: bash

   $ sudo yum groupinstall "Development Tools"
   $ sudo yum install git make gcc glib-devel.i686 \
     glib2-devel.i686 g++ libc6-dev-i386 g++-multilib \
     glibc-static libstdc++-static

