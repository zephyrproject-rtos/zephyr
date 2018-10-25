#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from jinja2 import Environment, FileSystemLoader, BaseLoader, DictLoader
import ntpath
import os
import json
import sys
from codegen.options import Options

# Capture our current directory
THIS_DIR = os.path.dirname(os.path.abspath(__file__))

class CodeGen():

    def __init__(self):
        self.data = dict()
        self.options = Options(self.data)
        self.template_handle = None

    def create_datacontext(self):
        self.create_config_context()
        self.load_edts_database()  

    def create_config_context(self):
        truefalse = {"y" : "true", "n" : "false"}
        self.data['config'] = dict()

        with open(self.data['runtime']['defines']['PROJECT_BINARY_DIR']+"/.config") as f:
            configlines = f.readlines()
            for confline in configlines:
                if not confline.startswith('#') and confline.strip():
                    config_item = confline.strip().split("=")

                    if len(config_item[1]) == 1:
                        if ("y" in config_item[1]) or ("n" in config_item[1]):
                            self.data['config'].update({config_item[0]:truefalse[config_item[1]]})
                        else:
                            self.data["config"].update({config_item[0]:config_item[1].strip()})
                    else:
                        self.data["config"].update({config_item[0]: config_item[1].strip().replace('"','')})
                        
    def load_edts_database(self):
        with open(self.data['runtime']['defines']['GENERATED_EDTS']) as f:
            dts = json.load(f)

        self.data['devicetree'] = dts

    def write_generated_output(self, output_str):
        output_handle = open(self.data['runtime']['output_name'], "w")
        output_handle.write(output_str)
        output_handle.close()

    def render(self):
        return self.template_handle.render(data=self.data) 

    def main(self, argv):

        argv = argv[1:]

        self.options.parse_args(argv)
        self.create_datacontext()
     
        input_path, input_filename = ntpath.split(self.data['runtime']['input_file'])

        include_paths = []
        include_paths.append(input_path)
        include_paths.extend(self.data['runtime']['include_path'])
 
        loader = FileSystemLoader(include_paths)
        env = Environment(loader=loader, extensions=['jinja2.ext.do','jinja2.ext.loopcontrols'])
        self.template_handle = env.get_template(input_filename)

        self.write_generated_output(self.render())

if __name__ == '__main__':

    ret = CodeGen().main(sys.argv)

