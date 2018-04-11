#!/bin/bash -ex

if [[ "$1" == "server_runner" ]]; then
  PROJECT="server_runner"
  TARGET="server_runner"
else
  PROJECT="local_runner"
  TARGET="local_runner.app"
fi

/usr/local/opt/qt/bin/qmake "$PROJECT.pro"
make

OUTPUT="../$TARGET"
rm -rf "$OUTPUT"
mv -f "$TARGET" "$OUTPUT"
