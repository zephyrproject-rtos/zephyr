#!/bin/bash -x

# Get the Jenkins Job ID as Unique Build path before passing to the regtest
# Fail Safe: Else Job ID not exist, create a temporary folder
JENKINS_JOB=$1;
TARGET_BRANCH=$2;
SOURCE_BRANCH=$3;
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
ZEPHYR_BRANCH=$SOURCE_BRANCH
cd /nfs/site/home/sys_gsrd
source /nfs/site/home/sys_gsrd/.bashrc
lsb_release -a

export PATH=/build/zephyr-sdk-0.16.1:/build/zephyrproject/.venv/bin:$PATH
export CCACHE_DIR=/build/ccache

unset CROSS_COMPILE

# West init and West Update
cd $WORK_DIR
west init -m $ZEPHYR_REPOSITORY --mr ${ZEPHYR_BRANCH} zephyrproject
cd zephyrproject/zephyr
west update -n

# show the last commit
git log -1

#check compliance test
python scripts/ci/check_compliance.py -c "origin/${TARGET_BRANCH}..HEAD"
