#!/usr/bin/env bash

set -e

shopt -s extglob

if [ -z "$1" ] || [ -z "$2" ]; then
  echo "Usage: $0 version destdir [--no-package] [--dev-build]"
  exit 1
fi

DXVK_VERSION="$1"
DXVK_SRC_DIR=$(readlink -f "$0")
DXVK_SRC_DIR=$(dirname "$DXVK_SRC_DIR")
DXVK_BUILD_DIR=$(realpath "$2")"/dxvk-$DXVK_VERSION"
DXVK_ARCHIVE_PATH=$(realpath "$2")"/dxvk-$DXVK_VERSION.tar.gz"

if [ -e "$DXVK_BUILD_DIR" ]; then
  echo "Build directory $DXVK_BUILD_DIR already exists"
  exit 1
fi

shift 2

opt_nopackage=0
opt_devbuild=0
opt_buildid=false
opt_64_only=0
opt_32_only=0

crossfile="build-win"

while [ $# -gt 0 ]; do
  case "$1" in
  "--no-package")
    opt_nopackage=1
    ;;
  "--dev-build")
    opt_nopackage=1
    opt_devbuild=1
    ;;
  "--build-id")
    opt_buildid=true
    ;;
  "--64-only")
    opt_64_only=1
    ;;
  "--32-only")
    opt_32_only=1
    ;;
  *)
    echo "Unrecognized option: $1" >&2
    exit 1
  esac
  shift
done

function build_arch {
  export WINEARCH="win$2"
  export WINEPREFIX="$DXVK_BUILD_DIR/wine.$1"
  
  cd "$DXVK_SRC_DIR"

  opt_strip=
  #if [ $opt_devbuild -eq 0 ]; then
  #  opt_strip=--strip
  #fi

  # instead of --debug
  CXXFLAGS='-fno-omit-frame-pointer -g1 -ffunction-sections -fdata-sections' \
  LDFLAGS='-Wl,--gc-sections' \
  meson setup --cross-file "$DXVK_SRC_DIR/$crossfile$1.txt" \
        --buildtype "release"                               \
        -Db_lto=true \
        --prefix "$DXVK_BUILD_DIR"                          \
        $opt_strip                                          \
        --bindir "$3"                                      \
        --libdir "$3"                                      \
        -Db_ndebug=false                                    \
        -Dbuild_id=$opt_buildid                             \
        "$DXVK_BUILD_DIR/build.$1"

  cd "$DXVK_BUILD_DIR/build.$1"
  ninja -v install

  if [ $opt_devbuild -eq 0 ]; then
    # get rid of some useless .a files
    rm "$DXVK_BUILD_DIR/$3/"*.!(dll)
    rm -R "$DXVK_BUILD_DIR/build.$1"
  fi
}

function package {
  cd "$DXVK_BUILD_DIR/.."
  tar -czf "$DXVK_ARCHIVE_PATH" "dxvk-$DXVK_VERSION"
  rm -R "dxvk-$DXVK_VERSION"
}

if [ $opt_32_only -eq 0 ]; then
  build_arch 64 64 x64
  #build_arch aarch64 64 aarch64
  build_arch arm64ec 64 arm64ec
fi
if [ $opt_64_only -eq 0 ]; then
  build_arch 32 32 x32
fi

if [ $opt_nopackage -eq 0 ]; then
  package
fi
