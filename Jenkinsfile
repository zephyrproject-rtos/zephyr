pipeline {
    agent any 
    
    options {
        timeout(time: 120, unit: 'MINUTES')
        buildDiscarder(logRotator(numToKeepStr: '10'))
        timestamps()
    }
    
    stages {
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
