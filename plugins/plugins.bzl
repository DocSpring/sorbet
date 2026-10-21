"""The rewriter plugins compiled into this Sorbet build.

Each plugin is its own Bazel repository with a `:plugin` cc_library (alwayslink = True) that depends on
`@//plugins:api`. Use `path` for a local checkout, or `remote` + `commit` for a git repository.
"""

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

SORBET_PLUGINS = {
    "sorbet_plugin_active_admin": {"path": "../sorbet-plugins/active_admin"},
    "sorbet_plugin_capybara": {"path": "../sorbet-plugins/capybara"},
    "sorbet_plugin_factory_bot": {"path": "../sorbet-plugins/factory_bot"},
    "sorbet_plugin_rswag": {"path": "../sorbet-plugins/rswag"},
    "sorbet_plugin_rspec": {"path": "../sorbet-plugins/rspec"},
}

def register_sorbet_plugins():
    for name, spec in SORBET_PLUGINS.items():
        if "path" in spec:
            native.local_repository(name = name, path = spec["path"])
        else:
            git_repository(name = name, remote = spec["remote"], commit = spec["commit"])
