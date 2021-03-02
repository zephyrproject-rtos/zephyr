#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Syntax run_parallel.sh [-h] [options]

set -u

start=$SECONDS

function display_help(){
  echo "run_parallel.sh [-help] [options]"
  echo "  Execute all cases which do not start with an _ (underscore)"
  echo "  [options] will be passed directly to the scripts"
  echo "  The results will be saved to \${RESULTS_FILE}, by deault"
  echo "  ../RunResults.xml"
  echo "  Testcases are searched for in \${SEARCH_PATH}, by default this folder"
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

SEARCH_PATH="${SEARCH_PATH:-.}"

#All the testcases we want to run:
all_cases=`find ${SEARCH_PATH} -name "*.sh" | \
           grep -Ev "(/_|run_parallel|compile.sh)"`
#we dont run ourselves

RESULTS_FILE="${RESULTS_FILE:-`pwd`/../RunResults.xml}"
tmp_res_file=tmp.xml

all_cases_a=( $all_cases )
n_cases=$((${#all_cases_a[@]}))
touch ${RESULTS_FILE}
echo "Attempting to run ${n_cases} cases (logging to \
 `realpath ${RESULTS_FILE}`)"

chmod +x $all_cases

export CLEAN_XML="sed -E -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' \
                  -e 's/\"/&quot;/g'"

echo -n "" > $tmp_res_file

if [ `command -v parallel` ]; then
  parallel '
  echo "<testcase name=\"{}\" time=\"0\">"
  {} $@ &> {#}.log
  if [ $? -ne 0 ]; then
    (>&2 echo -e "\e[91m{} FAILED\e[39m")
    (>&2 cat {#}.log)
    echo "<failure message=\"failed\" type=\"failure\">"
    cat {#}.log | eval $CLEAN_XML
    echo "</failure>"
    rm {#}.log
    echo "</testcase>"
    exit 1
  else
    (>&2 echo -e "{} PASSED")
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
 name=\"bsim_bt tests\" skip=\"0\" tests=\"$n_cases\" time=\"$dur\">" \
 | cat - $tmp_res_file > $RESULTS_FILE
rm $tmp_res_file

exit $err
