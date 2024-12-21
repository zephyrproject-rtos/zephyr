# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 Intel Corporation

import yaml
import ijson
import json
import re
import argparse
import xlsxwriter
class Json_report:

    json_object = {
        "components":[]
    }

    simulators = [
        'unit_testing',
        'native',
        'qemu',
        'mps2/an385'
    ]

    report_json = {}

    def __init__(self):
        args = parse_args()
        self.parse_testplan(args.testplan)
        self.maintainers_file = self.get_maintainers_file( args.maintainers)
        self.report_json = self.generate_json_report( args.coverage)

        if args.format == "json":
            self.save_json_report( args.output, self.report_json)
        elif args.format == "xlsx":
            self.generate_xlsx_report(self.report_json, args.output)
        elif args.format == "all":
            self.save_json_report( args.output, self.report_json)
            self.generate_xlsx_report(self.report_json, args.output)
        else:
            print("Format incorrect")

    def get_maintainers_file(self, maintainers):
        maintainers_file = ""
        with open(maintainers, 'r') as file:
            maintainers_file = yaml.safe_load(file)
            file.close()
        return maintainers_file


    def parse_testplan(self, testplan_path):
        with open(testplan_path, 'r') as file:
            parser = ijson.items(file, 'testsuites')
            for element in parser:
                for testsuite in element:
                    for testcase in testsuite['testcases']:
                        if testcase['status'] == 'None':
                            testcase_name = testcase['identifier']
                            component_name = testcase_name[:testcase_name.find('.')]
                            component = {
                                "name": component_name,
                                "sub_components":[],
                                "files":[]
                            }
                            features = self.json_object['components']
                            known_component_flag = False
                            for item in features:
                                if component_name == item['name']:
                                    component = item
                                    known_component_flag = True
                                    break
                            sub_component_name = testcase_name[testcase_name.find('.'):]
                            sub_component_name = sub_component_name[1:]
                            if sub_component_name.find(".") > 0:
                                sub_component_name = sub_component_name[:sub_component_name.find(".")]
                            if known_component_flag is False:

                                sub_component = {
                                    "name":sub_component_name,
                                    "test_suites":[]
                                }
                                test_suite = {
                                    "name":testsuite['name'],
                                    "path":testsuite['path'],
                                    "platforms":[],
                                    "runnable": testsuite['runnable'],
                                    "status":"",
                                    "test_cases":[]
                                }
                                test_case = {
                                    "name":testcase_name
                                }
                                if any(platform in testsuite['platform'] for platform in self.simulators):
                                    if test_suite['status'] == "":
                                        test_suite['status'] = 'sim_only'

                                    if test_suite['status'] == 'hw_only':
                                        test_suite['status'] = 'mixed'
                                else:
                                    if test_suite['status'] == "":
                                        test_suite['status'] = 'hw_only'

                                    if test_suite['status'] == 'sim_only':
                                        test_suite['status'] = 'mixed'
                                test_suite['test_cases'].append(test_case)
                                test_suite['platforms'].append(testsuite['platform'])
                                sub_component["test_suites"].append(test_suite)
                                component['sub_components'].append(sub_component)
                                self.json_object['components'].append(component)
                            else:
                                sub_component = {}
                                sub_components = component['sub_components']
                                known_sub_component_flag = False
                                for i_sub_component in sub_components:
                                    if sub_component_name == i_sub_component['name']:
                                        sub_component = i_sub_component
                                        known_sub_component_flag = True
                                        break
                                if known_sub_component_flag is False:
                                    sub_component = {
                                        "name":sub_component_name,
                                        "test_suites":[]
                                    }
                                    test_suite = {
                                        "name":testsuite['name'],
                                        "path":testsuite['path'],
                                        "platforms":[],
                                        "runnable": testsuite['runnable'],
                                        "status":"",
                                        "test_cases":[]
                                    }
                                    test_case = {
                                        "name": testcase_name
                                    }
                                    if any(platform in testsuite['platform'] for platform in self.simulators):
                                        if test_suite['status'] == "":
                                            test_suite['status'] = 'sim_only'

                                        if test_suite['status'] == 'hw_only':
                                            test_suite['status'] = 'mixed'
                                    else:
                                        if test_suite['status'] == "":
                                            test_suite['status'] = 'hw_only'

                                        if test_suite['status'] == 'sim_only':
                                            test_suite['status'] = 'mixed'
                                    test_suite['test_cases'].append(test_case)
                                    test_suite['platforms'].append(testsuite['platform'])
                                    sub_component["test_suites"].append(test_suite)
                                    component['sub_components'].append(sub_component)
                                else:
                                    test_suite = {}
                                    test_suites = sub_component['test_suites']
                                    known_testsuite_flag = False
                                    for i_testsuite in test_suites:
                                        if testsuite['name'] == i_testsuite['name']:
                                            test_suite = i_testsuite
                                            known_testsuite_flag = True
                                            break
                                    if known_testsuite_flag is False:
                                        test_suite = {
                                            "name":testsuite['name'],
                                            "path":testsuite['path'],
                                            "platforms":[],
                                            "runnable": testsuite['runnable'],
                                            "status":"",
                                            "test_cases":[]
                                        }
                                        test_case  = {
                                            "name": testcase_name
                                        }
                                        if any(platform in testsuite['platform'] for platform in self.simulators):
                                            if test_suite['status'] == "":
                                                test_suite['status'] = 'sim_only'

                                            if test_suite['status'] == 'hw_only':
                                                test_suite['status'] = 'mixed'
                                        else:
                                            if test_suite['status'] == "":
                                                test_suite['status'] = 'hw_only'

                                            if test_suite['status'] == 'sim_only':
                                                test_suite['status'] = 'mixed'
                                        test_suite['test_cases'].append(test_case)
                                        test_suite['platforms'].append(testsuite['platform'])
                                        sub_component["test_suites"].append(test_suite)
                                    else:
                                        if any(platform in testsuite['platform'] for platform in self.simulators):
                                            if test_suite['status'] == "":
                                                test_suite['status'] = 'sim_only'

                                            if test_suite['status'] == 'hw_only':
                                                test_suite['status'] = 'mixed'
                                        else:
                                            if test_suite['status'] == "":
                                                test_suite['status'] = 'hw_only'

                                            if test_suite['status'] == 'sim_only':
                                                test_suite['status'] = 'mixed'
                                        test_case = {}
                                        test_cases = test_suite['test_cases']
                                        known_testcase_flag = False
                                        for i_testcase in test_cases:
                                            if testcase_name == i_testcase['name']:
                                                test_case = i_testcase
                                                known_testcase_flag = True
                                                break
                                        if known_testcase_flag is False:
                                            test_case = {
                                                "name":testcase_name
                                            }
                                            test_suite['test_cases'].append(test_case)
            file.close()

    def get_files_from_maintainers_file(self, component_name):
        files_path = []
        for item in self.maintainers_file:
            _found_flag = False
            try:
                tests = self.maintainers_file[item].get('tests', [])
                for i_test in tests:
                    if component_name in i_test:
                        _found_flag = True

                if _found_flag is True:
                    for path in self.maintainers_file[item]['files']:
                        path = path.replace('*','.*')
                        files_path.append(path)
            except TypeError:
                print("ERROR: Fail while parsing MAINTAINERS file at %s", component_name)
        return files_path

    def generate_json_report(self, coverage):
        output_json = {
            "components":[]
        }

        with open(coverage, 'r') as file:
            parser = ijson.items(file, 'files')
            for element in parser:

                for i_json_component in self.json_object['components']:
                    json_component = {}
                    json_component["name"]=i_json_component["name"]
                    json_component["sub_components"] = i_json_component["sub_components"]
                    json_component["Comment"] = ""
                    files_path = []
                    files_path = self.get_files_from_maintainers_file(i_json_component["name"])
                    json_files = []
                    if len(files_path) != 0:
                        for i_file in files_path:
                            for i_covered_file in element:
                                x = re.search(('.*'+i_file+'.*'), i_covered_file['file'])
                                if x:
                                    file_name =  i_covered_file['file'][i_covered_file['file'].rfind('/')+1:]
                                    file_path = i_covered_file['file']
                                    file_coverage, file_lines, file_hit = self._calculate_coverage_of_file(i_covered_file)
                                    json_file = {
                                            "Name":file_name,
                                            "Path":file_path,
                                            "Lines": file_lines,
                                            "Hit":file_hit,
                                            "Coverage": file_coverage,
                                            "Covered_Functions": [],
                                            "Uncovered_Functions": []
                                        }
                                    for i_fun in i_covered_file['functions']:
                                        if i_fun['execution_count'] != 0:
                                            json_covered_funciton ={
                                                "Name":i_fun['name']
                                            }
                                            json_file['Covered_Functions'].append(json_covered_funciton)
                                    for i_fun in i_covered_file['functions']:
                                        if i_fun['execution_count'] == 0:
                                            json_uncovered_funciton ={
                                                "Name":i_fun['name']
                                            }
                                            json_file['Uncovered_Functions'].append(json_uncovered_funciton)
                                    comp_exists = [x for x in json_files if x['Path'] == json_file['Path']]
                                    if not comp_exists:
                                        json_files.append(json_file)
                        json_component['files']=json_files
                        output_json['components'].append(json_component)
                    else:
                        json_component["files"] = []
                        json_component["Comment"] = "Missed in maintainers.yml file."
                        output_json['components'].append(json_component)

        return output_json

    def _calculate_coverage_of_file(self, file):
        tracked_lines = len(file['lines'])
        covered_lines = 0
        for line in file['lines']:
            if line['count'] != 0:
                covered_lines += 1
        return ((covered_lines/tracked_lines)*100), tracked_lines, covered_lines

    def save_json_report(self, output_path, json_object):
        json_object = json.dumps(json_object, indent=4)
        with open(output_path+'.json', "w") as outfile:
            outfile.write(json_object)

    def _find_char(self, path, str, n):
        sep = path.split(str, n)
        if len(sep) <= n:
            return -1
        return len(path) - len(sep[-1]) - len(str)

    def _component_calculate_stats(self, json_component):
        testsuites_count = 0
        runnable_count = 0
        build_only_count = 0
        sim_only_count = 0
        hw_only_count = 0
        mixed_count = 0
        for i_sub_component in json_component['sub_components']:
            for i_testsuit in i_sub_component['test_suites']:
                testsuites_count += 1
                if i_testsuit['runnable'] is True:
                    runnable_count += 1
                else:
                    build_only_count += 1

                if i_testsuit['status'] == "hw_only":
                    hw_only_count += 1
                elif i_testsuit['status'] == "sim_only":
                    sim_only_count += 1
                else:
                    mixed_count += 1
        return testsuites_count, runnable_count, build_only_count, sim_only_count, hw_only_count, mixed_count

    def _xlsx_generate_summary_page(self, workbook, json_report):
        # formats
        header_format = workbook.add_format(
            {
                "bold": True,
                "fg_color":  "#538DD5",
                "color":"white"
            }
        )
        cell_format = workbook.add_format(
            {
                'valign': 'vcenter'
            }
        )

        #generate summary page
        worksheet = workbook.add_worksheet('Summary')
        row = 0
        col = 0
        worksheet.write(row,col,"Components",header_format)
        worksheet.write(row,col+1,"TestSuites",header_format)
        worksheet.write(row,col+2,"Runnable",header_format)
        worksheet.write(row,col+3,"Build only",header_format)
        worksheet.write(row,col+4,"Simulators only",header_format)
        worksheet.write(row,col+5,"Hardware only",header_format)
        worksheet.write(row,col+6,"Mixed",header_format)
        worksheet.write(row,col+7,"Coverage [%]",header_format)
        worksheet.write(row,col+8,"Total Functions",header_format)
        worksheet.write(row,col+9,"Uncovered Functions",header_format)
        worksheet.write(row,col+10,"Comment",header_format)
        row = 1
        col = 0
        for item in json_report['components']:
            worksheet.write(row, col, item['name'],cell_format)
            testsuites,runnable,build_only,sim_only,hw_only, mixed= self._component_calculate_stats(item)
            worksheet.write(row,col+1,testsuites,cell_format)
            worksheet.write(row,col+2,runnable,cell_format)
            worksheet.write(row,col+3,build_only,cell_format)
            worksheet.write(row,col+4,sim_only,cell_format)
            worksheet.write(row,col+5,hw_only,cell_format)
            worksheet.write(row,col+6,mixed,cell_format)
            lines = 0
            hit = 0
            coverage = 0.0
            total_funs = 0
            uncovered_funs = 0
            for i_file in item['files']:
                lines += i_file['Lines']
                hit += i_file['Hit']
                total_funs += (len(i_file['Covered_Functions'])+len(i_file['Uncovered_Functions']))
                uncovered_funs += len(i_file['Uncovered_Functions'])

            if lines != 0:
                coverage = (hit/lines)*100

            worksheet.write_number(row,col+7,coverage,workbook.add_format({'num_format':'#,##0.00'}))
            worksheet.write_number(row,col+8,total_funs)
            worksheet.write_number(row,col+9,uncovered_funs)
            worksheet.write(row,col+10,item["Comment"],cell_format)
            row += 1
            col = 0
        worksheet.conditional_format(1,col+7,row,col+7, {'type':     'data_bar',
                                                            'min_value':  0,
                                                            'max_value':  100,
                                                            'bar_color': '#3fd927',
                                                            'bar_solid': True,
                                                                })
        worksheet.autofit()
        worksheet.set_default_row(15)

    def generate_xlsx_report(self, json_report, output):
        self.report_book = xlsxwriter.Workbook(output+".xlsx")
        header_format = self.report_book.add_format(
            {
                "bold": True,
                "fg_color":  "#538DD5",
                "color":"white"
            }
        )

        # Create a format to use in the merged range.
        merge_format = self.report_book.add_format(
            {
                "bold": 1,
                "align": "center",
                "valign": "vcenter",
                "fg_color":  "#538DD5",
                "color":"white"
            }
        )
        cell_format = self.report_book.add_format(
            {
                'valign': 'vcenter'
            }
        )

        self._xlsx_generate_summary_page(self.report_book, self.report_json)
        row = 0
        col = 0
        for item in json_report['components']:
            worksheet = self.report_book.add_worksheet(item['name'])
            row = 0
            col = 0
            worksheet.write(row,col,"File Name",header_format)
            worksheet.write(row,col+1,"File Path",header_format)
            worksheet.write(row,col+2,"Coverage [%]",header_format)
            worksheet.write(row,col+3,"Lines",header_format)
            worksheet.write(row,col+4,"Hits",header_format)
            worksheet.write(row,col+5,"Diff",header_format)
            row += 1
            col = 0
            for i_file in item['files']:
                worksheet.write(row,col,i_file['Path'][i_file['Path'].rfind('/')+1:],cell_format)
                worksheet.write(row,col+1,i_file["Path"][(self._find_char(i_file["Path"],'/',3)+1):],cell_format)
                worksheet.write_number(row,col+2,i_file["Coverage"],self.report_book.add_format({'num_format':'#,##0.00'}))
                worksheet.write(row,col+3,i_file["Lines"],cell_format)
                worksheet.write(row,col+4,i_file["Hit"],cell_format)
                worksheet.write(row,col+5,i_file["Lines"]-i_file["Hit"],cell_format)
                row += 1
                col = 0
            row += 1
            col = 0
            worksheet.conditional_format(1,col+2,row,col+2, {'type':     'data_bar',
                                                            'min_value':  0,
                                                            'max_value':  100,
                                                            'bar_color': '#3fd927',
                                                            'bar_solid': True,
                                                                })
            worksheet.merge_range(row,col,row,col+2, "Uncovered Functions", merge_format)
            row += 1
            worksheet.write(row,col,'Function Name',header_format)
            worksheet.write(row,col+1,'Implementation File',header_format)
            worksheet.write(row,col+2,'Comment',header_format)
            row += 1
            col = 0
            for i_file in item['files']:
                for i_uncov_fun in i_file['Uncovered_Functions']:
                    worksheet.write(row,col,i_uncov_fun["Name"],cell_format)
                    worksheet.write(row,col+1,i_file["Path"][self._find_char(i_file["Path"],'/',3)+1:],cell_format)
                    row += 1
                    col = 0
            row += 1
            col = 0
            worksheet.write(row,col,"Components",header_format)
            worksheet.write(row,col+1,"Sub-Components",header_format)
            worksheet.write(row,col+2,"TestSuites",header_format)
            worksheet.write(row,col+3,"Runnable",header_format)
            worksheet.write(row,col+4,"Build only",header_format)
            worksheet.write(row,col+5,"Simulation only",header_format)
            worksheet.write(row,col+6,"Hardware only",header_format)
            worksheet.write(row,col+7,"Mixed",header_format)
            row += 1
            col = 0
            worksheet.write(row,col,item['name'],cell_format)
            for i_sub_component in item['sub_components']:
                testsuites_count = 0
                runnable_count = 0
                build_only_count = 0
                sim_only_count = 0
                hw_only_count = 0
                mixed_count = 0
                worksheet.write(row,col+1,i_sub_component['name'],cell_format)
                for i_testsuit in i_sub_component['test_suites']:
                    testsuites_count += 1
                    if i_testsuit['runnable'] is True:
                        runnable_count += 1
                    else:
                        build_only_count += 1

                    if i_testsuit['status'] == "hw_only":
                        hw_only_count += 1
                    elif i_testsuit['status'] == "sim_only":
                        sim_only_count += 1
                    else:
                        mixed_count += 1
                worksheet.write(row,col+2,testsuites_count,cell_format)
                worksheet.write(row,col+3,runnable_count,cell_format)
                worksheet.write(row,col+4,build_only_count,cell_format)
                worksheet.write(row,col+5,sim_only_count,cell_format)
                worksheet.write(row,col+6,hw_only_count,cell_format)
                worksheet.write(row,col+7,mixed_count,cell_format)
                row += 1
                col = 0

            worksheet.autofit()
            worksheet.set_default_row(15)
        self.report_book.close()

def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument('-m','--maintainers', help='Path to maintainers.yml [Required]', required=True)
    parser.add_argument('-t','--testplan', help='Path to testplan [Required]', required=True)
    parser.add_argument('-c','--coverage', help='Path to components file [Required]', required=True)
    parser.add_argument('-o','--output', help='Report name [Required]', required=True)
    parser.add_argument('-f','--format', help='Output format (json, xlsx, all) [Required]', required=True)

    args = parser.parse_args()
    return args

if __name__ == '__main__':
    Json_report()
