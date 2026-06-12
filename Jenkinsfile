pipeline {
    agent any 
    
    options {
        timeout(time: 3, unit: 'HOURS')
        buildDiscarder(logRotator(numToKeepStr: '10'))
        timestamps()
    }

    triggers {
        // cron('H 0 * * *')
        pollSCM('H/5 * * * *')
    }
    
    stages {
        stage('Pre-flight Cleanup') {
            steps {
                // Use standard Windows CMD to forcefully delete the zephyr folder quietly
                bat '''
                    if exist "zephyr" (
                        rmdir /s /q "zephyr"
                    )
                '''
            }
        }

        stage('Execute Containerized Tests') {
            steps {
                echo 'Spawning clean Ubuntu environment and running tests...'
                
                // Back to the clean, simple command
                bat 'docker run --rm -v "%WORKSPACE%":/build -w /build ubuntu:24.04 bash ./ci-test.sh'
            }
        }
    }

    post {
        always {
            echo 'Cleaning up Windows host workspace...'

            // Disables background deletion and forces immediate wipe
            cleanWs(disableDeferredWipeout: true, deleteDirs: true)
        }
    }
}
