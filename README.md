<p align="center"><img src="design/logo.png" alt="Meandris Write logo" width="200"></p>

# Meandris Write

By Rik Onckelinx - meandris.ai

A very small, fast, native text and Markdown editor for Windows, macOS and Linux, built with Qt 6.

- Tabs, plain-text find, word wrap, printing and PDF export.
- Markdown in **Source**, **Split** and **Preview** views (Ctrl+1 / 2 / 3), with a live preview.
- **Tools → Paste as Markdown / New from Clipboard** turns text copied from an LLM web page into Markdown.
- Reloads files that change on disk and asks before discarding unsaved work.

## Download and install

Download the file for your system from the [Releases page](https://github.com/onckelinx/meandriswrite/releases).

| System | File | How |
|---|---|---|
| Windows 10 and 11 (64-bit) | `meandriswrite-<version>-windows-x64-setup.exe` | Run it and choose **Only for me** (no administrator rights needed) or **All users**. Windows will warn you first: see [Unsigned downloads](#unsigned-downloads). |
| macOS 12 or newer, Apple Silicon (M1 or newer) | `meandriswrite-<version>-macos-arm64.dmg` | Open it and drag **Meandris Write** into **Applications**. macOS will block the first launch: see [Unsigned downloads](#unsigned-downloads). |
| Ubuntu, Debian, Linux Mint | `meandriswrite_<version>_amd64.deb` | Double-click it and choose **Install** (or `sudo apt install ./meandriswrite_*.deb`). |
| Fedora | `meandriswrite-<version>-1.x86_64.rpm` | Double-click it and choose **Install** (or `sudo dnf install ./meandriswrite-*.rpm`). |
| Any other Linux | `meandriswrite-<version>-x86_64.AppImage` | Right-click → Properties → allow executing, then double-click. Nothing is installed; delete the file to remove it. |

There is no version for Macs with an Intel processor.

## Unsigned downloads

The Windows and macOS downloads are **not code-signed**. Signing needs a paid certificate on
Windows and a paid Apple Developer membership on macOS, and this free hobby project doesn't pay
for either. Your system therefore warns you the first time, as it does for any unsigned app.

The files are built from the source code in this repository. You can [check that your
download is the published file](#checking-a-download), or [build it yourself](#build-from-source).

**Windows**

1. Your browser may say the file isn't commonly downloaded. In Edge, open the download's
   **…** menu → **Keep** (and **Keep anyway** if asked).
2. When you run the setup, **Windows protected your PC** appears. Click **More info** →
   **Run anyway**.

**macOS**

1. Double-click Meandris Write in Applications. macOS says it can't verify the app; click
   **Done** (not Move to Trash).
2. Open **System Settings → Privacy & Security**, scroll down to the message about Meandris Write
   and click **Open Anyway**, then confirm with your password.

On macOS 14 and older you can also Control-click the app → **Open** → **Open**. Or, in Terminal:
`xattr -dr com.apple.quarantine "/Applications/Meandris Write.app"`

You only need to do this once; after that Meandris Write starts normally.

**Linux** has no such warning.

### Checking a download

Each release includes `SHA256SUMS.txt`, with a checksum for every file. If the checksum of your
download matches, the file is exactly the one published here: not damaged and not replaced.

- Windows (PowerShell): `Get-FileHash .\meandriswrite-*-setup.exe`
- macOS: `shasum -a 256 meandriswrite-*.dmg`
- Linux: `sha256sum -c SHA256SUMS.txt --ignore-missing` in the download folder

Compare the result with the line for that file in `SHA256SUMS.txt`.

## Questions

### How do I open `.md` files with Meandris Write by default?

**Windows.** The installer doesn't register Meandris Write for any file type, so it never takes
over `.md` or `.txt` files on its own. To choose it yourself: right-click a `.md` file →
**Open with** → **Choose another app** → **Look for another app on this PC** (or **Choose an
app on your PC**), and select `meandriswrite.exe`. It is in `%LOCALAPPDATA%\Programs\meandriswrite`
if you installed it only for yourself, or in `C:\Program Files\meandriswrite` for all users.
Then choose **Always** (or tick **Always use this app**).

Note: Windows remembers this choice even after you uninstall Meandris Write. `.md` files then
ask which app to use; just pick another one.

**macOS.** Select a `.md` file in Finder → **File → Get Info** → **Open with:** choose
Meandris Write → **Change All…**

**Linux.** On first launch Meandris Write asks whether to **open `.md` files with it by
default** (and, for the AppImage, whether to **add itself to the app menu**; the .deb and .rpm do
that automatically). You can change both later under **Tools → Desktop Integration**. When you
turn it off, the program that opened `.md` files before takes over again. You can also use your
desktop's own settings: right-click a `.md` file → **Open With**.

### Where is Meandris Write in the app menu? (Linux)

Under **Utilities** (sometimes called **Accessories**). You can also open the app menu and
type `meandris` to search for it.

With the AppImage, the menu entry only exists if you chose **Add to the app menu** on first
launch. You can turn it on later under **Tools → Desktop Integration**.

### The AppImage doesn't start when I double-click it

- Make sure the file is allowed to run: right-click it → **Properties** → **Permissions** →
  tick **Allow executing file as program** (the wording differs a little per desktop). Or in a
  terminal: `chmod +x meandriswrite-*.AppImage`.
- On Ubuntu, AppImages need FUSE 2: `sudo apt install libfuse2` (Ubuntu 22.04) or
  `sudo apt install libfuse2t64` (Ubuntu 24.04 and newer).

### The software center keeps "Loading…" when I open the .deb or .rpm

Some software centers (KDE Discover in particular) don't handle downloaded package files well.
Install it from a terminal instead, in the folder where you downloaded it:

- Ubuntu, Debian, Linux Mint: `sudo apt install ./meandriswrite_*_amd64.deb`
- Fedora: `sudo dnf install ./meandriswrite-*.x86_64.rpm`

## Uninstalling

- **Windows:** **Settings → Apps**, find Meandris Write → **Uninstall**. Meandris Write keeps no
  settings on Windows, so nothing is left behind (except a default-app choice you made yourself,
  see above).
- **macOS:** drag Meandris Write from **Applications** to the Trash. It keeps no settings on macOS.
- **AppImage:** open Meandris Write, go to **Tools → Desktop Integration**, untick **Add to the
  app menu** and click **Apply**. This removes the menu entry and the icon, and gives `.md` files
  back to the program that opened them before. Then delete the AppImage file.
  If you already deleted the AppImage, delete `~/.local/share/applications/meandriswrite.desktop`
  and `~/.local/share/icons/hicolor/256x256/apps/meandriswrite.png` to remove the menu entry.
- **.deb** (Ubuntu, Debian, Linux Mint): remove it in your software center, or
  `sudo apt remove meandriswrite`.
- **.rpm** (Fedora): remove it in your software center, or `sudo dnf remove meandriswrite`.

On Linux, Meandris Write keeps one small settings file, `~/.config/meandriswrite/meandriswrite.conf`.
It only remembers your answer to the first-launch question. Uninstalling leaves it in place;
delete the `~/.config/meandriswrite` folder to remove it, or to see the first-launch question again.

## Build from source

Meandris Write needs a C++17 compiler, CMake and Qt 6.2 or newer (Widgets and PrintSupport).

```sh
# Linux (Debian/Ubuntu)
sudo apt install cmake g++ qt6-base-dev qt6-base-dev-tools qt6-qpa-plugins
# Linux (Fedora)
sudo dnf install gcc-c++ cmake qt6-qtbase-devel
```

On **macOS** (Apple Silicon), install CMake with [Homebrew](https://brew.sh) and Qt 6.8.3 into
`~/Qt` using [aqtinstall](https://github.com/miurahr/aqtinstall):

```sh
brew install cmake aqtinstall
aqt install-qt mac desktop 6.8.3 clang_64 -O ~/Qt
```

On **Windows**, install Qt with the MinGW compiler, CMake and Ninja into `C:\Qt` using
[aqtinstall](https://github.com/miurahr/aqtinstall), and build from **Git Bash**:

```sh
py -m pip install aqtinstall
py -m aqt install-qt  windows desktop 6.8.3 win64_mingw -O C:/Qt
py -m aqt install-tool windows desktop tools_mingw1310 qt.tools.win64_mingw1310 -O C:/Qt
py -m aqt install-tool windows desktop tools_cmake -O C:/Qt
py -m aqt install-tool windows desktop tools_ninja -O C:/Qt
```

Then, on any system:

```sh
./build.sh            # release build in build/
./build.sh test       # unit tests
packaging/release-linux.sh     # Linux: AppImage, .deb and .rpm in dist/ (needs podman)
packaging/package-macos.sh     # macOS: .dmg in dist/
packaging/package-windows.sh   # Windows: setup.exe in dist/ (needs Inno Setup 6)
```

## Feedback

Found a bug, or have an idea? Bug reports and ideas are welcome on the
[Issues page](https://github.com/onckelinx/meandriswrite/issues).

## License

© 2026 Rik Onckelinx - meandris.ai. Meandris Write is free software under the
[GNU General Public License](LICENSE), version 3 or (at your option) any later version.
The Windows, macOS and AppImage downloads include the Qt libraries, which are used under the
LGPL v3; see [packaging/THIRD-PARTY.txt](packaging/THIRD-PARTY.txt).
