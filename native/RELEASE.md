# Native commercial release readiness

Status: **native-v0.1.0-preview.1 is the first public developer preview; commercial distribution gates remain open**. This is a developer readiness document. Customer instructions are in GETTING_STARTED.md; release notes and the website describe features, usage, downloads and necessary installation requirements.

## Implemented

- Capture, pinning, precise color sampling, local OCR, Window/Element selection and four focused annotation tools.
- A redesigned control panel with Workspace and Preferences, last-image thumbnail and native tray commands.
- A three-step first-run guide: capture/pin introduction, explicit macOS permission buttons with read-only status checks, and current/customizable shortcuts. Dismissal is remembered; Getting started reopens it. The guide is created on demand and has no polling or animation timer. Background and OCR-worker launches bypass it.
- A branded drag-to-Applications DMG with the approved mark, Retina background and fixed Finder layout. The public packaging script signs and notarizes both the application and DMG; current artifacts still use local development signing.
- A static launch frontend with an illustrated four-tool demo, a captioned 29-second recording of actual macOS capture/annotation/pinning/recovery, and first-use instructions. The website is public; preview downloads link to the owned GitHub release.
- Configurable global capture/paste/restore keys, persistent settings, disabling a binding, duplicate validation and rollback when registration fails.
- Mouse4/Mouse5 shortcuts, a two-side-button chord and optional keyboard modifiers; recording suspends global bindings. Release completes a gesture, with ordinary side-click replay for an incomplete chord. Mouse hooks are registered only when needed.
- Restore the latest cropped capture or pin, including flattened annotations, position, DPR and zoom, across restarts. One local file, bounded asynchronous encoding, and an explicit disable/clear control.
- macOS local development signing retains the existing identity. Windows source includes UI Automation and packaged Windows OCR, plus MSIX generation. Windows packaging carries app-local MSVC DLLs; unused network/touch plugins, shader compiler payloads and the VC installer are excluded for the raster Widgets application.
- Public packaging entry points: Developer ID + hardened runtime + notarization/stapling/Gatekeeper verification on macOS; optional timestamped certificate signing + SignTool verification for Windows MSIX.

## Release gates

| Gate | Current evidence / remaining work |
| --- | --- |
| Mouse shortcuts | Mouse binding persistence, two-button matching, ordinary-click fallback and recorder regression tests pass on macOS and Windows. Mac keyboard recorder/Apply interaction is verified with Cua Driver, including shifted digits. Physical side-button recording, Accessibility grant/registration and global mouse triggering still require interactive hardware acceptance on both platforms. |
| macOS build and regression tests | Release build and native regression tests pass on macOS 15.6.1, Qt 6.11.2 arm64. Cua Driver verified the packaged first-run guide, permission status, navigation and shortcut-customization link. It also inspected the actual branded DMG in Finder. Public signing/clean-machine acceptance remains pending. |
| Windows | Native MSVC 2022 / Qt 6.8.3 x64 build and MSIX packaging completed on the Windows host. 18 functional tests pass; the actual global-registration test is skipped because SSH runs in Session 0 (Windows error 1459, requires interactive window station). App-local DLL smoke passes with Qt removed from PATH. The unsigned MSIX is about 11.6 MB; installation, interactive shortcuts/capture/Element detection and packaged OCR remain unverified. |
| Public macOS signature | No valid Developer ID identity is installed on this host. Current certificate is local development only. Run `release-macos.sh` with the distribution identity and a configured notary Keychain profile; install the result on a clean Mac. |
| Public Windows signature/install | Configure the real publisher identity and matching CA-trusted/Store signing route. Verify signed MSIX installation, uninstall, upgrades and user-data preservation on a clean Windows machine. An unsigned MSIX is a build artifact. |
| Automatic updates | **Not implemented.** Dopamine currently uses Sparkle on macOS and Velopack on Windows, with signed feeds and install/download/replace/restart smoke tests. For this MSIX application, use Windows App Installer or Store updates so OCR package identity is preserved. Integrate signed Sparkle updates on macOS; test two actual versions, signature rejection and interrupted downloads. A release link is not an updater. |
| Distribution ownership | The native preview is published in TempestShaw/ScreenPinKit, a fork retaining upstream MIT attribution. Origin remains the upstream repository; the release remote points to the owned fork. Keep application/publisher IDs and certificate continuity across updates. |
| Dependency licensing | Root MIT attribution is included; Qt is dynamically linked and LGPL/GPL license texts are bundled. Audit the **actual shipped** Qt/plugins/ICU/HarfBuzz/FreeType/etc. notices and exact corresponding sources/replacement instructions. Resolve LGPL obligations or obtain a suitable commercial Qt license. Review Store restrictions for the chosen channel. |
| Privacy and terms | Ship a privacy statement for Screen Recording/Accessibility, local OCR, saved image, crash reports and updates. Decide retention defaults, deletion/uninstall expectations and support contact. No screenshot upload/telemetry currently exists. |
| Commercial model | Pricing, checkout, trial/entitlement rules, refunds and offline behavior are undecided; there is no payment or activation implementation. Do not invent a service until the sales model is chosen. |
| Performance qualification | Idle widgets have no repaint timer; recovery adds no idle polling. Actual 144/165 Hz presentation and cross-display capture latency are not measured. Prior post-capture compositor memory retention remains a release concern; profile on both OSes with a fixed capture/cancel/pin workload. |
| Product acceptance | Validate Windows packaged OCR with Chinese/English language packs, non-admin/elevated app detection, mixed DPI and multiple displays; Mac permission persistence, sleep/wake, display changes and both supported architectures. Current Mac package is arm64/macOS 14+; Intel is not built. Selection still cannot span displays. |
| Operational readiness | Add single-instance/reopen behavior, startup option if promised, accessibility/keyboard review and localization for target users. Clean-machine installation and signed update smoke evidence are required before paid launch. The preview is versioned against committed native source; development evidence does not establish commercial acceptance. |

## Reproducible packaging

Preview Mac:

```sh
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/qt
cmake --build native/build --parallel
ctest --test-dir native/build --output-on-failure
native/package-macos.sh native/build native/build/new-preview
python3 -m venv native/build/dmg-tools
native/build/dmg-tools/bin/python -m pip install dmgbuild==1.6.7
SCREENPIN_DMG_PYTHON="$PWD/native/build/dmg-tools/bin/python" \
  native/package-dmg.sh native/build/new-preview/ScreenPinKitNative.app native/build/new-preview/ScreenPinKit-macOS.dmg
```

Public Mac (no local-signing fallback):

Install `dmgbuild==1.6.7` in the packaging environment above; set `SCREENPIN_DMG_PYTHON` to that Python for this command too. It emits a notarized ZIP and a signed/notarized/stapled DMG, with SHA-256 checksums.

```sh
native/release-macos.sh native/build native/build/new-public \
  'Developer ID Application: Your Publisher (TEAMID)' your-notary-keychain-profile
```

Preview Windows (MSVC/Qt environment):

```powershell
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release
cmake --build native/build --config Release --parallel
ctest --test-dir native/build -C Release --output-on-failure
$package = Join-Path (Get-Location) "native/build/package"
cmake --install native/build --config Release --prefix "$package"
./native/package-windows.ps1 -PackageDirectory native/build/package -Output native/build/ScreenPinKitNative-x64.msix
```

Configure `-DSCREENPIN_WINDOWS_PUBLISHER="CN=Your Publisher"` with the exact certificate subject before packaging. Add `-CertificateThumbprint <trusted-publisher-certificate>` for signing. Its subject must match the publisher in the manifest. A self-signed test certificate does not satisfy public distribution trust. Do not bundle private certificates/keys in releases.

## Primary release references

- [Qt LGPL obligations](https://www.qt.io/development/open-source-lgpl-obligations): dynamic linking alone does not exhaust the obligations.
- [Apple Developer ID](https://developer.apple.com/developer-id/) and [notarization](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution).
- [Windows OCR requirements](https://learn.microsoft.com/en-us/uwp/api/windows.media.ocr): package identity is required by the supported desktop API.
- [Microsoft distribution paths](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/choose-distribution-path) and [MSIX signing](https://learn.microsoft.com/en-us/windows/msix/package/sign-msix-package-guide).
- [Dopamine release workflow](https://github.com/TempestShaw/Dopamine/blob/main/.github/workflows/build.yml), inspected locally for its signing, feeds and updater smoke-test requirements. Its earlier release is a preparation reference, not evidence for ScreenPinKit.
