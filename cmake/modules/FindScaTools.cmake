# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA

#[=======================================================================[.rst:
FindScaTools
-------------------

Find and load Static Code Analysis (SCA) tools for Zephyr.

This module locates and loads the specified Static Code Analysis (SCA) tool
based on the ``ZEPHYR_SCA_VARIANT`` variable. It searches for the tool implementation
in the ``cmake/sca/<variant>/sca.cmake`` file within the directories specified in
``SCA_ROOT``.

Variables Affecting Behavior
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. cmake:variable:: ZEPHYR_SCA_VARIANT

   Specifies which SCA tool variant to use. This variable must be set when
   ``find_package(ScaTools REQUIRED)`` is called.

.. cmake:variable:: SCA_ROOT

   A list of directories where SCA tools may be found. The module searches for
   ``cmake/sca/${ZEPHYR_SCA_VARIANT}/sca.cmake`` in each of these directories.
   By default, it includes ``${ZEPHYR_BASE}`` at the lowest priority.

Search Process
^^^^^^^^^^^^^

The module searches for the SCA tool implementation in the following order:

1. In each directory listed in ``SCA_ROOT``, looking for
   ``<dir>/cmake/sca/${ZEPHYR_SCA_VARIANT}/sca.cmake``
2. If found, the implementation is included and the search stops
3. If not found, a fatal error is raised

Example Usage
^^^^^^^^^^^^

.. code-block:: cmake

   # Set the SCA tool variant
   set(ZEPHYR_SCA_VARIANT "cppcheck")

   # Find the SCA tools
   find_package(ScaTools REQUIRED)

   # The SCA tool's functionality is now available

#]=======================================================================]

# 'SCA_ROOT' is a prioritized list of directories where SCA tools may
# be found. It always includes ${ZEPHYR_BASE} at the lowest priority.
list(APPEND SCA_ROOT ${ZEPHYR_BASE})

zephyr_get(ZEPHYR_SCA_VARIANT)

if(ScaTools_FIND_REQUIRED AND NOT DEFINED ZEPHYR_SCA_VARIANT)
  message(FATAL_ERROR "ScaTools required but 'ZEPHYR_SCA_VARIANT' is not set. "
    "Please set 'ZEPHYR_SCA_VARIANT' to desired tool.")
endif()

if(NOT DEFINED ZEPHYR_SCA_VARIANT)
  return()
endif()

foreach(root ${SCA_ROOT})
  if(EXISTS ${root}/cmake/sca/${ZEPHYR_SCA_VARIANT}/sca.cmake)
    include(${root}/cmake/sca/${ZEPHYR_SCA_VARIANT}/sca.cmake)
    return()
  endif()
endforeach()

message(FATAL_ERROR "ZEPHYR_SCA_VARIANT set to '${ZEPHYR_SCA_VARIANT}' but no "
  "implementation for '${ZEPHYR_SCA_VARIANT}' found. SCA_ROOTs searched: ${SCA_ROOT}")
