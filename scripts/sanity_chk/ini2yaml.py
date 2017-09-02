#!/usr/bin/env python

import ConfigParser, os
import yaml
import sys


sample = False
in_file = sys.argv[1]
if sys.argv[2] == 'sample':
    sample = True

out_file = os.path.join(os.path.dirname(in_file), sys.argv[2] + ".yaml")

config = ConfigParser.ConfigParser()
config.readfp(open(sys.argv[1]))
y = {'tests': 'tests'}

tests = []
for section in config.sections():
    tc = {}
    for opt in config.options(section):
        value = config.get(section, opt)
        if value in ['false', 'true']:
            tc[opt] = True if value == 'true' else False
        else:
            tc[opt] = value

    test = { section : tc}
    tests.append(test)

y['tests'] = tests
if sample:
    y['sample'] = { 'name': "TBD", 'description': "TBD" }

with open(out_file, "w") as f:
    yaml.dump(y, f, width=50, indent=4,  default_flow_style=False)
