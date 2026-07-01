pipeline {
    agent any 
    
    options {
        timeout(time: 3, unit: 'HOURS')
        buildDiscarder(logRotator(numToKeepStr: '10'))
        timestamps()
    }

    triggers {
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
                echo 'Spawning Zephyr CI environment and running QEMU tests...'
                
                bat 'docker run --rm -v "%WORKSPACE%":/build -w /build zephyrprojectrtos/ci:latest bash ./ci-test.sh'
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
