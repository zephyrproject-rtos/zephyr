#!/bin/sh
../../extract_dts_includes.py -d ./edts.dts -y ../bindings/ -e ./reg.edts -i /dev/null -k /dev/null
../../extract_dts_includes.py -d ./reg.dts -y ../bindings/ -e ./test.edts -i /dev/null -k /dev/null
