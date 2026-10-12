#!/usr/bin/env bash
# One-command build for Linux, macOS and Windows (Git Bash).
#   ./build.sh          release build
#   ./build.sh debug    debug build
#   ./build.sh test     build and run the unit tests
set -euo pipefail
cd "$(dirname "$0")"

build_type=Release
build_dir=build
cmake_args=()
case "${1:-}" in
    debug) build_type=Debug ;;
    test)  build_dir=build-tests; cmake_args+=(-DMEANDRISWRITE_BUILD_TESTS=ON) ;;
    "")    ;;
    *)     echo "usage: $0 [debug|test]" >&2; exit 1 ;;
esac

case "$(uname -s)" in
    Darwin)
        # Apple Silicon only, macOS 12 and newer (Qt 6.8's minimum).
        source packaging/macos/env.sh
        cmake_args+=(-DCMAKE_PREFIX_PATH="$qt_root" -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0) ;;
    MINGW*|MSYS*)
        source packaging/windows/env.sh
        cmake_args+=(-G Ninja -DCMAKE_PREFIX_PATH="$(cygpath -m "$qt_root")") ;;
esac

cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_type" ${cmake_args[@]+"${cmake_args[@]}"}
cmake --build "$build_dir" --parallel

if [[ "${1:-}" == test ]]; then
    ctest --test-dir "$build_dir" --output-on-failure
elif [[ "$(uname)" == Darwin ]]; then
    echo "Built $build_dir/meandriswrite.app  (run: open $build_dir/meandriswrite.app)"
elif [[ -f "$build_dir/meandriswrite.exe" ]]; then
    # Copy the Qt DLLs next to the exe so it also starts outside this shell.
    deploy_qt "$build_dir/meandriswrite.exe"
    echo "Built $build_dir/meandriswrite.exe  (run: $build_dir/meandriswrite.exe)"
else
    echo "Built $build_dir/meandriswrite  (run: ./$build_dir/meandriswrite)"
fi
