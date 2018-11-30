//
// Copyright (c) 2020 Nordic Semiconductor ASA. All Rights Reserved.
//
// The information contained herein is confidential property of Nordic Semiconductor ASA.
// The use, copying, transfer or disclosure of such information is prohibited except by
// express written agreement with Nordic Semiconductor ASA.
//

@Library("CI_LIB") _

HashMap CI_STATE = lib_State.getConfig(JOB_NAME)
def TestExecutionList = [:]

properties([
  pipelineTriggers([
    parameterizedCron( [
        ((JOB_NAME =~ /latest\/night\/.*\/master/).find() ? CI_STATE.CFG.CRON.NIGHTLY : ''),
        ((JOB_NAME =~ /latest\/week\/.*\/master/).find() ? CI_STATE.CFG.CRON.WEEKLY : '')
    ].join('    \n') )
  ]),
  ( JOB_NAME.contains('sub/') ? disableResume() :  disableConcurrentBuilds() )
])

pipeline {

  parameters {
   booleanParam(name: 'RUN_DOWNSTREAM', description: 'if false skip downstream jobs', defaultValue: false)
   booleanParam(name: 'RUN_TESTS', description: 'if false skip testing', defaultValue: true)
   booleanParam(name: 'RUN_BUILD', description: 'if false skip building', defaultValue: true)
   booleanParam(name: 'RUN_BUILD_UPSTREAM', description: 'if false skip building', defaultValue: true)
   string(name: 'SANITYCHECK_RETRY_NUM', description: 'Default number of sanitycheck retries', defaultValue: '7')
   string(name: 'PLATFORMS', description: 'Default Platforms to test', defaultValue: 'nrf9160dk_nrf9160 nrf9160dk_nrf9160ns nrf52dk_nrf52832 nrf52840dk_nrf52840')
   string(name: 'jsonstr_CI_STATE', description: 'Default State if no upstream job', defaultValue: CI_STATE.CFG.INPUT_STATE_STR)
   choice(name: 'CRON', choices: ['COMMIT', 'NIGHTLY', 'WEEKLY'], description: 'Cron Test Phase')
  }
  agent { label CI_STATE.CFG.AGENT_LABELS }

  options {
    // Checkout the repository to this folder instead of root
    checkoutToSubdirectory('zephyr')
    parallelsAlwaysFailFast()
    timeout(time: CI_STATE.CFG.TIMEOUT.time, unit: CI_STATE.CFG.TIMEOUT.unit)
  }

  environment {
      // ENVs for check-compliance
      GH_TOKEN = credentials('nordicbuilder-compliance-token') // This token is used to by check_compliance to comment on PRs and use checks
      GH_USERNAME = "NordicBuilder"
      COMPLIANCE_ARGS = "-r NordicPlayground/fw-nrfconnect-zephyr --exclude-module Kconfig"

      LC_ALL = "C.UTF-8"

      // ENVs for sanitycheck
      ARCH = "-a arm"
      SANITYCHECK_OPTIONS = "--inline-logs --enable-coverage --coverage-platform arm -N" // DEFAULT: --testcase-root tests
      SANITYCHECK_RETRY_CMDS = '' // initialized so that it is shared to parrallel stages
  }

  stages {
    stage('Load') { steps { script { CI_STATE = lib_State.load('ZEPHYR', CI_STATE) }}}
    stage('Specification') { steps { script {

      def TestStages = [:]

      TestStages["compliance"] = {
        node (CI_STATE.CFG.AGENT_LABELS) {
          stage('Compliance Test'){
            println "Using Node:$NODE_NAME"
            docker.image("$CI_STATE.CFG.DOCKER_REG/$CI_STATE.CFG.IMAGE_TAG").inside {
              dir('zephyr') {
                checkout scm
                CI_STATE.SELF.REPORT_SHA = lib_Main.checkoutRepo(
                      CI_STATE.SELF.GIT_URL, "ZEPHYR", CI_STATE.SELF, false)
                lib_Status.set("PENDING", 'ZEPHYR', CI_STATE);
                lib_West.AddManifestUpdate("ZEPHYR", 'zephyr',
                      CI_STATE.SELF.GIT_URL, CI_STATE.SELF.GIT_REF, CI_STATE)
              }
              lib_West.InitUpdate('zephyr')
              lib_West.ApplyManifestUpdates(CI_STATE)

              dir('zephyr') {
                def BUILD_TYPE = lib_Main.getBuildType(CI_STATE.SELF)
                if (BUILD_TYPE == "PR") {
                  if (CI_STATE.SELF.IS_MERGEUP) {
                    println 'This is a MERGE-UP PR.   CI_STATE.SELF.IS_MERGEUP=' + CI_STATE.SELF.IS_MERGEUP
                    CI_STATE.SELF.MERGEUP_BASE = sh( script: "git log --oneline --grep='\\[nrf mergeup\\].*' -i -n 1 --pretty=format:'%h' | tr -d '\\n'" , returnStdout: true)
                    println "CI_STATE.SELF.MERGEUP_BASE = $CI_STATE.SELF.MERGEUP_BASE"
                    COMMIT_RANGE = "$CI_STATE.SELF.MERGEUP_BASE..$CI_STATE.SELF.REPORT_SHA"
                  } else {
                    COMMIT_RANGE = "$CI_STATE.SELF.MERGE_BASE..$CI_STATE.SELF.REPORT_SHA"
                  }
                  COMPLIANCE_ARGS = "$COMPLIANCE_ARGS -p $CHANGE_ID -S $CI_STATE.SELF.REPORT_SHA -g"
                  println "Building a PR [$CHANGE_ID]: $COMMIT_RANGE"
                }
                else if (BUILD_TYPE == "TAG") {
                  COMMIT_RANGE = "tags/${env.BRANCH_NAME}..tags/${env.BRANCH_NAME}"
                  println "Building a Tag: " + COMMIT_RANGE
                }
                // If not a PR, it's a non-PR-branch or master build. Compare against the origin.
                else if (BUILD_TYPE == "BRANCH") {
                  COMMIT_RANGE = "origin/${env.BRANCH_NAME}..HEAD"
                  println "Building a Branch: " + COMMIT_RANGE
                }
                else {
                    assert condition : "Build fails because it is not a PR/Tag/Branch"
                }

                // Run the compliance check
                try {
                  sh "(source ../zephyr/zephyr-env.sh && ../tools/ci-tools/scripts/check_compliance.py $COMPLIANCE_ARGS --commits $COMMIT_RANGE)"
                }
                finally {
                  junit 'compliance.xml'
                  archiveArtifacts artifacts: 'compliance.xml'
                }
              }
            }
          }
        }
      }

      CI_STATE.SELF.IS_MERGEUP = false
      if (CI_STATE.SELF.containsKey('CHANGE_TITLE')) {
        if (((CI_STATE.SELF.CHANGE_TITLE.toLowerCase().contains('mergeup')  ) || (CI_STATE.SELF.CHANGE_TITLE.toLowerCase().contains('upmerge')  )) &&
            ((CI_STATE.SELF.CHANGE_BRANCH.toLowerCase().contains('mergeup') ) || (CI_STATE.SELF.CHANGE_BRANCH.toLowerCase().contains('upmerge') ))) {
          CI_STATE.SELF.IS_MERGEUP = true
          println 'This is a MERGE-UP PR.   CI_STATE.SELF.IS_MERGEUP=' + CI_STATE.SELF.IS_MERGEUP
        }
      }

      if (CI_STATE.SELF.CRON == 'COMMIT') {
        println "Running Commit Tests"
      } else if (CI_STATE.SELF.CRON == 'NIGHTLY') {
        println "Running Nightly Tests"
      } else if (CI_STATE.SELF.CRON == 'WEEKLY') {
        println "Running Weekly Tests"
      }

      if (CI_STATE.SELF.RUN_TESTS) {
        TestExecutionList['compliance'] = TestStages["compliance"]
      }

      if (CI_STATE.SELF.RUN_BUILD) {
        def SUBSET_LIST = ['1/4', '2/4', '3/4', '4/4' ]
        def COMPILER_LIST = ['gnuarmemb'] // 'zephyr',
        def INPUT_MAP = [set : SUBSET_LIST, compiler : COMPILER_LIST ]

        def OUTPUT_MAP = INPUT_MAP.values().combinations { args ->
            [INPUT_MAP.keySet().toList(), args].transpose().collectEntries { [(it[0]): it[1]]}
        }

        SANITYCHECK_RETRY_CMDS_LIST = []
        if (!CI_STATE.SELF.SANITYCHECK_RETRY_NUM) {
          CI_STATE.SELF.SANITYCHECK_RETRY_NUM='7'
        }
        for (i=1; i <= CI_STATE.SELF.SANITYCHECK_RETRY_NUM.toInteger(); i++) {
          SANITYCHECK_RETRY_CMDS_LIST.add("(sleep 30; ./scripts/sanitycheck $SANITYCHECK_OPTIONS --only-failed)")
        }
        SANITYCHECK_RETRY_CMDS = SANITYCHECK_RETRY_CMDS_LIST.join(' || \n')

        def sanityCheckNRFStages = OUTPUT_MAP.collectEntries {
            ["SanityCheckNRF\n${it.compiler}\n${it.set}" : generateParallelStageNRF(it.set, it.compiler, JOB_NAME, CI_STATE)]
        }
        TestExecutionList = TestExecutionList.plus(sanityCheckNRFStages)

        if (CI_STATE.SELF.RUN_BUILD_UPSTREAM) {
          def sanityCheckALLStages = OUTPUT_MAP.collectEntries {
              ["SanityCheckALL\nzephyr\n${it.set}" : generateParallelStageALL(it.set, 'zephyr', JOB_NAME, CI_STATE)]
          }
          TestExecutionList = TestExecutionList.plus(sanityCheckALLStages)
        }
      }

      println "TestExecutionList = $TestExecutionList"

    }}}

    stage('Execution') { steps { script {
      parallel TestExecutionList
    }}}

    stage('Trigger Downstream Jobs') {
      when { expression { CI_STATE.SELF.RUN_DOWNSTREAM } }
      steps { script { lib_Stage.runDownstream(JOB_NAME, CI_STATE) } }
    }

    stage('Report') {
      when { expression { CI_STATE.SELF.RUN_TESTS } }
      steps { script {
          println 'no report generation yet'
      } }
    }

  }

  post {
    // This is the order that the methods are run. {always->success/abort/failure/unstable->cleanup}
    always {
      echo "always"
      script { if ( !CI_STATE.SELF.RUN_BUILD && !CI_STATE.SELF.RUN_TESTS ) { currentBuild.result = "UNSTABLE"}}
    }
    success {
      echo "success"
      script { lib_Status.set("SUCCESS", 'ZEPHYR', CI_STATE) }
    }
    aborted {
      echo "aborted"
      script { lib_Status.set("ABORTED", 'ZEPHYR', CI_STATE) }
    }
    unstable {
      echo "unstable"
      script { lib_Status.set("FAILURE", 'ZEPHYR', CI_STATE) }
    }
    failure {
      echo "failure"
      script { lib_Status.set("FAILURE", 'ZEPHYR', CI_STATE) }
    }
    cleanup {
        echo "cleanup"
        cleanWs()
    }
  }
}

def generateParallelStageNRF(subset, compiler, JOB_NAME, CI_STATE) {
  return {
    node (CI_STATE.CFG.AGENT_LABELS) {
      try {
        stage('Sanity Check - Zephyr'){
          println "Using Node:$NODE_NAME"
          docker.image("$CI_STATE.CFG.DOCKER_REG/$CI_STATE.CFG.IMAGE_TAG").inside {
            dir('zephyr') {
              checkout scm
              CI_STATE.SELF.REPORT_SHA = lib_Main.checkoutRepo(
                    CI_STATE.SELF.GIT_URL, "ZEPHYR", CI_STATE.SELF, false)
              lib_West.AddManifestUpdate("ZEPHYR", 'zephyr',
                    CI_STATE.SELF.GIT_URL, CI_STATE.SELF.GIT_REF, CI_STATE)
            }
            lib_West.InitUpdate('zephyr')
            lib_West.ApplyManifestUpdates(CI_STATE)
            dir('zephyr') {
              def PLATFORM_ARGS = lib_Main.getPlatformArgs(CI_STATE.SELF.PLATFORMS)
              println "$compiler SANITY NRF PLATFORMS_ARGS = $PLATFORM_ARGS"
              sh """
                  export ZEPHYR_TOOLCHAIN_VARIANT='$compiler' && \
                  source zephyr-env.sh && \
                  ./scripts/sanitycheck $SANITYCHECK_OPTIONS $ARCH $PLATFORM_ARGS --subset $subset || $SANITYCHECK_RETRY_CMDS
                 """
            }
          }
        }
      } finally {
        cleanWs(); echo "Run: cleanWs()"
      }
    }
  }
}

def generateParallelStageALL(subset, compiler, JOB_NAME, CI_STATE) {
  return {
    node (CI_STATE.CFG.AGENT_LABELS) {
      try {
        stage('Sanity Check - Zephyr'){
          println "Using Node:$NODE_NAME"
          docker.image("$CI_STATE.CFG.DOCKER_REG/$CI_STATE.CFG.IMAGE_TAG").inside {
            dir('zephyr') {
              checkout scm
              CI_STATE.SELF.REPORT_SHA = lib_Main.checkoutRepo(
                    CI_STATE.SELF.GIT_URL, "ZEPHYR", CI_STATE.SELF, false)
              lib_West.AddManifestUpdate("ZEPHYR", 'zephyr',
                    CI_STATE.SELF.GIT_URL, CI_STATE.SELF.GIT_REF, CI_STATE)
            }
            lib_West.InitUpdate('zephyr')
            lib_West.ApplyManifestUpdates(CI_STATE)
            dir('zephyr') {
              sh """
                  export ZEPHYR_TOOLCHAIN_VARIANT='$compiler' && \
                  source zephyr-env.sh && \
                  ./scripts/sanitycheck $SANITYCHECK_OPTIONS $ARCH --subset $subset || $SANITYCHECK_RETRY_CMDS
                 """
            }
          }
        }
      } finally {
        cleanWs(); echo "Ran: cleanWs()"
      }
    }
  }
}

def generateComplianceStage(AGENT_LABELS, DOCKER_REG, IMAGE_TAG, JOB_NAME, CI_STATE) {
  return {
    node (AGENT_LABELS) {
      try {
        stage('Compliance Test'){
          println "Using Node:$NODE_NAME"
          docker.image("$DOCKER_REG/$IMAGE_TAG").inside {
            dir('zephyr') {
              checkout scm
              CI_STATE.ZEPHYR.REPORT_SHA = lib_Main.checkoutRepo(
                    CI_STATE.ZEPHYR.GIT_URL, "ZEPHYR", CI_STATE.ZEPHYR, false)
              lib_Status.set("PENDING", 'ZEPHYR', CI_STATE);
              lib_West.AddManifestUpdate("ZEPHYR", 'zephyr',
                    CI_STATE.ZEPHYR.GIT_URL, CI_STATE.ZEPHYR.GIT_REF, CI_STATE)
            }
            lib_West.InitUpdate('zephyr')
            lib_West.ApplyManifestUpdates(CI_STATE)

            dir('zephyr') {
              def BUILD_TYPE = lib_Main.getBuildType(CI_STATE.ZEPHYR)
              if (BUILD_TYPE == "PR") {
                if (CI_STATE.ZEPHYR.IS_MERGEUP) {
                  println 'This is a MERGE-UP PR.   CI_STATE.ZEPHYR.IS_MERGEUP=' + CI_STATE.ZEPHYR.IS_MERGEUP
                  CI_STATE.ZEPHYR.MERGEUP_BASE = sh( script: "git log --oneline --grep='\\[nrf mergeup\\].*' -i -n 1 --pretty=format:'%h' | tr -d '\\n'" , returnStdout: true)
                  println "CI_STATE.ZEPHYR.MERGEUP_BASE = $CI_STATE.ZEPHYR.MERGEUP_BASE"
                  COMMIT_RANGE = "$CI_STATE.ZEPHYR.MERGEUP_BASE..$CI_STATE.ZEPHYR.REPORT_SHA"
                } else {
                  COMMIT_RANGE = "$CI_STATE.ZEPHYR.MERGE_BASE..$CI_STATE.ZEPHYR.REPORT_SHA"
                }
                COMPLIANCE_ARGS = "$COMPLIANCE_ARGS -p $CHANGE_ID -S $CI_STATE.ZEPHYR.REPORT_SHA -g"
                println "Building a PR [$CHANGE_ID]: $COMMIT_RANGE"
              }
              else if (BUILD_TYPE == "TAG") {
                COMMIT_RANGE = "tags/${env.BRANCH_NAME}..tags/${env.BRANCH_NAME}"
                println "Building a Tag: " + COMMIT_RANGE
              }
              // If not a PR, it's a non-PR-branch or master build. Compare against the origin.
              else if (BUILD_TYPE == "BRANCH") {
                COMMIT_RANGE = "origin/${env.BRANCH_NAME}..HEAD"
                println "Building a Branch: " + COMMIT_RANGE
              }
              else {
                  assert condition : "Build fails because it is not a PR/Tag/Branch"
              }

              // Run the compliance check
              try {
                sh "source ../zephyr/zephyr-env.sh &&  \
                    pip install --user -r ../tools/ci-tools/requirements.txt && \
                    ../tools/ci-tools/scripts/check_compliance.py $COMPLIANCE_ARGS --commits $COMMIT_RANGE"
              }
              finally {
                junit 'compliance.xml'
                archiveArtifacts artifacts: 'compliance.xml'
              }
            }
          }
        }
      } finally {
        cleanWs(); echo "Ran: cleanWs()"
      }
    }
  }
}