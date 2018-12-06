#!/usr/bin/env bash
# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

# Syntax run_parallel.sh [-h] [options]

set -u

START=$SECONDS

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
    exit 0;
  fi
fi

err=0
i=0

SEARCH_PATH="${SEARCH_PATH:-`pwd`}"

#All the testcases we want to run:
ALL_CASES=`find ${SEARCH_PATH} -name "*.sh" | \
           grep -Ev "(/_|run_parallel|compile.sh)"`
#we dont run ourselves

RESULTS_FILE="${RESULTS_FILE:-`pwd`/../RunResults.xml}"
TMP_RES_FILE=tmp.xml

ALL_CASES_A=( $ALL_CASES )
N_CASES=$((${#ALL_CASES_A[@]}))
touch ${RESULTS_FILE}
echo "Attempting to run ${N_CASES} cases (logging to \
 `realpath ${RESULTS_FILE}`)"

chmod +x $ALL_CASES

export CLEAN_XML="sed -E -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' \
                  -e 's/\"/&quot;/g'"

echo -n "" > $TMP_RES_FILE

if [ `command -v parallel` ]; then
  parallel '
  echo "<testcase name=\"{}\" time=\"0\">";
  {} $@ &> {#}.log ;
  if [ $? -ne 0 ]; then
    (>&2 echo -e "\e[91m{} FAILED\e[39m");
    (>&2 cat {#}.log);
    echo "<failure message=\"failed\" type=\"failure\">";
    cat {#}.log | eval $CLEAN_XML;
    echo "</failure>";
    rm {#}.log ;
    echo "</testcase>";
    exit 1;
  else
    (>&2 echo -e "{} PASSED");
    rm {#}.log ;
    echo "</testcase>";
  fi;
  ' ::: $ALL_CASES >> $TMP_RES_FILE ; err=$?
else #fallback in case parallel is not installed
  for CASE in $ALL_CASES; do
    echo "<testcase name=\"$CASE\" time=\"0\">" >> $TMP_RES_FILE
    $CASE $@ &> $i.log
    if [ $? -ne 0 ]; then
      echo -e "\e[91m$CASE FAILED\e[39m"
      cat $i.log
      echo "<failure message=\"failed\" type=\"failure\">" >> $TMP_RES_FILE
      cat $i.log | eval $CLEAN_XML >> $TMP_RES_FILE
      echo "</failure>" >> $TMP_RES_FILE
      let "err++"
    else
      echo -e "$CASE PASSED"
    fi;
    echo "</testcase>" >> $TMP_RES_FILE
    rm $i.log
    let i=i+1
  done
fi
echo -e "</testsuite>\n</testsuites>\n" >> $TMP_RES_FILE
dur=$(($SECONDS - $START))
echo -e "<testsuites>\n<testsuite errors=\"0\" failures=\"$err\"\
 name=\"bsim_bt tests\" skip=\"0\" tests=\"$N_CASES\" time=\"$dur\">" \
 | cat - $TMP_RES_FILE > $RESULTS_FILE
rm $TMP_RES_FILE

exit $err
