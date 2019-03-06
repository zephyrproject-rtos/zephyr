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
# ./scripts/ci/run_ci.sh -b master -r origin  -l -R <commit range>

set -xe

SANITYCHECK_OPTIONS=" --inline-logs --enable-coverage -N"
SANITYCHECK_OPTIONS_RETRY="${SANITYCHECK_OPTIONS} --only-failed --outdir=out-2nd-pass"
SANITYCHECK_OPTIONS_RETRY_2="${SANITYCHECK_OPTIONS} --only-failed --outdir=out-3nd-pass"
export BSIM_OUT_PATH="${BSIM_OUT_PATH:-/opt/bsim/}"
export BSIM_COMPONENTS_PATH="${BSIM_OUT_PATH}/components/"
BSIM_BT_TEST_RESULTS_FILE="./bsim_bt_out/bsim_results.xml"
WEST_COMMANDS_RESULTS_FILE="./pytest_out/west_commands.xml"

MATRIX_BUILDS=1
MATRIX=1

while getopts ":p:m:b:r:M:cfslR:" opt; do
	case $opt in
		c)
			echo "Execute CI" >&2
			MAIN_CI=1
			;;
		l)
			echo "Executing script locally" >&2
			LOCAL_RUN=1
			MAIN_CI=1
			;;
		s)
			echo "Success" >&2
			SUCCESS=1
			;;
		f)
			echo "Failure" >&2
			FAILURE=1
			;;
		p)
			echo "Testing a Pull Request: $OPTARG." >&2
			PULL_REQUEST_NR=$OPTARG
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
		r)
			echo "Remote: $OPTARG" >&2
			REMOTE=$OPTARG
			;;
		R)
			echo "Range: $OPTARG" >&2
			RANGE=$OPTARG
			;;
		\?)
			echo "Invalid option: -$OPTARG" >&2
			;;
	esac
done

DOC_MATRIX=${MATRIX_BUILDS}

if [ -n "$MAIN_CI" ]; then

	# West handling
        pushd ..
	if [ ! -d .west ]; then
		west init -l zephyr
		west update
	fi
        popd

	if [ -z "$BRANCH" ]; then
		echo "No base branch given"
		exit 1
	else
		COMMIT_RANGE=$REMOTE/${BRANCH}..HEAD
		echo "Commit range:" ${COMMIT_RANGE}
	fi
	if [ -n "$RANGE" ]; then
		COMMIT_RANGE=$RANGE
	fi
	source zephyr-env.sh
	SANITYCHECK="${ZEPHYR_BASE}/scripts/sanitycheck"

	# Possibly the only record of what exact version is being tested:
	short_git_log='git log -n 5 --oneline --decorate --abbrev=12 '

	if [ -n "$PULL_REQUEST_NR" ]; then
		$short_git_log $REMOTE/${BRANCH}
		# Now let's pray this script is being run from a
		# different location
# https://stackoverflow.com/questions/3398258/edit-shell-script-while-its-running
		git rebase $REMOTE/${BRANCH};
	fi
	$short_git_log
fi

function handle_coverage() {
	# this is for shippable coverage reports
	echo "Calling gcovr"
	gcovr -r ${ZEPHYR_BASE} -x > shippable/codecoverage/coverage.xml;

	# Capture data
	echo "Running lcov --capture ..."
	lcov --capture \
		--directory sanity-out/native_posix/ \
		--directory sanity-out/nrf52_bsim/ \
		--directory sanity-out/unit_testing/ \
		--directory bsim_bt_out/ \
		--output-file lcov.pre.info -q --rc lcov_branch_coverage=1;

	# Remove noise
	echo "Exclude data from coverage report..."
	lcov -q \
		--remove lcov.pre.info mylib.c \
		--remove lcov.pre.info tests/\* \
		--remove lcov.pre.info samples/\* \
		--remove lcov.pre.info ext/\* \
		--remove lcov.pre.info *generated* \
		-o lcov.info --rc lcov_branch_coverage=1;

	# Cleanup
	rm lcov.pre.info;
	rm -rf sanity-out out-2nd-pass;

	# Upload to codecov.io
	echo "Upload coverage reports to codecov.io"
	bash <(curl -s https://codecov.io/bash) -f "lcov.info" -X coveragepy -X fixes;
	rm -f lcov.info;

}

function handle_compiler_cache() {
	# Give more details in case we fail because of compiler cache
	if [ -f "$HOME/.cache/zephyr/ToolchainCapabilityDatabase.cmake" ]; then
		echo "Dumping the capability database in case we are affected by #9992"
		cat $HOME/.cache/zephyr/ToolchainCapabilityDatabase.cmake
	fi;
}

function on_complete() {
	source zephyr-env.sh
	if [ "$1" == "failure" ]; then
		handle_compiler_cache
	fi

	rm -rf ccache $HOME/.cache/zephyr
	mkdir -p shippable/testresults
	mkdir -p shippable/codecoverage

	if [ -e compliance.xml ]; then
		echo "Copy compliance.xml"
		cp compliance.xml shippable/testresults/;
	fi;

	if [ -e ./scripts/sanity_chk/last_sanity.xml ]; then
		echo "Copy ./scripts/sanity_chk/last_sanity.xml"
		cp ./scripts/sanity_chk/last_sanity.xml shippable/testresults/;
	fi;

	if [ -e ${BSIM_BT_TEST_RESULTS_FILE} ]; then
		echo "Copy ${BSIM_BT_TEST_RESULTS_FILE}"
		cp ${BSIM_BT_TEST_RESULTS_FILE} shippable/testresults/;
	fi;

	if [ -e ${WEST_COMMANDS_RESULTS_FILE} ]; then
		echo "Copy ${WEST_COMMANDS_RESULTS_FILE}"
		cp ${WEST_COMMANDS_RESULTS_FILE} shippable/testresults;
	fi;

	if [ "$MATRIX" = "1" ]; then
		echo "Handle coverage data..."
		handle_coverage
	else
		rm -rf sanity-out out-2nd-pass;
	fi;
}


function build_btsim() {
	NRF_HW_MODELS_VERSION=`cat boards/posix/nrf52_bsim/hw_models_version`
	pushd . ;
	cd ${BSIM_COMPONENTS_PATH} ;
	if [ -d ext_NRF52_hw_models ]; then
		cd ext_NRF52_hw_models
		git describe --tags --abbrev=0 ${NRF52_HW_MODELS_TAG}\
		> /dev/null ||
		(
			echo "`pwd` seems to contain the nRF52 HW\
 models but they are out of date"
			exit 1;
		)
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

if [ -n "$MAIN_CI" ]; then

	if [ -n "${BSIM_OUT_PATH}" -a -d "${BSIM_OUT_PATH}" ]; then
		echo "Build BT simulator tests"
		# Build BT Simulator
		build_btsim

		# Run BLE tests in simulator on the 1st CI instance:
		if [ "$MATRIX" = "1" ]; then
			run_bsim_bt_tests
		fi
	else
		echo "Skipping BT simulator tests"
	fi

	if [ "$MATRIX" = "1" ]; then
		# Run pytest-based testing for Python in matrix
		# builder 1.  For now, this is just done for the west
		# extension commands, but additional directories which
		# run pytest could go here too.
		PYTEST=$(type -p pytest-3 || echo "pytest")
		mkdir -p $(dirname ${WEST_COMMANDS_RESULTS_FILE})
		WEST_SRC=$(west list --format='{abspath}' west)/src
		PYTHONPATH=./scripts/west_commands:$WEST_SRC "${PYTEST}" \
			  --junitxml=${WEST_COMMANDS_RESULTS_FILE} \
			  ./scripts/west_commands/tests
	else
		echo "Skipping west command tests"
	fi

	# In a pull-request see if we have changed any tests or board definitions
	if [ -n "${PULL_REQUEST_NR}" -o -n "${LOCAL_RUN}"  ]; then
		get_tests_to_run
	fi

	# Save list of tests to be run
	${SANITYCHECK} ${SANITYCHECK_OPTIONS} --save-tests test_file.txt || exit 1

	# Run a subset of tests based on matrix size
	${SANITYCHECK} ${SANITYCHECK_OPTIONS} --load-tests test_file.txt \
		--subset ${MATRIX}/${MATRIX_BUILDS} || \
		( sleep 10; ${SANITYCHECK} ${SANITYCHECK_OPTIONS_RETRY} ) || \
		( sleep 10; ${SANITYCHECK} ${SANITYCHECK_OPTIONS_RETRY_2}; )
		# sleep 10 to let the host settle down

	# cleanup
	rm -f test_file.txt

elif [ -n "$FAILURE" ]; then
	on_complete failure
elif [ -n "$SUCCESS" ]; then
	on_complete
else
	echo "Nothing to do"
fi

