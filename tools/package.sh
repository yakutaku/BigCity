#!/bin/bash
# Builds, signs and packages BigCity.vst3 into a Developer ID–signed installer (PRD §10).
#
#   tools/package.sh              build, sign and package; stops before notarizing
#   tools/package.sh --notarize   also notarizes the .pkg and staples the ticket
#
# Notarizing uses the keychain profile "bigcity-notary", already saved on this Mac.
# The version comes from project() in CMakeLists.txt, so the plugin and the installer always agree.
set -euo pipefail

readonly APPLICATION_IDENTITY="Developer ID Application: Jonathan Day (Q23N9LZFM4)"
readonly INSTALLER_IDENTITY="Developer ID Installer: Jonathan Day (Q23N9LZFM4)"
readonly NOTARY_PROFILE="bigcity-notary"
readonly PACKAGE_IDENTIFIER="com.sunshoweraudio.bigcity.vst3"
readonly INSTALL_LOCATION="/Library/Audio/Plug-Ins/VST3"

readonly REPO="$(cd "$(dirname "$0")/.." && pwd)"
readonly BUILD_DIR="$REPO/build"
readonly BUILT_PLUGIN="$BUILD_DIR/BigCity_artefacts/Release/VST3/BigCity.vst3"
readonly DIST_DIR="$REPO/dist"
readonly STAGE_DIR="$DIST_DIR/stage"

notarize=false
case "${1:-}" in
    "") ;;
    --notarize) notarize=true ;;
    *) echo "usage: $0 [--notarize]" >&2; exit 2 ;;
esac

step() { printf '\n==> %s\n' "$*"; }
fail() { printf 'error: %s\n' "$*" >&2; exit 1; }

version="$(sed -nE 's/^project\(BigCity VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "$REPO/CMakeLists.txt")"
[[ -n "$version" ]] || fail "couldn't read the version from project() in CMakeLists.txt"
readonly PACKAGE="$DIST_DIR/BigCity-$version.pkg"

step "Building BigCity $version (Release, universal)"
cmake -S "$REPO" -B "$BUILD_DIR" -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build "$BUILD_DIR" --config Release --target BigCity_VST3

architectures="$(lipo -archs "$BUILT_PLUGIN/Contents/MacOS/BigCity")"
[[ "$architectures" == *arm64* && "$architectures" == *x86_64* ]] \
    || fail "the plugin isn't universal (architectures: $architectures)"

built_version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$BUILT_PLUGIN/Contents/Info.plist")"
[[ "$built_version" == "$version" ]] || fail "the built plugin says $built_version, expected $version"

step "Staging a clean copy"
rm -rf "$STAGE_DIR" "$PACKAGE"
mkdir -p "$STAGE_DIR"
ditto "$BUILT_PLUGIN" "$STAGE_DIR/BigCity.vst3"

step "Signing the plugin (Developer ID Application, hardened runtime, timestamp)"
codesign --force --options runtime --timestamp --sign "$APPLICATION_IDENTITY" "$STAGE_DIR/BigCity.vst3"
codesign --verify --strict --verbose=2 "$STAGE_DIR/BigCity.vst3"

step "Building the installer"
# Not relocatable: otherwise macOS installs over any copy with the same bundle ID it finds elsewhere,
# such as a developer build in ~/Library, instead of installing to $INSTALL_LOCATION.
component_plist="$DIST_DIR/component.plist"
pkgbuild --analyze --root "$STAGE_DIR" "$component_plist"
plutil -replace 0.BundleIsRelocatable -bool NO "$component_plist"

pkgbuild --root "$STAGE_DIR" \
         --component-plist "$component_plist" \
         --install-location "$INSTALL_LOCATION" \
         --identifier "$PACKAGE_IDENTIFIER" \
         --version "$version" \
         --sign "$INSTALLER_IDENTITY" \
         --timestamp \
         "$PACKAGE"
pkgutil --check-signature "$PACKAGE"

if [[ "$notarize" != true ]]; then
    step "Done: $PACKAGE (signed, not notarized). Run with --notarize to finish."
    exit 0
fi

step "Notarizing (this usually takes 5–30 minutes)"
submission="$(xcrun notarytool submit "$PACKAGE" --keychain-profile "$NOTARY_PROFILE" --wait --output-format json)"
echo "$submission"
submission_id="$(plutil -extract id raw - <<< "$submission")"
status="$(plutil -extract status raw - <<< "$submission")"

if [[ "$status" != "Accepted" ]]; then
    xcrun notarytool log "$submission_id" --keychain-profile "$NOTARY_PROFILE" || true
    fail "notarization finished as \"$status\" (submission $submission_id); the log is above"
fi

step "Stapling the ticket"
xcrun stapler staple "$PACKAGE"
xcrun stapler validate "$PACKAGE"
spctl --assess --type install --verbose=2 "$PACKAGE"

step "Done: $PACKAGE (signed, notarized and stapled)"
