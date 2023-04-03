#!/bin/bash -x

# Get the Jenkins Job ID as Unique Build path before passing to the regtest
# Fail Safe: Else Job ID not exist, create a temporary folder
JENKINS_JOB=$1;
ZEPHYR_BRANCH=$2;
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

cd /nfs/site/home/sys_gsrd
source /nfs/site/home/sys_gsrd/.bashrc
lsb_release -a
export PATH=/build/zephyr-sdk-0.15.1:/build/zephyrproject/.venv/bin:$PATH
export CCACHE_DIR=/build/ccache
unset CROSS_COMPILE
PROJECT_NAME="building-zephyr-project"
ZEPHYR_FOLDER="zephyr"

# West init and West Update
cd $WORK_DIR
west init -m $ZEPHYR_REPOSITORY --mr $ZEPHYR_BRANCH commits_build_folder
cd commits_build_folder
west update
rm -rf $ZEPHYR_FOLDER
cp -rf $WORKSPACE $WORK_DIR/commits_build_folder/$ZEPHYR_FOLDER
cd $WORK_DIR/commits_build_folder/$ZEPHYR_FOLDER
git checkout $SOURCE_BRANCH
git rev-list origin/$ZEPHYR_BRANCH..HEAD | nl | sort -nr  > commits.txt
echo "printing commit ids"
cat commits.txt
filename='commits.txt'
boards_samples=$4
while read i id;
  do
  echo "**************************************************"
  west update -n > /dev/null
  git stash -u
  git checkout $id
  build=build_$id
  git log -1
    while IFS=, read -r board sample file
      do
      if [ -d "$sample" ] # check for sample folder exists
      then
        command="west build -b $board $sample"
        if [ ! -z "$file" ] # check for CONF_FILE not null
        then
          command="${command} -DCONF_FILE=$file"
        fi
        mkdir -p $build
        command="${command} -d $build"
        $command > $build/temp.txt
        if [ $? != 0 ]
        then
          cat $build/temp.txt
          echo "$id is unbuildable commit"
          exit -1;
        else
          echo "$id is buildable commit for $sample application of board $board $file"
          rm -rf $build
        fi
      else
        echo "no $sample application !!!"
      fi
    done < "$boards_samples"
done < "$filename"
git clean -fdx
