#!/bin/bash

# run the filter-known-issues.py script to remove "expected" warning
# messages from the output of the document build process and write
# the filtered output to stdout
#
# Only argument is the name of the log file saved by the build.

KI_SCRIPT=${ZEPHYR_BASE}/scripts/filter-known-issues.py
CONFIG_DIR=${ZEPHYR_BASE}/.known-issues/doc

LOG_FILE=$1

if [ -z "${LOG_FILE}" ]; then
        echo "Error in $0: missing input parameter <logfile>"
        exit 1
fi

if [ -e "${LOG_FILE}" ]; then
   if [ -s "${LOG_FILE}" ]; then
      $KI_SCRIPT --config-dir ${CONFIG_DIR} ${LOG_FILE}
   fi
else
   echo "Error in $0: logfile \"${LOG_FILE}\" not found."
   exit 1
fi
