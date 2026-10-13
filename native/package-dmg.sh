#!/bin/bash
set -euo pipefail
if [[ $# != 2 ]]; then
    echo 'Usage: package-dmg.sh signed-app-bundle new-output.dmg' >&2
    exit 1
fi
script_dir=$(cd "$(dirname "$0")" && pwd)
app=$(cd "$1" && pwd)
output=$2
[[ ! -e "$output" ]] || { echo 'Output already exists.' >&2; exit 1; }
/usr/bin/codesign --verify --deep --strict "$app"
"${SCREENPIN_DMG_PYTHON:-python3}" -c 'import dmgbuild, sys; dmgbuild.build_dmg(sys.argv[1], "ScreenPinKit", settings_file=sys.argv[2], defines={"app": sys.argv[3], "background": sys.argv[4], "icon": sys.argv[5]})' \
    "$output" "$script_dir/dmg-settings.py" "$app" "$script_dir/assets/dmg/background@2x.png" "$script_dir/assets/app-icon.icns"
hdiutil verify "$output"
