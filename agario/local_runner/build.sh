#!/bin/bash -ex

PROJECT="${1:-local_runner}"

case "$PROJECT" in
server_runner)
  TARGET=server_runner
  ;;
*)
  TARGET=local_runner.app
  ;;
esac

/usr/local/opt/qt/bin/qmake "$PROJECT.pro"
make

OUTPUT="../$TARGET"
rm -rf "$OUTPUT"
mv -f "$TARGET" "$OUTPUT"
