#!/bin/bash -x

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
ZEPHYR_BRANCH=$2
cd /nfs/site/home/sys_gsrd
source /nfs/site/home/sys_gsrd/.bashrc
lsb_release -a
export PATH=/build/zephyr-sdk-0.15.1:/build/zephyrproject/.venv/bin:$PATH
export CCACHE_DIR=/build/ccache
unset CROSS_COMPILE
PROJECT_NAME="building-zephyr-project"
BOARD="intel_socfpga_agilex5_socdk"
EXAMPLE="samples/boards/intel_socfpga/cli/"
ZEPHYR_FOLDER="zephyr"

# West init and West Update
cd $WORK_DIR
west init -m $ZEPHYR_REPOSITORY --mr $ZEPHYR_BRANCH zephyrproject
cd zephyrproject
west update

# Starting zephyr build
rm -rf $ZEPHYR_FOLDER
cp -rf $WORKSPACE $WORK_DIR/zephyrproject/$ZEPHYR_FOLDER
cd $WORK_DIR/zephyrproject/$ZEPHYR_FOLDER
west build -b $BOARD $EXAMPLE -d zephyr_build
(($? != 0)) && { printf '%s\n' "Command exited with non-zero"; exit -1; }

# Pass The $ZEPHYR_OUTPUT_DIR to the next Job
mkdir -p $PWD/zephyr/cli
cp zephyr_build/zephyr/zephyr.bin zephyr_build/zephyr/zephyr.elf zephyr/cli

tar cf zephyr.tar zephyr

#add ssh and pass this tar files
rmt_dir="/nfs/site/disks/swbld_regofficiala_pg12/users/sys_gsrd/zephyr-ci/$BRANCH_NAME/$JENKINS_JOB"
ssh ppglswbld001.png.intel.com -- mkdir -p $rmt_dir
scp -r zephyr.tar  ppglswbld001.png.intel.com:$rmt_dir
ssh ppglswbld001.png.intel.com -- "cd $rmt_dir && tar xf zephyr.tar && rm -rf zephyr.tar"
