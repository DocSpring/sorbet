#!/bin/bash
# Benchmarks the plugin build against the upstream release it's based on, on a public Rails app's RSpec suite with
# every spec forced to `typed: true`:
#
# - upstream sorbet-static <BASE_VERSION>
# - this build, no plugins (the plugin core's own cost)
# - this build, with the RSpec-side plugins
#
# each through the rewriter only, and through a full check.
#
#   docspring/benchmark.sh <dir with this build's sorbet-static gem>
#
# PLATFORM (default x86_64-linux) picks this build's gem; UPSTREAM_PLATFORM (default the same) the upstream one.
#
# Needs hyperfine. Writes a Markdown table to stdout, and to the job summary on GitHub Actions.

set -euo pipefail

gems_dir="$(cd "${1:?usage: docspring/benchmark.sh <gems dir>}" && pwd)"
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
base="$(cat "$here/BASE_VERSION")"
platform="${PLATFORM:-x86_64-linux}"
upstream_platform="${UPSTREAM_PLATFORM:-$platform}"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
cd "$work"

# A large public Rails app with an RSpec suite, at a fixed commit.
corpus_repo=https://github.com/mastodon/mastodon.git
corpus_ref=v4.3.0
git clone --quiet --depth 1 --branch "$corpus_ref" "$corpus_repo" corpus

gem fetch sorbet-static --version "$base" --platform "$upstream_platform" >/dev/null
mkdir upstream plugins
tar -xOf "sorbet-static-${base}-${upstream_platform}.gem" data.tar.gz | tar -xzf - -C upstream
tar -xOf "$gems_dir"/sorbet-static-*-"${platform}".gem data.tar.gz | tar -xzf - -C plugins

{
  echo 'true:'
  (cd corpus && find spec -name '*_spec.rb' | sort | sed 's|^|  - ./|')
} > typed_true.yml

upstream="$work/upstream/libexec/sorbet"
plugins="$work/plugins/libexec/sorbet"
common="--no-config --quiet --typed-override=$work/typed_true.yml --enable-experimental-rspec ."
enable="--enable-plugin=capybara --enable-plugin=rswag --enable-plugin=rspec --enable-plugin=factory_bot"

"$upstream" --version
"$plugins" --version
echo "Corpus: $corpus_repo $corpus_ref, $(grep -c '^  - ' typed_true.yml) spec files, $(getconf _NPROCESSORS_ONLN) CPUs"

cd corpus
results="$work/results.md"
hyperfine --warmup 2 --runs "${RUNS:-10}" --ignore-failure --export-markdown "$results" \
  -n "upstream, rewriter only" "$upstream $common --stop-after=rewriter" \
  -n "plugin build, no plugins, rewriter only" "$plugins $common --stop-after=rewriter" \
  -n "plugin build, plugins, rewriter only" "$plugins $common $enable --stop-after=rewriter" \
  -n "upstream, full check" "$upstream $common" \
  -n "plugin build, no plugins, full check" "$plugins $common" \
  -n "plugin build, plugins, full check" "$plugins $common $enable"

cat "$results"
if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
  {
    echo "### Plugin benchmark: $corpus_repo $corpus_ref specs at typed: true"
    cat "$results"
  } >> "$GITHUB_STEP_SUMMARY"
fi
