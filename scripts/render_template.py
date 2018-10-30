#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from jinja2 import Environment, FileSystemLoader, BaseLoader, DictLoader
import sys
import os
sys.path.append(os.path.dirname(sys.argv[0])+'/dts/')
from dts.edtsdatabase import EDTSConsumerMixin
import ntpath
import json
from codegen.options import Options

# Capture our current directory
THIS_DIR = os.path.dirname(os.path.abspath(__file__))

class CodeGen(EDTSConsumerMixin):

    def __init__(self):
        self.data = dict()
        self.options = Options(self.data)
        self.template_handle = None
        self.edts_api = {}
        self.device = None

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

        self.load(self.data['runtime']['defines']['GENERATED_EDTS'])

        with open(self.data['runtime']['defines']['GENERATED_EDTS']) as f:
            dts = json.load(f)

        self.data['devicetree'] = dts

    def setup_template_env(self):
        input_path, input_filename = ntpath.split(self.data['runtime']['input_file'])

        self.include_paths = []
        self.include_paths.append(input_path)
        self.include_paths.extend(self.data['runtime']['include_path'])
 
        self.loader = FileSystemLoader(self.include_paths)
        self.template_env = Environment(loader=self.loader, extensions=['jinja2.ext.do','jinja2.ext.loopcontrols'])

        self.template_handle = self.template_env.get_template(input_filename)
 
    def setup_edts_database_api(self):
        self.edts_api.update({'get_compatibles':self.get_compatibles})
        self.edts_api.update({'get_device_ids_by_compatible':self.get_device_ids_by_compatible})
        self.edts_api.update({'get_device_by_device_id':self.get_device_by_device_id})

        self.template_env.globals.update(edts_api=self.edts_api)


    def write_generated_output(self, output_str):
        output_handle = open(self.data['runtime']['output_name'], "w")
        output_handle.write(output_str)
        output_handle.close()

    def render(self):
        return self.template_handle.render(data=self.data, edts_api=self.edts_api) 

    def main(self, argv):

        argv = argv[1:]

        self.options.parse_args(argv)
        self.create_datacontext()
        self.setup_template_env()
        self.setup_edts_database_api()
     
        self.write_generated_output(self.render())

if __name__ == '__main__':

    ret = CodeGen().main(sys.argv)

