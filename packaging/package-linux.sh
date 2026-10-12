#!/usr/bin/env bash
# Builds build/meandriswrite-<arch>.AppImage with linuxdeploy (downloaded on first use).
set -euo pipefail
cd "$(dirname "$0")/.."

./build.sh

arch="$(uname -m)"
tools=build/tools
appdir=build/AppDir
mkdir -p "$tools"
for tool in linuxdeploy linuxdeploy-plugin-qt; do
    file="$tools/$tool-$arch.AppImage"
    if [[ ! -x "$file" ]]; then
        curl -fsSL -o "$file" "https://github.com/linuxdeploy/$tool/releases/download/continuous/$tool-$arch.AppImage"
        chmod +x "$file"
    fi
done

rm -rf "$appdir"
DESTDIR="$appdir" cmake --install build --prefix /usr
# The AppImage bundles Qt, so the licences go along, as in the Windows and macOS downloads.
doc="$appdir/usr/share/doc/meandriswrite"
mkdir -p "$doc"
cp LICENSE "$doc/LICENSE.txt"
cp packaging/THIRD-PARTY.txt packaging/LGPL-3.0.txt "$doc/"

export PATH="$PWD/$tools:$PATH"
export QMAKE="${QMAKE:-$(command -v qmake6 || echo /usr/lib/qt6/bin/qmake)}"
export APPIMAGE_EXTRACT_AND_RUN=1 # no FUSE needed (containers, CI)
export LDAI_OUTPUT="build/meandriswrite-$arch.AppImage"
export OUTPUT="$LDAI_OUTPUT"

"$tools/linuxdeploy-$arch.AppImage" --appdir "$appdir" --plugin qt --output appimage
echo "Built $LDAI_OUTPUT"
