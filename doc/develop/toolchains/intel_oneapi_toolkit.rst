.. _toolchain_intel_oneapi_toolkit:

Intel oneAPI Toolkit
####################

#. Download `Intel oneAPI Base Toolkit
   <https://software.intel.com/content/www/us/en/develop/tools/oneapi/all-toolkits.html>`_

#. Assuming the toolkit is installed in ``/opt/intel/oneApi``, set environment
   using::

        # Linux, macOS:
        export ONEAPI_TOOLCHAIN_PATH=/opt/intel/oneapi
        source $ONEAPI_TOOLCHAIN_PATH/compiler/latest/env/vars.sh

        # Windows:
        > set ONEAPI_TOOLCHAIN_PATH=C:\Users\Intel\oneapi

   To setup the complete oneApi environment, use::

        source  /opt/intel/oneapi/setvars.sh

   The above will also change the python environment to the one used by the
   toolchain and might conflict with what Zephyr uses.

#. Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``oneApi``.
