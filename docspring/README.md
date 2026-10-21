# DocSpring's Sorbet build

This branch (`docspring`) is upstream Sorbet plus a small plugin core, built with the rewriter plugins from
[DocSpring/sorbet-plugins](https://github.com/DocSpring/sorbet-plugins). The plugins teach Sorbet the DSLs that
otherwise leave blocks untyped: RSpec (spec types, loops, shared examples), Capybara's feature DSL, rswag, FactoryBot
and ActiveAdmin.

## What's changed from upstream

- `plugins/`: the plugin API (`Plugin.h`), and `PluginHost.cc`, which fills Sorbet's existing link-time
  `SemanticExtensionProvider` slot (stock Sorbet links an empty one) and adds `--enable-plugin=<name>`.
  `plugins.bzl` lists the plugin repositories to compile in.
- `main/BUILD` and `WORKSPACE`: link `//plugins:host` and register the plugin repositories.
- `core/GlobalState.cc`: `copyForIndexThread` copies the semantic extensions. Without it the rewriter's
  semantic-extension hook never runs for files indexed on worker threads. Upstream merged the fix on 2026-10-21
  ([sorbet/sorbet#10804](https://github.com/sorbet/sorbet/pull/10804), `94623ce8a`), so rebasing onto a base that
  includes it drops this commit.
- `tools/buildstamp/get_workspace_status`: `SORBET_VERSION_COMMIT_COUNT` overrides the commit count in the version, so
  this build reports the upstream version it's based on.
- `docspring/` and `.github/workflows/docspring-release.yml`: the release build.

Plugins only run when enabled (`--enable-plugin=rspec`, or in `sorbet/config`), so without flags this build behaves
like upstream.

## Semantic extensions aren't a supported API

Sorbet added the `SemanticExtensionProvider` slot in 2019
([sorbet/sorbet#1485](https://github.com/sorbet/sorbet/pull/1485)) for the Sorbet Compiler, which upstream removed in
2024 ([sorbet/sorbet#7849](https://github.com/sorbet/sorbet/pull/7849)). The slot was never documented for others to
build on, and the maintainers have said it might be deleted. If it is, this branch re-adds it: the plugins only see
`plugins/Plugin.h`, so the change is limited to the rewriter's call, `GlobalState` and `PluginHost.cc`. When moving to
a new upstream release, check that `rewriter/rewriter.cc` still calls each extension's `run` on every class body (the
plugin tests fail if it doesn't).

## Building locally

The plugin repositories are expected next to this checkout (`../sorbet-plugins`):

```sh
git clone git@github.com:DocSpring/sorbet-plugins.git ../sorbet-plugins
./bazel build //main:sorbet -c opt
ruby ../sorbet-plugins/test/run_tests.rb bazel-bin/main/sorbet
```

## Releases

`docspring/BASE_VERSION` is the upstream release this branch is based on; `docspring/PLUGINS_REF` is the
sorbet-plugins commit a release builds with. Gems are versioned `<BASE_VERSION>.<n>` (`0.6.13583.1`), so they never
collide with an upstream version.

Pushing a tag `docspring-<BASE_VERSION>.<n>` builds `sorbet-static` for universal-darwin (arm64 plus an x86_64
cross-build, as upstream does), x86_64-linux and aarch64-linux, plus the matching `sorbet` and
`sorbet-static-and-runtime` gems, runs the plugin tests, benchmarks against upstream, and attaches the gems to a
GitHub release. Then run `gh workflow run publish.yml --repo DocSpring/gems`, which adds
them to the gem source at https://docspring.github.io/gems/ (see [DocSpring/gems](https://github.com/DocSpring/gems)).

The `sorbet` gem also carries each plugin's RBI generator, from the same sorbet-plugins commit as the binary, at
`lib/sorbet-plugins/<plugin>/generate_rbi.rb`. An app runs them from the installed gem
(`Gem::Specification.find_by_name('sorbet').gem_dir`), so the RBIs always match the plugins that read them.

## Moving to a new upstream release

```sh
git fetch https://github.com/sorbet/sorbet.git master
git rebase --onto <upstream commit for the new version> <old base commit> docspring
```

Then update `docspring/BASE_VERSION`. Sorbet's version is `0.6.<commit count>`, so the upstream commit for
`0.6.N` is the one where `git rev-list --count <commit>` is N.
