#!/usr/bin/env bash
# Builds the Windows installer into dist/ (run in Git Bash):
#   meandriswrite-<version>-windows-x64-setup.exe   Windows 10 (1809+) and 11, 64-bit
#   SHA256SUMS-windows.txt
# Needs Qt, MinGW, CMake and Ninja in C:\Qt (aqtinstall) and Inno Setup 6; see the README, "Build from source".
set -euo pipefail
cd "$(dirname "$0")/.."
source packaging/windows/env.sh

version="$(sed -n 's/^project(meandriswrite VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
setup="meandriswrite-$version-windows-x64-setup.exe"

iscc=""
for dir in "$LOCALAPPDATA/Programs/Inno Setup 6" "/c/Program Files (x86)/Inno Setup 6" "/c/Program Files/Inno Setup 6"; do
    if [[ -x "$dir/ISCC.exe" ]]; then iscc="$dir/ISCC.exe"; break; fi
done
[[ -n "$iscc" ]] || { echo "Inno Setup 6 not found: winget install JRSoftware.InnoSetup" >&2; exit 1; }

./build.sh test
./build.sh

# Only the exe, its Qt files and the licences go into the installer, not the rest of build/.
stage=build/windows-stage
rm -rf "$stage"
mkdir -p "$stage"
cp build/meandriswrite.exe "$stage/"
deploy_qt "$stage/meandriswrite.exe"
cp LICENSE "$stage/LICENSE.txt"
cp packaging/THIRD-PARTY.txt packaging/LGPL-3.0.txt "$stage/"

mkdir -p dist
rm -f dist/meandriswrite-*-windows-*-setup.exe dist/SHA256SUMS-windows.txt
# MSYS_NO_PATHCONV: Git Bash would otherwise treat the /D options as paths.
MSYS_NO_PATHCONV=1 "$iscc" /Q "/DAppVersion=$version" "/DSourceDir=$(cygpath -aw "$stage")" \
    "/DOutputDir=$(cygpath -aw dist)" "$(cygpath -aw packaging/windows/meandriswrite.iss)"

(cd dist && sha256sum "$setup" | sed 's/ \*/  /' > SHA256SUMS-windows.txt)
echo
ls -lh "dist/$setup"
cat dist/SHA256SUMS-windows.txt
