#!/bin/sh
../../extract_dts_includes.py -d ./edts.dts -y ../bindings/ -e ./compat.edts -i /dev/null -k /dev/null
../../extract_dts_includes.py -d ./compat.dts -y ../bindings/ -e ./test.edts -i /dev/null -k /dev/null
