#!/bin/bash
# Packages a Sorbet binary (SORBET_BIN, default bazel-bin/main/sorbet) as the sorbet-static gem for one platform, plus
# the matching sorbet and sorbet-static-and-runtime gems.
#
#   docspring/build-gems.sh <platform> <version>    # e.g. universal-darwin 0.6.13583.1
#
# Writes dist/sorbet-static-<version>-<platform>.gem, dist/sorbet-<version>.gem and
# dist/sorbet-static-and-runtime-<version>.gem. The sorbet gem also carries each plugin's RBI generator
# (lib/sorbet-plugins/<plugin>/generate_rbi.rb, from SORBET_PLUGINS_DIR, default ../sorbet-plugins), so an app gets the
# generators that match the plugins in its binary. Tapioca and Spoom depend on sorbet-static-and-runtime, which pins
# sorbet and sorbet-runtime exactly, so we need our own: it pins our sorbet and upstream's unchanged sorbet-runtime.

set -euo pipefail

platform="${1:?usage: docspring/build-gems.sh <platform> <version>}"
version="${2:?usage: docspring/build-gems.sh <platform> <version>}"

cd "$(dirname "${BASH_SOURCE[0]}")/.."
plugins_dir="$(cd "${SORBET_PLUGINS_DIR:-../sorbet-plugins}" && pwd)"
base="$(cat docspring/BASE_VERSION)"
[[ "$version" == "$base".* ]] || { echo "Version $version doesn't start with BASE_VERSION $base" >&2; exit 1; }
dist="$PWD/dist"
mkdir -p "$dist"

rm -rf gems/sorbet-static/libexec
mkdir gems/sorbet-static/libexec
cp "${SORBET_BIN:-bazel-bin/main/sorbet}" gems/sorbet-static/libexec/sorbet

source_uri="https://github.com/DocSpring/sorbet/tree/docspring"
(
  cd gems/sorbet-static
  sed -e "s/0\.0\.0/${version}/" -e "s/Gem::Platform::CURRENT/'${platform}'/" -e "s|https://github.com/sorbet/sorbet|${source_uri}|" \
    sorbet-static.gemspec > docspring.gemspec
  gem build docspring.gemspec --output "$dist/sorbet-static-${version}-${platform}.gem"
  rm docspring.gemspec
)
(
  build="$(mktemp -d)"
  trap 'rm -rf "$build"' EXIT
  cp -R gems/sorbet/. "$build"
  generators=("$plugins_dir"/*/generate_rbi.rb)
  [[ -f "${generators[0]}" ]] || { echo "No generators in $plugins_dir" >&2; exit 1; }
  for generator in "${generators[@]}"; do
    plugin="$(basename "$(dirname "$generator")")"
    mkdir -p "$build/lib/sorbet-plugins/$plugin"
    cp "$generator" "$build/lib/sorbet-plugins/$plugin/"
  done
  cd "$build"
  sed -e "s/0\.0\.0/${version}/g" -e "s|https://github.com/sorbet/sorbet|${source_uri}|" sorbet.gemspec > docspring.gemspec
  gem build docspring.gemspec --output "$dist/sorbet-${version}.gem"
)
(
  cd gems/sorbet-static-and-runtime
  sed -e "s/'sorbet-runtime', '0\.0\.0'/'sorbet-runtime', '${base}'/" -e "s/0\.0\.0/${version}/g" \
    -e "s|https://github.com/sorbet/sorbet|${source_uri}|" sorbet-static-and-runtime.gemspec > docspring.gemspec
  gem build docspring.gemspec --output "$dist/sorbet-static-and-runtime-${version}.gem"
  rm docspring.gemspec
)
ls -l "$dist"
