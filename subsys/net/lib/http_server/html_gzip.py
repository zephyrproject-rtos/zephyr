#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2023 Emna Rekik

import subprocess
import os

# Define the paths
input_file = "src/index.html"
output_file = "src/index.html.gz"
header_file = "src/headers/index_html_gz.h"

# Compress the HTML file
gzip_cmd = ["gzip", "-9", "<", input_file, ">", output_file]
subprocess.run(" ".join(gzip_cmd), shell=True, check=True)

# Generate the header file
xxd_cmd = ["xxd", "-i", output_file, header_file]
subprocess.run(xxd_cmd, check=True)

# Delete the compressed file
os.remove(output_file)
