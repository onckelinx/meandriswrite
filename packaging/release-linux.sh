#!/usr/bin/env bash
# Builds the Linux release files into dist/:
#   meandriswrite-<version>-x86_64.AppImage   any Linux (built on Ubuntu 22.04, so it runs on 22.04 and newer)
#   meandriswrite_<version>_amd64.deb         Ubuntu 22.04+, Debian 12+, Mint 21+
#   meandriswrite-<version>-1.x86_64.rpm      Fedora (built on the Fedora release below)
#   SHA256SUMS-linux.txt                      checksums of the three files
# Everything is compiled inside throwaway podman containers, so this machine stays clean.
set -euo pipefail
cd "$(dirname "$0")/.."

ubuntu_image=docker.io/library/ubuntu:22.04
fedora_image=registry.fedoraproject.org/fedora:44
version="$(sed -n 's/^project(meandriswrite VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"

rm -rf dist
mkdir -p dist

# Each container gets a copy of the sources and its own build folder, so nothing it
# builds ends up in this checkout except the finished files in dist/.
run_in() {
    local image="$1" script="$2"
    podman run --rm --security-opt label=disable \
        -v "$PWD:/src:ro" -v "$PWD/dist:/dist" -v meandriswrite-tools:/tools \
        "$image" bash -euo pipefail -c "
            cp -r /src /work && cd /work && rm -rf build build-* dist
            $script"
}

echo "== Ubuntu 22.04: .deb and AppImage"
run_in "$ubuntu_image" "
    export DEBIAN_FRONTEND=noninteractive
    apt-get update -qq
    apt-get install -y -qq --no-install-recommends cmake g++ make file curl ca-certificates dpkg-dev \
        qt6-base-dev qt6-base-dev-tools qt6-qpa-plugins qt6-wayland libgl-dev >/dev/null
    ./build.sh test
    ./build.sh
    (cd build && cpack -G DEB >/dev/null)
    cp build/*.deb /dist/
    mkdir -p build/tools && cp /tools/*.AppImage build/tools/ 2>/dev/null || true
    EXTRA_PLATFORM_PLUGINS=libqwayland-generic.so \
        packaging/package-linux.sh
    cp build/tools/*.AppImage /tools/
    cp build/meandriswrite-x86_64.AppImage /dist/meandriswrite-$version-x86_64.AppImage
"

echo "== Fedora: .rpm"
run_in "$fedora_image" "
    dnf install -y -q gcc-c++ cmake qt6-qtbase-devel rpm-build appstream desktop-file-utils >/dev/null
    appstreamcli validate --no-net packaging/linux/*.metainfo.xml
    desktop-file-validate packaging/linux/meandriswrite.desktop
    ./build.sh
    (cd build && cpack -G RPM >/dev/null)
    cp build/*.rpm /dist/
"

(cd dist && sha256sum -- *.AppImage *.deb *.rpm > SHA256SUMS-linux.txt)

echo
ls -lh dist
