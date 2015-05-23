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

   $ sudo apt-get install git make gcc gcc-multilib g++
   libc6-dev-i386 g++-multilib

Install the required packages in a Fedora host system, type:

.. code-block:: bash

   $ sudo yum groupinstall General Development gitmake
   gccglib-devel.i686 glib2-devel.i686 g++ libc6-dev-i386g++-multilib
   glibc-static libstdc++-static

Packages Required for Building Crosstool-next Generation (ct-ng)
################################################################

Your host system must have the following packages for crosstool-next
generation (ct-ng):

.. code-block:: bash

   $ sudo apt-get install gperf gawk bison flex texinfo libtool
   automake ncurses- devexpat libexpat1-dev libexpat1 python-dev

Install libtool-bin for Debian systems, type:

.. code-block:: bash

   $ sudo apt- get install libtool-bin

Requirements for building ARC
*****************************
Install the needed packages for building ARC in Ubuntu, type:

.. code-block:: bash

   $ sudo apt-get install texinfo byacc flex libncurses5-dev
   zlib1g-dev libexpat1-dev libx11-dev texlive build-essential

Install the needed packages for building ARC in Fedora, type:

.. code-block:: bash

   $ sudo yum groupinstall Development Tools

   $ sudo yum install texinfo-tex byacc flex ncurses-devel zlib-devel expat-
   devel libX11-devel git

Optional Packages for Crosstool-next Generation building
########################################################

The following packages are optional since the first crosstool-next
generation build downloads them if they are not installed.

Install the optional packages on your host system manually, type:

.. code-block:: bash

   $ sudo apt-get install gmp mpfr isl cloog mpc binutils
