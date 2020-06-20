node {
  stage ('checkout') {
    git '/f/Projects/cppext'
  }
  stage ('build') {
    bat returnStdout: true, script: "\"${tool 'vs2019.msbuild'}\" cppext.sln /t:Build /p:Configuration=Debug /p:Platform=x86 -m -v:m"
  }
  try {
    stage ('test') {
      bat returnStdout: true, script: "\"${tool 'vs2019.vstest'}\" Debug\\test.dll /Platform:x86 /Logger:trx;LogFileName=${BUILD_TAG}.trx"
    }
  } catch (err) {
    throw err
  }
  finally {
    mstest testResultsFile: "TestResults/${BUILD_TAG}.trx"
  }
}