#!/bin/bash

# Get the Jenkins Job ID as Unique Build path before passing to the regtest
# Fail Safe: Else Job ID not exist, create a temporary folder
JENKINS_JOB=$1
WORK_DIR=/build/github_ci_zephyr/
if [ "x$JENKINS_JOB" == "x" ]
then
mkdir -p $WORK_DIR
JENKINS_JOB=`basename $(mktemp -d --tmpdir=/build/github_ci_zephyr)`
fi
WORK_DIR=/build/github_ci_zephyr/$JENKINS_JOB
mkdir -p $WORK_DIR

# Do Environment setup on container machine
ZEPHYR_REPOSITORY=https://github.com/intel-innersource/os.rtos.zephyr.socfpga.zephyr-socfpga-dev
ZEPHYR_BRANCH=main
cd /nfs/site/home/sys_gsrd
source /nfs/site/home/sys_gsrd/.bashrc
lsb_release -a
export PATH=/build/zephyr-sdk-0.13.1:/build/zephyrproject/.venv/bin:$PATH
export CCACHE_DIR=/build/ccache
unset CROSS_COMPILE
PROJECT_NAME="building-zephyr-project"
BOARD="intel_socfpga_agilex_socdk"
EXAMPLE="samples/hello_world"
ZEPHYR_FOLDER="zephyr"
ZEPHYR_OUTPUT_DIR=$WORK_DIR/zephyrproject/$ZEPHYR_FOLDER/build/zephyr

# West init and West Update
cd $WORK_DIR
west init -m $ZEPHYR_REPOSITORY --mr $ZEPHYR_BRANCH zephyrproject
cd zephyrproject
west update

# Starting Build
rm -rf $ZEPHYR_FOLDER
cp -rf $WORKSPACE $WORK_DIR/zephyrproject/$ZEPHYR_FOLDER
cd $WORK_DIR/zephyrproject/$ZEPHYR_FOLDER
west build -b $BOARD $EXAMPLE

# Pass The $ZEPHYR_OUTPUT_DIR to the next Job
