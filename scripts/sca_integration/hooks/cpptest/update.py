#!/usr/bin/env python3

import subprocess
import sys
import json

print( "Exectuing: '" + str(sys.argv[0:]) + "'", file=sys.stderr)

# Do some useful processing of sca_export.json as passed in argv[1]

file = open(sys.argv[1])
data = json.load(file)

print("Write/update 'extern CPPTEST_CC=\"" + data['Build']['C_COMPILER'] + "\"' in your ~/.bashrc or similar", file=sys.stderr)

file.close()


sys.exit(0)

