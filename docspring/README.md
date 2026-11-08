# DocSpring's Sorbet build

This branch (`docspring`) is upstream Sorbet plus a small plugin core, built with the plugins from
[DocSpring/sorbet-plugins](https://github.com/DocSpring/sorbet-plugins). The plugins teach Sorbet the DSLs that
otherwise leave blocks untyped: RSpec (spec types, loops, shared examples), Capybara's feature DSL, rswag, FactoryBot
and ActiveAdmin. One more, `erb_views`, type-checks Rails' ERB templates: each template is checked at `typed: strict`
against the controllers and mailers that render it, with its instance variables' types inferred from their actions,
and each partial's locals typed by a `sig` comment. And `rails` writes the methods Rails and Sidekiq define from an
app's own code (a job's `perform_async`, typed from its `perform`), in place of Tapioca's DSL RBIs for them.

Our other Sorbet work:

- [DocSpring/sorbet-plugins](https://github.com/DocSpring/sorbet-plugins): the plugins, their RBI generators and
  their tests, and what we learned writing them.
- [DocSpring/gems](https://github.com/DocSpring/gems): the gem source (https://docspring.github.io/gems/) that serves
  this build's releases.
- [sorbet/sorbet#10804](https://github.com/sorbet/sorbet/pull/10804): our fix for semantic extensions on Sorbet's
  indexing threads, merged upstream.

## What's changed from upstream

- `plugins/`: the plugin API (`Plugin.h`), and `PluginHost.cc`, which fills Sorbet's existing link-time
  `SemanticExtensionProvider` slot (stock Sorbet links an empty one) and adds `--enable-plugin=<name>`.
  `plugins.bzl` lists the plugin repositories to compile in.
- `main/BUILD` and `WORKSPACE`: link `//plugins:host` and register the plugin repositories.
- `main/pipeline/pipeline.cc`, `parser/` and `SemanticExtension.h`: two more hooks for plugins.
  `transformSource` gives Sorbet the Ruby to parse for a file in another language (the erb_views plugin's templates),
  with as many bytes as the file, so error messages, autocorrects and the language server's edits use the file's own
  text at the same offsets. `strictLevelWithoutSigil` gives such a file's strictness when it has no `typed:` sigil.
  Sorbet still reads only the extensions `--allowed-extension` lists.
- `plugins/Plugin.h`: `PluginError` (3590), the error a plugin reports when it meets code it can't type exactly.
- `namer/namer.cc` and `core/Symbols.cc`: a positional parameter that a plugin synthesized with a zero-length
  location keeps its name. Sorbet shows a positional parameter by its source text, which such a parameter doesn't
  have, so errors about a generated method said "Expected `String` for argument ``" or named it `<arg>1`.
- `resolver/resolver.cc` and `main/pipeline/pipeline.cc`: several `sig`s that a rewriter or plugin wrote before one
  method are overloads, as they would be in an RBI file (upstream allows overloads only in RBI files, and in a `.rb`
  file keeps the last `sig`), and the body of such a method isn't type-checked (it only raises). The `rails` plugin
  writes Tapioca's overloaded relation methods (`create`, `find`, `first`) into each model.
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
`lib/sorbet-plugins/<plugin>/generate_rbi.rb` (with the plugin's `generator/` directory, for a generator in several
files). An app runs them from the installed gem (`Gem::Specification.find_by_name('sorbet').gem_dir`), so the RBIs
always match the plugins that read them. The erb_views generator also runs the installed binary
(`sorbet-static`'s `libexec/sorbet`): it reads the types of the values that reach each template from Sorbet. Its
runtime check (`lib/sorbet-plugins/erb_views/generator/runtime_check.rb`), which an app's test environment requires,
checks each render against the RBIs the generator wrote.

## Moving to a new upstream release

```sh
git fetch https://github.com/sorbet/sorbet.git master
git rebase --onto <upstream commit for the new version> <old base commit> docspring
```

Then update `docspring/BASE_VERSION`. Sorbet's version is `0.6.<commit count>`, so the upstream commit for
`0.6.N` is the one where `git rev-list --count <commit>` is N.
