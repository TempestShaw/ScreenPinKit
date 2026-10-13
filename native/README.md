# ScreenPinKit Native (C++ migration)

The native implementation lives beside the Python application during migration. It uses C++17 and Qt Core/Gui/Widgets only. There is no Python runtime, browser engine, OpenCV, OCR model, recurring repaint timer or mouse polling thread in the native executable.

## Implemented

- One desktop snapshot per display on capture activation; an independent overlay uses that display's geometry and device pixel ratio.
- Free selection, moving a selection, all eight resize handles, physical-pixel magnifier, RGB/HEX and copying a color. Arrow keys step the sample by one physical pixel on Retina displays.
- Copy the selected snapshot directly, or create an always-on-top pin. Multiple pins can coexist, move and zoom. Pins remain visible when the application is inactive on macOS.
- A compact floating toolbar for capture selections and pins: pen, arrow, rectangle, text, color, three stroke widths, undo/redo, clipboard export, PNG export and OCR. Capture selections can be annotated before copying/saving/pinning. Annotations retain original image coordinates when the pin zooms; toolbar controls are never exported into the image.
- A monochrome macOS menu-bar icon, short native menu entries with aligned shortcuts, and a compact help/control panel.
- Click-through pins; restore input for all pins from the tray or control panel.
- OS-delivered global shortcuts: Carbon `RegisterEventHotKey` and a side-button event tap on macOS; `RegisterHotKey` and a mouse hook on Windows. Registration failure is reported in the control panel.
- Window detection by default; Tab switches to Element detection and back. Hover previews a target, click locks it, and dragging creates a free selection. Tab clears the locked selection and resumes detection. All display overlays share the mode.
- Local OCR of the selected region or a pin (including annotations), using macOS Vision or Windows.Media.Ocr. Press O or choose the pin's OCR menu action; recognized lines are copied to the clipboard. No text or an error leaves the clipboard intact.
- OCR runs in a short-lived instance of the same executable, without a GUI, tray, hotkeys or screen capture. Image encoding and process waits stay off the GUI thread. Exiting the helper releases its model/cache memory; no OCR helper remains in the background. Cold-start latency is the tradeoff for bounded idle memory.

## Build and test

Install a C++17 compiler, CMake 3.24+ and Qt 6.8+ with Widgets and Test. On macOS, Homebrew's `qtbase` is sufficient; the full `qt` formula is unnecessary.

```sh
brew install qtbase
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase)"
cmake --build native/build --parallel
ctest --test-dir native/build --output-on-failure
QT_QPA_PLATFORM=offscreen native/build/screenpin_benchmark
```

For Windows, use the Qt MSVC 2022 x64 package and a matching compiler:

```powershell
cmake -S native -B native/build -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build native/build --config Release --parallel
ctest --test-dir native/build -C Release --output-on-failure
```

The workflow in `.github/workflows/native.yml` builds/tests/packages both platforms. A configured workflow is not evidence that its Windows job has run.

## Package and run

Use a **fresh install directory** for each package: deployment tools rewrite the installed binaries and their rpaths. Reinstalling over a previously deployed/signed directory can retain newer modified binaries.

```sh
cmake --install native/build --config Release --prefix "$PWD/native/build/package"-fresh
```

This copies the application and Qt runtime dependencies. On macOS the result is `ScreenPinKitNative.app`; on Windows it is `ScreenPinKitNative.exe` plus its DLLs/plugins.

For stable **local macOS development signing**, use the packaging script with a new output directory:

```sh
./native/package-macos.sh native/build native/build/package-signed-1
```

It installs dependencies, signs nested libraries/frameworks before the app, verifies the result, and creates an arm64 or x86_64 ZIP. The first run creates one self-signed, code-signing-only certificate valid for ten years. Its private key is imported as non-extractable into the user's default keychain, with access for `/usr/bin/codesign`. If macOS asks to access the key, enter the keychain password **locally** and choose Always Allow. Temporary key/PKCS12 files are removed; only the public certificate and original keychain path remain in `~/Library/Application Support/ScreenPinKit/signing`, outside this repository. The temporary PKCS12's fixed import passphrase is only a format-compatibility value; temporary-directory permissions protect its contents. No system trust settings are changed.

Subsequent runs reuse that exact certificate. The app's designated requirement combines `io.screenpinkit.native` with the certificate hash, rather than a changing executable hash. Missing saved identities cause an error instead of silently creating a replacement. Keep the keychain identity and public certificate: losing/replacing the identity requires new privacy grants. To sign an already deployed bundle, run `./native/sign-macos-local.sh <bundle>` before creating its ZIP. Do not change bundle contents after signing.

Migrating from the old ad-hoc signature needs one new Screen Recording/Accessibility grant. A stable signature removes the rebuild-induced identity change; it does not bypass privacy checks or macOS's own periodic permission policies. Launch the signed deployed app at a consistent path; launching the raw CMake build or an ad-hoc CI artifact uses a different identity. Public distribution still requires Apple Developer ID signing and notarization; this local certificate is for development on your Mac.

Windows system OCR [requires package identity](https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/winrt-api-desktop-app-support). Build the MSIX after installing runtime dependencies:

```powershell
./native/package-windows.ps1 -PackageDirectory native/build/package-fresh -Output native/build/ScreenPinKitNative-x64.msix
```

The script produces a validated but unsigned x64 MSIX and adds the manifest's logo assets to the runtime folder. Distribution requires signing with a trusted certificate matching the manifest publisher. For local development, enable Windows Developer Mode, then run `Add-AppxPackage -Register <absolute-runtime-folder>/AppxManifest.xml` and launch **ScreenPinKit Native from Start** to obtain package identity. Running the loose EXE does not qualify for OCR; it reports this rather than silently failing. Windows OCR uses installed user-profile language packs; add Chinese/English OCR languages in Windows Settings as needed. Signing, registration and OCR runtime still need Windows validation.

Preferences records a global keyboard chord or mouse side-button gesture for capture, paste and restoring the last image. Click a field, then press Mouse4, Mouse5, or hold both together; release all side buttons to finish. Control/Command/Alt/Option/Shift can be held with side buttons. Global mouse actions also complete on release, so a two-button chord takes priority over a single-button prefix. An unfinished prefix is replayed as an ordinary side click. Left/right/middle clicks are not bindings. Mouse monitoring is installed only when a mouse binding is active and is suspended while recording. On macOS, applying mouse bindings requires the app's Accessibility permission. If mouse software remaps side buttons into keyboard shortcuts, record the emitted keyboard chord instead.

Clear a field to disable that action. Letters, digits, Space and F1–F20 are supported; letters/digits/Space require Control/Command/Alt/Option. Duplicate or occupied shortcuts are rejected and the previous registration is restored. Successful changes are persisted and update the panel and tray menu. Existing keyboard settings remain compatible.

Recovery saves **one cropped image** in `QStandardPaths::AppLocalDataLocation/last-image.png`, after capture/paste and when a pin closes. It includes rendered annotations, original pixel resolution/DPR, position and zoom. A small preview is encoded in PNG metadata so opening the panel only decodes the thumbnail. Restore works without the clipboard and across restarts; annotations are flattened and cannot be independently edited after restoration. The saved position is clamped to a connected display. Preferences can clear the file or turn recovery off (also clears it). Encoding runs in one temporary worker, with only the newest pending image retained; there is no idle timer, persistent full-screen snapshot, network upload or unbounded history.

The control panel can be closed or hidden to leave the app in the tray/menu bar. `--background` starts without the panel; image paths passed on the command line open as pins. macOS file-open events also open pins.

macOS screen capture needs the app's own Screen Recording permission, requested on the first capture. Grant it in System Settings if necessary and restart the app when macOS requires it. Cua Driver's capture permission does not grant permission to this app.

macOS Element mode additionally needs Accessibility permission for this app; its first Tab switch requests it. After granting access, switch back to Window and then Element to refresh permission. AX/UI Automation queries run off the GUI thread against the underlying window, with one outstanding request per overlay and a one-shot coalescing delay. Unavailable elements explicitly fall back to the window. Apps that do not expose controls through accessibility cannot offer precise element detection. Windows cannot inspect elevated/protected apps from this ordinary unelevated process.

| Operation | Input |
| --- | --- |
| Capture / paste / restore last image | Ctrl+Shift+2 / Ctrl+Shift+3 / Ctrl+Shift+4; customize in Preferences |
| Select / move / resize | Drag outside / inside / along selection edges |
| Detect window / element | Window by default; Tab toggles; click locks the detected region |
| OCR selection / pin | O; pin right-click → OCR → copy text |
| Pin selection / copy selection | Enter or double-click / Ctrl+C on Windows, Cmd+C on Mac |
| Copy color / switch HEX-RGB | C / Shift |
| Sample adjacent physical pixel | Arrow keys |
| Move / zoom a pin | Drag / wheel |
| Annotate selection / pin | Floating toolbar; M/P/A/R/T select move/pen/arrow/rectangle/text |
| Enter text on the image | T, then click; Enter finishes, Shift+Enter adds a line, Esc cancels the text |
| Hide / show pin tools | E; pin right-click → Annotate |
| Undo / redo / copy / save | Platform-standard Ctrl or Cmd shortcuts |
| Original zoom / close pin | 0 / Escape or double-click in Move mode |
| Restore click-through pins | Tray menu or control panel |

## Performance and migration limits

`CaptureOverlay::paintEvent` draws an immutable snapshot through Qt's dirty-region clip. Mouse motion invalidates the old/new decorations, changed selection mask and border. It allocates no full-screen copy. Cropping/export allocate only when requested. Closing a capture session releases the full-screen frames after Qt processes deferred deletion. Idle pins have no repaint timer.

Element detection keeps the last valid rectangle while the pointer remains inside it and a newer query is pending. Fresh results can still select child controls; results from an earlier detection mode or another window are discarded. This prevents the selection mask from repeatedly expanding to the whole window between asynchronous hit-tests.

The benchmark measures **synthetic 4K offscreen raster paint time**, not OS capture latency, input latency, compositor presentation, GPU throughput or achieved display FPS. Actual 144/165 Hz behavior needs a supported display and end-to-end measurement.

This is a working core migration, not complete Python feature parity. Plugin marketplace, mosaic/blur/eraser, scrolling capture, multiple-image history and full annotation editing are not migrated. OCR currently copies whole recognized lines; selectable text overlays are not implemented. A selection currently stays on one display; spanning displays and mixed-DPI compositing require a shared capture session before parity. RGB/HEX report the captured image's pixel values; explicit HDR/ICC conversion is not implemented.

The local Homebrew Qt 6.11.2 build is Apple Silicon and requires macOS 14+. Intel/Universal packages require matching Qt binaries and another build. Windows native code must also be built and exercised on Windows before release. Qt Widgets raster painting remains the current renderer; a GPU renderer should only follow evidence of a remaining bottleneck.

ScreenPinKit's application source/assets retain the root MIT license. Qt is dynamically linked; distribution must include the applicable Qt and bundled third-party license notices. See [Qt licensing](https://doc.qt.io/qt-6/licensing.html) and the chosen Qt package's notices.

Public distribution prerequisites and verification gates: [RELEASE.md](RELEASE.md). The local package script uses the existing development certificate; `release-macos.sh` requires Developer ID and accepted notarization, with no development fallback.
