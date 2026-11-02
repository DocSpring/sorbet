#ifndef SORBET_PLUGINS_PLUGIN_H
#define SORBET_PLUGINS_PLUGIN_H

#include "ast/Helpers.h"
#include "ast/ast.h"
#include "core/core.h"
#include <memory>
#include <optional>
#include <string>
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

    // Called before Sorbet parses a file, so a plugin can read a file that isn't Ruby (an ERB template) as Ruby.
    // Returns the Ruby to parse in place of the source, or nullopt to parse the source itself. The Ruby must have
    // exactly as many bytes as the source (Sorbet raises otherwise): the file keeps its own source, and error
    // messages, autocorrects and the language server read it at the locations in the parsed Ruby, so keep every byte
    // of code where it was. Like `rewriteClassBody`, it may only use what it's given. Sorbet only reads the file types
    // `--allowed-extension` lists (`.rb` and `.rbi` by default).
    virtual std::optional<std::string> transformSource(std::string_view path, std::string_view source) const {
        return std::nullopt;
    }

    // The strictness of a file without a `# typed:` sigil, for the files a plugin reads with `transformSource`, or
    // nullopt for Sorbet's default (`false`). Sorbet finds a sigil anywhere in a file's own source.
    virtual std::optional<core::StrictLevel> strictLevelWithoutSigil(std::string_view path) const {
        return std::nullopt;
    }
};

// Adds a plugin to the binary's registry. Plugins call this through SORBET_PLUGIN from a static initializer, so their
// Bazel `cc_library` needs `alwayslink = True`.
bool registerPlugin(std::unique_ptr<Plugin> plugin);

} // namespace sorbet::plugins

#define SORBET_PLUGIN(Type) \
    [[maybe_unused]] static const bool sorbet_plugin_registered_##Type = \
        ::sorbet::plugins::registerPlugin(std::make_unique<Type>())

#endif
