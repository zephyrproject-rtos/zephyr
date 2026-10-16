#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0
#
# This script generate a C source files that containt the
# necessary code to run tf-psa-crypto test  in a Cube applcation.

import argparse
import re
import sys

def escape_c_string(value: str) -> str:
    """Escape characters for use inside a C-style string literal."""
    return value.replace("\\", "\\\\").replace('"', '\\"')


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="""
        Generate Ztest source file (zephyr_test.c) in the target build directory
        (2nd argument), based on the selected test_suite and test string IDs defined
        in the test suite list input file (1st argument).
        """
    )

    parser.add_argument(
        "input_file",
        help="Input test suites and string IDs list file",
    )
    parser.add_argument(
        "output_path",
        help="""
        Path of to tf-psa-crypto build directory where test files (.function
        and .datax files) have already been generated. zephyr_test.c is created
        in this directory.
        """
    )

    return parser.parse_args()


def write_array_data(file, line):
    bytes_per_line = 12

    line_display = line.rstrip("\r\n")
    file.write(f'/* Line:\n * {line_display}\n */\n')
    byte_line = line.encode("ascii")

    for offset in range(0, len(byte_line), bytes_per_line):
        chunk = byte_line[offset:offset + bytes_per_line]
        byte_values = ", ".join(f"0x{byte:02X}" for byte in chunk)
        file.write(f"    {byte_values}")
        file.write(",")

        file.write("\n")


def parse_input_test_suite_file(file_path):
    data = {}
    current_basename = None

    with open(file_path, "r", encoding="ascii") as file:
        for line_number, line in enumerate(file, start=1):
            line = line.rstrip("\r\n")

            if line == "" or line.startswith("#"):
                 continue

            # Start a new dictionary entry
            if line.startswith("basename="):
                basename = line.split("=", 1)[1].strip()

                if not basename:
                    raise ValueError(
                        f"Empty basename on line {line_number}"
                    )

                data[basename] = []
                current_basename = basename

            # Store lines belonging to the current basename
            elif current_basename is not None:
                data[current_basename].append(line)

            # Ignore lines appearing before the first basename
            else:
                print(
                    f"Warning: ignoring line {line_number}: {line!r}"
                )

    # Remove empty tetst suites
    filtered_dict = {k: v for k, v in data.items() if v}
    data.clear()
    data.update(filtered_dict)

    return data


def main():

    args = parse_arguments()

    test_suite_list = parse_input_test_suite_file(args.input_file)

    if test_suite_list is None:
        print(f'No entries found in {args.input_file}. Abort')
        return

    outfile_path = args.output_path + "/" + "zephyr_test.c"
    with open(outfile_path, "w", encoding="ascii", newline="\n") as output_file:

        output_file.write(
            "/*\n"
            " * # SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics\n"
            " * SPDX-License-Identifier: Apache-2.0\n"
            " */\n"
            "\n"
            "#include <zephyr/ztest.h>\n"
            "#include <stdio.h>\n"
            "#include <stdint.h>\n"
            "\n"
        )

        for index, (basename, entries) in enumerate(test_suite_list.items()):

            output_file.write(
                f'/*\n * ZTEST for {basename}\n */\n'
                "\n"
                f'const uint8_t {basename}_datax[] = '
                "{\n"
            )

            filtered_out_entries = []

            input_datax_file_path = args.output_path + "/" + basename + ".datax"
            with open(input_datax_file_path, "r", encoding="ascii", newline="") as datax_file:

                line_num = 0
                embedded_test_count = 0

                while True:
                    line_num = line_num + 1
                    datax_line = datax_file.readline()

                    if datax_line == "":
                        break

                    if datax_line.rstrip("\r\n") == "" or datax_line.startswith("#"):
                        continue

                    # TODO: scan input file then scan datax file.
                    found = False
                    for entry in entries:
                        entry = entry.rstrip("\r\n")

                        if entry != "<all>" and not datax_line.startswith(entry):
                            continue
                        found = True
                        write_array_data(output_file, datax_line)

                        line_num = line_num + 1
                        datax_line = datax_file.readline()
                        if datax_line.rstrip(" \t\r\n") != "":
                            write_array_data(output_file, datax_line)
                            line_num = line_num + 1
                            datax_line = datax_file.readline()
                            if datax_line.rstrip(" \t\r\n") != "":
                                write_array_data(output_file, datax_line)

                        # add an empty line
                        write_array_data(output_file, "\n")
                        break

                    if found:
                        embedded_test_count = embedded_test_count + 1
                    else:
                        filtered_out_entries.append(datax_line.rstrip("\r\n"))
                        line_num = line_num + 1
                        # Skip the next 1 or 2 lines, we shall found an empty line
                        # before the next test case description.
                        line = datax_file.readline().rstrip(" \t\r\n")
                        if line != "" and line != "":
                            line_num = line_num + 1
                            line = datax_file.readline().rstrip(" \t\r\n")
                        if line != "" and line != "":
                            line_num = line_num + 1
                            line = datax_file.readline().rstrip(" \t\r\n")
                        if line != "" and line != "":
                            print(
                                "Error: no empty line found after test entry:\n"
                                f"  file: {input_datax_file_path}\n"
                                f"  linne: {line_num}\n"
                                f"  test: {datax_line}"
                            )
                            sys.exit(1)

            # Test data MUST end of a nul byte
            output_file.write(
                "    0 /* Test suite termination */\n"
                "};\n"
                "\n"
            )

            if len(filtered_out_entries) == 0:
                output_file.write(
                    "/*\n"
                    f" * {embedded_test_count} embedded test(s).\n"
                    " * No filtered out test cases\n"
                    " */\n"
                    "\n"
                )
            else:
                output_file.write(
                    "/*\n"
                    f" * {embedded_test_count} embedded test(s).\n"
                    f" * {len(filtered_out_entries)} filtered out test cases:\n"
                )
                for entry in filtered_out_entries:
                    output_file.write(
                        f' * {entry}\n'
                    )
                output_file.write(
                    " */\n"
                    "\n"
                )

            # If the test suite is empty, skip adding ZTEST_USER() entry so that
            # no execute_<basename>() is called and let linker discard all ressources
            # (functions, global variables) that this function brings into the final binary.
            if embedded_test_count == 0:
                continue

            output_file.write(
                "/*\n"
                " * Entry function is exported to the Zephyr application\n"
                " * Return 0 upon success, any non-zero value upon errors.\n"
                " */\n"
                f'int execute_{basename}(const uint8_t *test_suite_array);\n'
                "\n"
                f'ZTEST_USER(psa_crypto_test_suite, {basename})\n'
                "{\n"
                f'    printf(\"Test suite {basename}: {embedded_test_count} test cases\\n\");\n'
                f'    zassert_equal(execute_{basename}(&{basename}_datax[0]), 0,\n'
                "                 \"Test failed\");\n"
                "}\n"
                "\n"
            )

        output_file.write(
            "static void *test_suite_init(void)\n"
            "{\n"
            "	return NULL;\n"
            "}\n"
            "\n"
            "ZTEST_SUITE(psa_crypto_test_suite, NULL, test_suite_init, NULL, NULL, NULL);\n"
        )

    print(f'All good, source code generated at: {outfile_path}')


if __name__ == "__main__":
    main()
