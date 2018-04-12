#!/bin/bash -ex

case "$1" in
all)
  "$0" server_runner
  make distclean
  "$0" local_runner
  make distclean
  exit
  ;;
server_runner)
  PROJECT="server_runner"
  TARGET="server_runner"
  ;;
*)
  PROJECT="local_runner"
  TARGET="local_runner.app"
  ;;
esac

/usr/local/opt/qt/bin/qmake "$PROJECT.pro"
make

OUTPUT="../$TARGET"
rm -rf "$OUTPUT"
mv -f "$TARGET" "$OUTPUT"
