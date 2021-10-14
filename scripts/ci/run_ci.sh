#!/bin/bash
# Copyright (c) 2017 Linaro Limited
# Copyright (c) 2018 Intel Corporation
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
# The script can be run locally using for example:
# ./scripts/ci/run_ci.sh -b main -r origin  -l -R <commit range>

set -xe

twister_options=" --inline-logs -M -N -v --integration"

west_commands_results_file="./pytest_out/west_commands.xml"

matrix_builds=1
matrix=1

function handle_coverage() {
	# Upload to codecov.io only on merged builds or if CODECOV_IO variable
	# is set.
	if [ -n "${CODECOV_IO}" -o -z "${pull_request_nr}" ]; then
		# Capture data
		echo "Running lcov --capture ..."
		lcov --capture \
			--directory twister-out/native_posix/ \
			--directory twister-out/nrf52_bsim/ \
			--directory twister-out/unit_testing/ \
			--output-file lcov.pre.info -q --rc lcov_branch_coverage=1

		# Remove noise
		echo "Exclude data from coverage report..."
		lcov -q \
			--remove lcov.pre.info mylib.c \
			--remove lcov.pre.info tests/\* \
			--remove lcov.pre.info samples/\* \
			--remove lcov.pre.info ext/\* \
			--remove lcov.pre.info *generated* \
			-o lcov.info --rc lcov_branch_coverage=1

		# Cleanup
		rm lcov.pre.info
		rm -rf twister-out out-2nd-pass

		# Upload to codecov.io
		echo "Upload coverage reports to codecov.io"
		bash <(curl -s https://codecov.io/bash) -f "lcov.info" -X coveragepy -X fixes
		rm -f lcov.info
	fi

	rm -rf twister-out out-2nd-pass

}

function handle_compiler_cache() {
	# Give more details in case we fail because of compiler cache
	if [ -f "$HOME/.cache/zephyr/ToolchainCapabilityDatabase.cmake" ]; then
		echo "Dumping the capability database in case we are affected by #9992"
		cat $HOME/.cache/zephyr/ToolchainCapabilityDatabase.cmake
	fi
}

function on_complete() {
	source zephyr-env.sh
	if [ "$1" == "failure" ]; then
		handle_compiler_cache
	fi

	rm -rf ccache $HOME/.cache/zephyr

	if [ "$matrix" = "1" ]; then
		echo "Skip handling coverage data..."
		#handle_coverage
	else
		rm -rf twister-out out-2nd-pass
	fi
}

function build_test_file() {
	# cleanup
	rm -f test_file_boards.txt test_file_tests.txt test_file_archs.txt test_file_full.txt
	touch test_file_boards.txt test_file_tests.txt test_file_archs.txt test_file_full.txt

	twister_exclude_tag_opt=""

	# In a pull-request see if we have changed any tests or board definitions
	if [ -n "${pull_request_nr}" -o -n "${local_run}"  ]; then
		./scripts/zephyr_module.py --twister-out module_tests.args
		./scripts/ci/get_twister_opt.py --commits ${commit_range}

		if [ -s modified_tags.args ]; then
			twister_exclude_tag_opt="+modified_tags.args"
		fi

		if [ -s modified_boards.args ]; then
			${twister} ${twister_options} ${twister_exclude_tag_opt} \
				+modified_boards.args \
				--save-tests test_file_boards.txt || exit 1
		fi
		if [ -s modified_tests.args ]; then
			${twister} ${twister_options} ${twister_exclude_tag_opt} \
				+modified_tests.args \
				--save-tests test_file_tests.txt || exit 1
		fi
		if [ -s modified_archs.args ]; then
			${twister} ${twister_options} ${twister_exclude_tag_opt} \
				+modified_archs.args \
				--save-tests test_file_archs.txt || exit 1
		fi
		rm -f modified_tests.args modified_boards.args modified_archs.args
	fi

	if [ "$SC" == "full" ]; then
		# Save list of tests to be run
		${twister} ${twister_options} ${twister_exclude_tag_opt} \
			--save-tests test_file_full.txt || exit 1
	fi

	rm -f modified_tags.args

	# Remove headers from all files.  We insert it into test_file.txt explicitly
	# so we treat all test_file*.txt files the same.
	tail -n +2 test_file_full.txt > test_file_full_in.txt
	tail -n +2 test_file_archs.txt > test_file_archs_in.txt
	tail -n +2 test_file_tests.txt > test_file_tests_in.txt
	tail -n +2 test_file_boards.txt > test_file_boards_in.txt

	echo "test,arch,platform,status,extra_args,handler,handler_time,ram_size,rom_size" \
		> test_file.txt
	cat test_file_full_in.txt test_file_archs_in.txt test_file_tests_in.txt \
		test_file_boards_in.txt >> test_file.txt
}

function west_setup() {
	# West handling
	git_dir=$(basename $PWD)
	pushd ..
	if [ ! -d .west ]; then
		west init -l ${git_dir}
		west update 1> west.update.log || west update 1> west.update-2.log
		west forall -c 'git reset --hard HEAD'
	fi
	popd
}


while getopts ":p:m:b:r:M:cfslR:" opt; do
	case $opt in
		c)
			echo "Execute CI" >&2
			main_ci=1
			;;
		l)
			echo "Executing script locally" >&2
			local_run=1
			main_ci=1
			;;
		s)
			echo "Success" >&2
			success=1
			;;
		f)
			echo "Failure" >&2
			failure=1
			;;
		p)
			echo "Testing a Pull Request: $OPTARG." >&2
			pull_request_nr=$OPTARG
			;;
		m)
			echo "Running on Matrix $OPTARG" >&2
			matrix=$OPTARG
			;;
		M)
			echo "Running a matrix of $OPTARG slaves" >&2
			matrix_builds=$OPTARG
			;;
		b)
			echo "Base Branch: $OPTARG" >&2
			branch=$OPTARG
			;;
		r)
			echo "Remote: $OPTARG" >&2
			remote=$OPTARG
			;;
		R)
			echo "Range: $OPTARG" >&2
			range=$OPTARG
			;;
		\?)
			echo "Invalid option: -$OPTARG" >&2
			;;
	esac
done

if [ -n "$main_ci" ]; then

	west_setup

	if [ -z "$branch" ]; then
		echo "No base branch given"
		exit 1
	else
		commit_range=$remote/${branch}..HEAD
		echo "Commit range:" ${commit_range}
	fi
	if [ -n "$range" ]; then
		commit_range=$range
	fi
	source zephyr-env.sh
	twister="${ZEPHYR_BASE}/scripts/twister"

	# Possibly the only record of what exact version is being tested:
	short_git_log='git log -n 5 --oneline --decorate --abbrev=12 '

	# check what files have changed for PRs or local runs.  If we are
	# building for a commit than we always do a "Full Run".
	if [ -n "${pull_request_nr}" -o -n "${local_run}"  ]; then
		SC=`./scripts/ci/what_changed.py --commits ${commit_range}`
	else
		echo "Full Run"
		SC="full"
	fi

	if [ -n "$pull_request_nr" ]; then
		$short_git_log $remote/${branch}
		# Now let's pray this script is being run from a
		# different location
# https://stackoverflow.com/questions/3398258/edit-shell-script-while-its-running
		git rebase $remote/${branch}
	fi
	$short_git_log

	build_test_file

	echo "+++ run twister"

	# Run a subset of tests based on matrix size
	${twister} ${twister_options} --load-tests test_file.txt \
		--subset ${matrix}/${matrix_builds} --retry-failed 3

	# Run module tests on matrix #1
	if [ "$matrix" = "1" -a  "$SC" == "full" ]; then
		if [ -s module_tests.args ]; then
			${twister} ${twister_options} \
				+module_tests.args --outdir module_tests
		fi
	fi

	# cleanup
	rm -f test_file*

elif [ -n "$failure" ]; then
	on_complete failure
elif [ -n "$success" ]; then
	on_complete
else
	echo "Nothing to do"
fi

