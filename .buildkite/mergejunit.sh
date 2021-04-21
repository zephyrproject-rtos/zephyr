#!/bin/bash
# Copyright (c) 2021 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
set -eE

buildkite-agent artifact download twister-*.xml .

xmls=""

for f in twister-*xml; do [ -s ${f} ] && xmls+="${f} "; done

if [ "${xmls}" ]; then
   junitparser merge ${xmls} junit.xml
   buildkite-agent artifact upload junit.xml
   junit2html junit.xml
   buildkite-agent artifact upload junit.xml.html
   buildkite-agent annotate --style "info" "Read the <a href=\"artifact://junit.xml.html\">JUnit test report</a>"
fi
