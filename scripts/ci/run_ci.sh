#!/bin/bash
# Copyright (c) 2017 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#
# Author: Anas Nashif
#
# This script is run in CI and support both pull request and commit modes. So
# it can be used also on commits to the tree or on tags.
# The following options are supports:
# -p  start the script for pull requests
# -m  matrix node number, for example 3 on a 5 node matrix
# -M  total number of nodes in the matrix
# -b  base branch
# -r  the remote to rebase on
#
# The script can be run locally using for exmaple:
# ./scripts/ci/run_ci.sh -b master -r upstream  -p
source zephyr-env.sh

SANITYCHECK_OPTIONS=" --inline-logs --enable-coverage -N"
SANITYCHECK_OPTIONS_RETRY="${SANITYCHECK_OPTIONS} --only-failed --outdir=out-2nd-pass"
SANITYCHECK_OPTIONS_RETRY_2="${SANITYCHECK_OPTIONS} --only-failed --outdir=out-3nd-pass"
SANITYCHECK="${ZEPHYR_BASE}/scripts/sanitycheck"
MATRIX_BUILDS=1
MATRIX=1

while getopts ":pm:b:B:r:M:" opt; do
  case $opt in
    p)
      echo "Testing a Pull Request." >&2
      PULL_REQUEST=1
      ;;
    m)
      echo "Running on Matrix $OPTARG" >&2
      MATRIX=$OPTARG
      ;;
    M)
      echo "Running a matrix of $OPTARG slaves" >&2
      MATRIX_BUILDS=$OPTARG
      ;;
    b)
      echo "Base Branch: $OPTARG" >&2
      BRANCH=$OPTARG
      ;;
    B)
      echo "bsim BT tests xml results file: $OPTARG" >&2
      BSIM_BT_TEST_RESULTS_FILE=$OPTARG
      ;;
    r)
      echo "Remote: $OPTARG" >&2
      REMOTE=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

DOC_MATRIX=${MATRIX_BUILDS}

if [ -z "$BRANCH" ]; then
	echo "No base branch given"
	exit
fi

if [ -n "$PULL_REQUEST" ]; then
	git rebase $REMOTE/${BRANCH};
fi

if [ -n "$BRANCH" ]; then
	COMMIT_RANGE=$REMOTE/${BRANCH}..HEAD
	echo "Commit range:" ${COMMIT_RANGE}
else
	exit
fi

function build_btsim() {
	NRF_HW_MODELS_VERSION=`cat boards/posix/nrf52_bsim/hw_models_version`
	pushd . ;
	cd ${BSIM_COMPONENTS_PATH} ;
	if [ -d ext_NRF52_hw_models ]; then
		cd ext_NRF52_hw_models
		git describe --tags --abbrev=0 ${NRF52_HW_MODELS_TAG}\
		> /dev/null
		if [ "$?" != "0" ]; then
			echo "`pwd` seems to contain the nRF52 HW\
 models but they are out of date"
			exit 1;
		fi
	else
		git clone -b ${NRF_HW_MODELS_VERSION} \
		https://github.com/BabbleSim/ext_NRF52_hw_models.git
	fi
	cd ${BSIM_OUT_PATH}
	make everything -j 8 -s
	popd ;
}

function run_bsim_bt_tests() {
	WORK_DIR=${ZEPHYR_BASE}/bsim_bt_out tests/bluetooth/bsim_bt/compile.sh
	RESULTS_FILE=${ZEPHYR_BASE}/${BSIM_BT_TEST_RESULTS_FILE} \
	SEARCH_PATH=tests/bluetooth/bsim_bt/bsim_test_app/tests_scripts \
	tests/bluetooth/bsim_bt/run_parallel.sh
}

function build_docs() {
    echo "- Building Documentation";
    make htmldocs
    if [ "$?" != "0" ]; then
      echo "Documentation build failed";
      exit 1;
    fi
    if [ -s doc/_build/doc.warnings ]; then
      echo " => New documentation warnings/errors";
      cp doc/_build/doc.warnings doc.warnings
    fi
    echo "- Verify commit message, coding style, doc build";
}

function get_tests_to_run() {
    ./scripts/ci/get_modified_tests.py --commits ${COMMIT_RANGE} > modified_tests.args;
    ./scripts/ci/get_modified_boards.py --commits ${COMMIT_RANGE} > modified_boards.args;

    if [ -s modified_boards.args ]; then
      ${SANITYCHECK} ${SANITYCHECK_OPTIONS} +modified_boards.args --save-tests test_file.txt || exit 1;
    fi
    if [ -s modified_tests.args ]; then
      ${SANITYCHECK} ${SANITYCHECK_OPTIONS} +modified_tests.args --save-tests test_file.txt || exit 1;
    fi
    rm -f modified_tests.args modified_boards.args;
}

if [ ! -z "${BSIM_OUT_PATH}" ]; then
	# Build BT Simulator
	build_btsim

	# Run BLE tests in simulator on the 1st CI instance:
	if [ "$MATRIX" = "1" ]; then
		run_bsim_bt_tests
	fi
else
	echo "Skipping BT simulator tests"
fi

# In a pull-request see if we have changed any tests or board definitions
if [ -n "${PULL_REQUEST}" ]; then
	get_tests_to_run
fi

# Save list of tests to be run
${SANITYCHECK} ${SANITYCHECK_OPTIONS} --save-tests test_file.txt || exit 1

# Run a subset of tests based on matrix size
${SANITYCHECK} ${SANITYCHECK_OPTIONS} --load-tests test_file.txt --subset ${MATRIX}/${MATRIX_BUILDS}
if [ "$?" != 0 ]; then
	# let the host settle down
	sleep 10
	${SANITYCHECK} ${SANITYCHECK_OPTIONS_RETRY} || \
		( sleep 10; ${SANITYCHECK} ${SANITYCHECK_OPTIONS_RETRY}; )
fi

# cleanup
rm test_file.txt
