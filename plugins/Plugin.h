#ifndef SORBET_PLUGINS_PLUGIN_H
#define SORBET_PLUGINS_PLUGIN_H

#include "ast/Helpers.h"
#include "ast/ast.h"
#include "core/core.h"
#include <memory>
#include <string_view>

namespace sorbet::plugins {

// A rewriter plugin, compiled into the Sorbet binary and turned on with `--enable-plugin=<name>`.
//
// Sorbet calls `rewriteClassBody` once for every class or module body in a file, including the file's top-level
// `<root>` class, innermost first. It runs after parsing and before Sorbet's own statement rewriters (Minitest/RSpec,
// `attr_reader`, `Struct.new`...), so a plugin can turn a DSL into code those rewriters already understand.
//
// Sorbet rewrites each file on its own (and again on every edit in the language server), so a plugin may only use
// the tree it is given: never other files, global state it keeps itself, or anything outside Sorbet. Knowledge that
// lives in other files (which class a factory builds, say) belongs in RBI files that the plugin's rewritten code
// refers to by name.
class Plugin {
public:
    virtual ~Plugin() = default;

    // The name used with `--enable-plugin=<name>`.
    virtual std::string_view name() const = 0;

    virtual void rewriteClassBody(core::MutableContext ctx, ast::ClassDef &classDef) const = 0;
};

// Adds a plugin to the binary's registry. Plugins call this through SORBET_PLUGIN from a static initializer, so their
// Bazel `cc_library` needs `alwayslink = True`.
bool registerPlugin(std::unique_ptr<Plugin> plugin);

} // namespace sorbet::plugins

#define SORBET_PLUGIN(Type) \
    [[maybe_unused]] static const bool sorbet_plugin_registered_##Type = \
        ::sorbet::plugins::registerPlugin(std::make_unique<Type>())

#endif
