#!/bin/bash

########################################################################
#
# Purpose: CI script for compliance check
# Interface:
#   - Actions
#     
# Environment: 
#  - BASE_REF
#  - GIT_CHECKOUT_SHA
# 
declare -r SELF="$(basename $0)"
# actions
declare -r A_MAINTAINER_CHECK="maintainer_check"
declare -r A_COMPLIANCE_CHECK="check_compliance"
# options for compliance checks:
declare -r CC_ALL="all"
declare -r CC_CHECKOUT="checkout"
declare -r CC_PATCHUPDATE="patch_update"
declare -r CC_CACHE="cache"
declare -r CC_PYTHON_DEPS="python-deps"
declare -r CC_WEST_SETUP="west-setup"
declare -r CC_COMPLIANCE_TESTS="compliance-tests"
declare -r CC_UPLOAD_RESULTS="upload-results"
declare -r CC_CHECK_WARNS="check-warns"
declare -r DEF_CC_STEPS="${CC_ALL}"

function usage() {

   cat <<EOT
   Usage: ${SELF} ${A_MAINTAINER_CHECK}
          ${SELF} ${A_COMPLIANCE_CHECK} [${CC_CHECKOUT}|${CC_PATCHUPDATE}|${CC_CACHE}|${CC_PYTHON_DEPS}|${CC_WEST_SETUP}|${CC_COMPLIANCE_TESTS}|${CC_UPLOAD_RESULTS}|${CC_CHECK_WARNS}
    
EOT
}

function do_git_checkout() {

    local sha="$1"
    
    #### TODO, only needed for non-github CI systems
    
    return 0
}

function maintainer_check() {

    if [ -z ${BASE_REF} ] ; then
        echo "${SELF}: ${FUNCNAME}: error: env var BASE_REF not set..."
        return 1
    fi
    
    python3 ./scripts/get_maintainer.py path CMakeLists.txt

    return $?
}

function check_compliance() {

    local step="$1"
    
    if [ ${step} == "all" ] ; then
        steps = "checkout path-update cache python-deps west-setup cg-checks check-warns"
    else
        steps="${step}"
    fi
    
    for step in ${steps} ; do
    
        case ${step} in
        
            "${CC_CHECKOUT}")
        	if ! do_git_checkout ${GIT_CHECKOUT_SHA} ; then
            	    return 1
        	fi    
        	;;
             "${CC_CHECKOUT}")
                export PATH="$HOME/.local/bin":${PATH}  ##### TODO: confirm GITHUB_PATH is set. But then, the name means what outside of github...
                ;;
             "${CC_CACHE}")
                true    #### TODO: managed by github or other CI systems, needed here?
                ;;
              "${CC_PYTHON_DEPS}")
                for pip_pack in setuptools wheel python-magic junitparser gitlint pylint pykwalify west ; do
                  if ! pip3 install "${pip_pack}" ; then
                       echo "${SELF}: ${FUNCNAME}: step=${step}: error: pip failed to install ${pip_pack}"
                       return 1
                  fi
                done
                ;;
              "${CC_WEST_SETUP}")
	        git config --global user.email "you@example.com"
        	git config --global user.name "Your Name"
	        git remote -v
        	git rebase origin/${BASE_REF}
        	# debug
        	git log  --pretty=oneline | head -n 10
        	west init -l . || true
        	west update 2>&1 1> west.update.log || west update 2>&1 1> west.update2.log                
        	;;
        	
              "${CC_COMPLIANCE_TESTS}")
        	export ZEPHYR_BASE=$PWD
        	./scripts/ci/check_compliance.py -m Devicetree -m Gitlint -m Identity -m Nits -m pylint -m checkpatch -m Kconfig -c origin/${BASE_REF}..    
        	return $?
        	;;  
              
              "${CC_UPLOAD_RESULTS}")
                true   #### TODO: managed by github, need to implement for other CI systems?
                ;;
                
              "${CC_CHECK_WARNS}")
	        if [[ ! -s "compliance.xml" ]]; then
        	  return 1;
      	        fi

	        for file in Nits.txt checkpatch.txt Identity.txt Gitlint.txt pylint.txt Devicetree.txt Kconfig.txt; do
	          if [[ -s $file ]]; then
        	    errors=$(cat $file)
        	    errors="${errors//'%'/'%25'}"
        	    errors="${errors//$'\n'/'%0A'}"
        	    errors="${errors//$'\r'/'%0D'}"
        	    echo "::error file=${file}::$errors"
        	    ret=1
        	  fi
        	done

        	if [ "${ret}" == "1" ]; then
          	  return 1;
        	fi
                ;;
                
              *)
                echo "${SELF}: ${FUNCNAME}: error: ${step}: unsupported..."
                return 1
                ;;
        esac
    
    done
    
    return 0
}

# Script starts here
declare CC_STEPS="${DEF_CC_STEPS}"

case $1 in
    -h|--help)
         usage; exit ;;
         
    "${A_MAINTAINER_CHECK}")
         maintainer_check
         exit $?
         ;;
         
    "${A_COMPLIANCE_CHECK}")
         shift ;
         if [ $# -gt 0 ] ; then
             CC_STEPS="$#"
         fi
         check_compliance ${CC_STEPS}
         exit $?
         ;;
    *)
         echo "${SELF}: error: $1: unsupported action..."
         exit 1
esac
