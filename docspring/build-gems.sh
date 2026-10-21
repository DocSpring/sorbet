#!/bin/bash
# Packages bazel-bin/main/sorbet as the sorbet-static gem for one platform, plus the matching sorbet gem.
#
#   docspring/build-gems.sh <platform> <version>    # e.g. arm64-darwin 0.6.13583.1
#
# Writes dist/sorbet-static-<version>-<platform>.gem and dist/sorbet-<version>.gem.

set -euo pipefail

platform="${1:?usage: docspring/build-gems.sh <platform> <version>}"
version="${2:?usage: docspring/build-gems.sh <platform> <version>}"

cd "$(dirname "${BASH_SOURCE[0]}")/.."
dist="$PWD/dist"
mkdir -p "$dist"

rm -rf gems/sorbet-static/libexec
mkdir gems/sorbet-static/libexec
cp bazel-bin/main/sorbet gems/sorbet-static/libexec/sorbet

source_uri="https://github.com/DocSpring/sorbet/tree/docspring"
(
  cd gems/sorbet-static
  sed -e "s/0\.0\.0/${version}/" -e "s/Gem::Platform::CURRENT/'${platform}'/" -e "s|https://github.com/sorbet/sorbet|${source_uri}|" \
    sorbet-static.gemspec > docspring.gemspec
  gem build docspring.gemspec --output "$dist/sorbet-static-${version}-${platform}.gem"
  rm docspring.gemspec
)
(
  cd gems/sorbet
  sed -e "s/0\.0\.0/${version}/g" -e "s|https://github.com/sorbet/sorbet|${source_uri}|" sorbet.gemspec > docspring.gemspec
  gem build docspring.gemspec --output "$dist/sorbet-${version}.gem"
  rm docspring.gemspec
)
ls -l "$dist"
