#!/usr/bin/env bash
# Builds the macOS download into dist/:
#   meandriswrite-<version>-macos-arm64.dmg   Apple Silicon, macOS 12 and newer
#   SHA256SUMS-macos.txt
# The .dmg holds "Meandris Write.app" (Qt bundled, ad-hoc signed), a link to Applications
# and the licences. Needs Qt from aqtinstall in ~/Qt; see the README, "Build from source".
set -euo pipefail
cd "$(dirname "$0")/.."
source packaging/macos/env.sh

version="$(sed -n 's/^project(meandriswrite VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
dmg="meandriswrite-$version-macos-arm64.dmg"

app=build/meandriswrite.app
rm -rf "$app" # start from a clean bundle, so nothing from an earlier deploy is left inside
./build.sh test
./build.sh

# Qt 6.8's macdeployqt doesn't sign; the copy in the .dmg is signed further down.
"$qt_root/bin/macdeployqt" "$app"

# Plugins the editor doesn't use (if present): the virtual keyboard pulls in QtQuick.
rm -f "$app"/Contents/PlugIns/platforminputcontexts/libqtvirtualkeyboardplugin.dylib \
      "$app"/Contents/PlugIns/imageformats/libqpdf.dylib \
      "$app"/Contents/PlugIns/iconengines/libqsvgicon.dylib

macho_files() {
    find "$app/Contents" -type f | while read -r f; do
        if file -b "$f" | grep -q Mach-O; then echo "$f"; fi
    done
}
# What a Mach-O file links to, without its own install name. (Lines ending in ":" are headers.)
linked_libs() {
    otool -L "$1" | awk '/^\t/ {print $1}' | grep -vxF "$(otool -D "$1" | grep -v ':$')" || true
}

# aqt's Qt is universal (arm64 + x86_64); the app is arm64 only, so drop the Intel half.
macho_files | while read -r f; do
    if [[ $(lipo -archs "$f") != arm64 ]]; then
        lipo -thin arm64 "$f" -output "$f.arm64" && mv "$f.arm64" "$f"
    fi
done

# Remove frameworks and libraries that nothing in the bundle links to any more.
removed=1
while ((removed)); do
    removed=0
    used=$(macho_files | while read -r f; do linked_libs "$f"; done)
    for lib in "$app"/Contents/Frameworks/*; do
        name=$(basename "$lib")
        if ! grep -qF "/$name" <<<"$used"; then
            rm -rf "$lib"
            removed=1
        fi
    done
done

# The executable still searches the Qt it was built with; point it at the bundle instead.
exe="$app/Contents/MacOS/meandriswrite"
for rpath in $(otool -l "$exe" | awk '/LC_RPATH/{getline; getline; print $2}'); do
    install_name_tool -delete_rpath "$rpath" "$exe"
done
install_name_tool -add_rpath @executable_path/../Frameworks "$exe"

# Every library the app links to must be inside the bundle or part of macOS.
broken=$(macho_files | while read -r f; do
    linked_libs "$f" | { grep -vE '^(/System/|/usr/lib/|@executable_path/|@loader_path/)' || true; } | while read -r dep; do
        name=${dep#@rpath/}
        [[ $dep == @rpath/* && -e "$app/Contents/Frameworks/$name" ]] || echo "  ${f#"$app"/} -> $dep"
    done
done)
if [[ -n $broken ]]; then
    echo "These libraries are not in the bundle:" >&2
    echo "$broken" >&2
    exit 1
fi

# What goes into the .dmg. Users see "Meandris Write.app"; the executable inside stays meandriswrite.
stage=build/macos-stage
rm -rf "$stage"
mkdir -p "$stage"
ditto "$app" "$stage/Meandris Write.app"
# Ad-hoc signature: needed to run on Apple Silicon; no Apple developer account needed.
codesign --force --deep --sign - "$stage/Meandris Write.app"
ln -s /Applications "$stage/Applications"
cp LICENSE "$stage/LICENSE.txt"
cp packaging/THIRD-PARTY.txt packaging/LGPL-3.0.txt "$stage/"

mkdir -p dist
rm -f dist/meandriswrite-*-macos-*.dmg dist/SHA256SUMS-macos.txt
hdiutil create -volname "Meandris Write" -srcfolder "$stage" -format UDZO -ov "dist/$dmg" >/dev/null
(cd dist && shasum -a 256 "$dmg" > SHA256SUMS-macos.txt)
echo
ls -lh "dist/$dmg"
cat dist/SHA256SUMS-macos.txt
