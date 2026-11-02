#ifndef SORBET_PIPELINE_SEMANTIC_EXTENSION
#define SORBET_PIPELINE_SEMANTIC_EXTENSION
#include <cstdint>
#include <memory> // unique_ptr
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cxxopts {
class ParseResult;
class Options;
} // namespace cxxopts
namespace sorbet {
namespace core {
class GlobalState;
class NameSubstitution;
class MutableContext;
class FileRef;
enum class StrictLevel : uint8_t;
} // namespace core

namespace ast {
class MethodDef;
class ClassDef;
} // namespace ast

namespace cfg {
class CFG;
}

namespace pipeline::semantic_extension {
class SemanticExtension {
public:
    virtual void finishTypecheckFile(const core::GlobalState &, const core::FileRef &) const = 0;
    virtual void finishTypecheck(const core::GlobalState &) const = 0;
    virtual void typecheck(const core::GlobalState &, core::FileRef file, cfg::CFG &) const = 0;
    virtual void run(core::MutableContext &, ast::ClassDef *) const = 0;
    // DocSpring: called before Sorbet parses a file. Returns the Ruby to parse in place of the file's source, with
    // exactly as many bytes, or nullopt to parse the source itself. The file keeps its own source, which error
    // messages, autocorrects and LSP read at the same offsets.
    virtual std::optional<std::string> transformSource(std::string_view path, std::string_view source) const {
        return std::nullopt;
    }
    // DocSpring: the strictness of a file without a `# typed:` sigil, or nullopt for Sorbet's default (`false`).
    virtual std::optional<core::StrictLevel> strictLevelWithoutSigil(std::string_view path) const {
        return std::nullopt;
    }
    virtual ~SemanticExtension() = default;
    virtual std::unique_ptr<SemanticExtension> deepCopy(const core::GlobalState &from, core::GlobalState &to) = 0;
    virtual void merge(const core::GlobalState &from, core::GlobalState &to, core::NameSubstitution &subst) = 0;
};

class SemanticExtensionProvider {
public:
    virtual void injectOptions(cxxopts::Options &) const = 0;
    virtual std::unique_ptr<SemanticExtension> readOptions(cxxopts::ParseResult &) const = 0;
    virtual std::unique_ptr<SemanticExtension> defaultInstance() const = 0;
    static std::vector<SemanticExtensionProvider *> getProviders();
    virtual ~SemanticExtensionProvider() = default;
};
} // namespace pipeline::semantic_extension
} // namespace sorbet

#endif
