#!/bin/bash -ex

/usr/local/opt/qt/bin/qmake ./local_runner.pro
make

LOCAL_RUNNER_PATH=../../../../local_runner.app
rm -rf $LOCAL_RUNNER_PATH
mv -f ./local_runner.app $LOCAL_RUNNER_PATH
