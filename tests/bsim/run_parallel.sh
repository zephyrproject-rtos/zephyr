#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

start=$SECONDS

function display_help(){
  echo "run_parallel.sh [-help] [options]"
  echo "  Execute all cases which do not start with an _ (underscore)"
  echo "  [options] will be passed directly to the scripts"
  echo "  The results will be saved to \${RESULTS_FILE}, by default"
  echo "  ../RunResults.xml"
  echo "  Testcases are searched for in \${SEARCH_PATH},"
  echo "  which by default is the folder the script is run from"
  echo "  You can instead also provide a space separated test list with \${TESTS_LIST}, "
  echo "  or an input file including a list of tests and/or tests search paths"
  echo "  \${TESTS_FILE} (w one line per test/path, you can comment lines with #)"
  echo ""
  echo "  Examples (run from \${ZEPHYR_BASE}):"
  echo " * Run all tests found under one folder:"
  echo "   SEARCH_PATH=tests/bsim/bluetooth/ll/conn/ tests/bsim/run_parallel.sh"
  echo " * Run all tests found under two separate folders, matching a pattern in the first:"
  echo "   SEARCH_PATH=\"tests/bsim/bluetooth/ll/conn/tests_scripts/*encr*  tests/bsim/net\"\
 tests/bsim/run_parallel.sh"
  echo " * Provide a tests list explicitly from an environment variable"
  echo "   TESTS_LIST=\
\"tests/bsim/bluetooth/ll/conn/tests_scripts/basic_conn_encrypted_split_privacy.sh\
 tests/bsim/bluetooth/ll/conn/tests_scripts/basic_conn_split_low_lat.sh\
 tests/bsim/bluetooth/ll/conn/tests_scripts/basic_conn_split.sh\" tests/bsim/run_parallel.sh"
  echo " * Provide a tests list in a file:"
  echo "   TESTS_FILE=my_tests.txt tests/bsim/run_parallel.sh"
}

# Parse command line
if [ $# -ge 1 ]; then
  if grep -Eiq "(\?|-\?|-h|help|-help|--help)" <<< $1 ; then
    display_help
    exit 0
  fi
fi

err=0
i=0

if [ -n "${TESTS_FILE}" ]; then
	#remove comments and empty lines from file
	search_pattern=$(sed 's/#.*$//;/^$/d' "${TESTS_FILE}") || exit 1
	all_cases=`find ${search_pattern} -name "*.sh" | \
	         grep -Ev "(/_|run_parallel|compile|generate_coverage_report.sh)"`
elif [ -n "${TESTS_LIST}" ]; then
	all_cases=${TESTS_LIST}
else
	SEARCH_PATH="${SEARCH_PATH:-.}"
	all_cases=`find ${SEARCH_PATH} -name "*.sh" | \
	         grep -Ev "(/_|run_parallel|compile|generate_coverage_report.sh)"`
	#we dont run ourselves
fi

set -u

RESULTS_FILE="${RESULTS_FILE:-`pwd`/../RunResults.xml}"
tmp_res_file=tmp.xml

all_cases_a=( $all_cases )
n_cases=$((${#all_cases_a[@]}))

mkdir -p $(dirname ${RESULTS_FILE})
touch ${RESULTS_FILE}
echo "Attempting to run ${n_cases} cases (logging to \
 `realpath ${RESULTS_FILE}`)"

export CLEAN_XML="sed -E -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' \
                  -e 's/\"/&quot;/g'"

echo -n "" > $tmp_res_file

if [ `command -v parallel` ]; then
  parallel '
  echo "<testcase name=\"{}\" time=\"0\">"
  start=$(date +%s%N)
  {} $@ &> {#}.log ; result=$?
  dur=$(($(date +%s%N) - $start))
  dur_s=$(awk -vdur=$dur "BEGIN { printf(\"%0.3f\", dur/1000000000)}")
  if [ $result -ne 0 ]; then
    (>&2 echo -e "\e[91m{} FAILED\e[39m ($dur_s s)")
    (>&2 cat {#}.log)
    echo "<failure message=\"failed\" type=\"failure\">"
    cat {#}.log | eval $CLEAN_XML
    echo "</failure>"
    rm {#}.log
    echo "</testcase>"
    exit 1
  else
    (>&2 echo -e "{} PASSED ($dur_s s)")
    rm {#}.log
    echo "</testcase>"
  fi
  ' ::: $all_cases >> $tmp_res_file ; err=$?
else #fallback in case parallel is not installed
  for case in $all_cases; do
    echo "<testcase name=\"$case\" time=\"0\">" >> $tmp_res_file
    $case $@ &> $i.log
    if [ $? -ne 0 ]; then
      echo -e "\e[91m$case FAILED\e[39m"
      cat $i.log
      echo "<failure message=\"failed\" type=\"failure\">" >> $tmp_res_file
      cat $i.log | eval $CLEAN_XML >> $tmp_res_file
      echo "</failure>" >> $tmp_res_file
      let "err++"
    else
      echo -e "$case PASSED"
    fi
    echo "</testcase>" >> $tmp_res_file
    rm $i.log
    let i=i+1
  done
fi
echo -e "</testsuite>\n</testsuites>\n" >> $tmp_res_file
dur=$(($SECONDS - $start))
echo -e "<testsuites>\n<testsuite errors=\"0\" failures=\"$err\"\
 name=\"bsim tests\" skip=\"0\" tests=\"$n_cases\" time=\"$dur\">" \
 | cat - $tmp_res_file > $RESULTS_FILE
rm $tmp_res_file

exit $err
