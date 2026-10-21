#include "absl/algorithm/container.h"
#include "absl/strings/str_join.h"
#include "common/common.h"
#include "common/EarlyReturnWithCode.h"
#include "main/pipeline/semantic_extension/SemanticExtension.h"
#include "plugins/Plugin.h"
#include <cxxopts.hpp>

using namespace std;

namespace sorbet::plugins {

namespace {

vector<unique_ptr<Plugin>> &registry() {
    static vector<unique_ptr<Plugin>> plugins;
    return plugins;
}

vector<string> registeredNames() {
    vector<string> names;
    for (auto &plugin : registry()) {
        names.emplace_back(plugin->name());
    }
    return names;
}

// Runs the enabled plugins from the rewriter's semantic extension hook. The other hooks are unused.
class PluginExtension final : public pipeline::semantic_extension::SemanticExtension {
    vector<const Plugin *> enabled;

public:
    explicit PluginExtension(vector<const Plugin *> enabled) : enabled(move(enabled)) {}

    void finishTypecheckFile(const core::GlobalState &, const core::FileRef &) const override {}
    void finishTypecheck(const core::GlobalState &) const override {}
    void typecheck(const core::GlobalState &, core::FileRef, cfg::CFG &) const override {}

    void run(core::MutableContext &ctx, ast::ClassDef *classDef) const override {
        for (auto plugin : enabled) {
            plugin->rewriteClassBody(ctx, *classDef);
        }
    }

    unique_ptr<SemanticExtension> deepCopy(const core::GlobalState &, core::GlobalState &) override {
        return make_unique<PluginExtension>(enabled);
    }

    void merge(const core::GlobalState &, core::GlobalState &, core::NameSubstitution &) override {}
};

class PluginExtensionProvider final : public pipeline::semantic_extension::SemanticExtensionProvider {
public:
    void injectOptions(cxxopts::Options &options) const override {
        auto names = registeredNames();
        auto help = names.empty() ? string("Enable a rewriter plugin (none are compiled into this build)")
                                  : fmt::format("Enable a rewriter plugin compiled into this build: {}",
                                                absl::StrJoin(names, ", "));
        options.add_options("plugins")("enable-plugin", help, cxxopts::value<vector<string>>(), "<name>");
    }

    unique_ptr<pipeline::semantic_extension::SemanticExtension>
    readOptions(cxxopts::ParseResult &options) const override {
        if (options.count("enable-plugin") == 0) {
            return nullptr;
        }

        vector<const Plugin *> enabled;
        for (auto &requested : options["enable-plugin"].as<vector<string>>()) {
            auto found = absl::c_find_if(registry(), [&](auto &plugin) { return plugin->name() == requested; });
            if (found == registry().end()) {
                fmt::print(stderr, "Unknown plugin `{}`. Plugins in this build: {}\n", requested,
                           registry().empty() ? "none" : absl::StrJoin(registeredNames(), ", "));
                throw EarlyReturnWithCode(1);
            }
            if (!absl::c_linear_search(enabled, found->get())) {
                enabled.emplace_back(found->get());
            }
        }
        return make_unique<PluginExtension>(move(enabled));
    }

    unique_ptr<pipeline::semantic_extension::SemanticExtension> defaultInstance() const override {
        return make_unique<PluginExtension>(vector<const Plugin *>{});
    }
};

} // namespace

bool registerPlugin(unique_ptr<Plugin> plugin) {
    registry().emplace_back(move(plugin));
    return true;
}

} // namespace sorbet::plugins

namespace sorbet::pipeline::semantic_extension {

vector<SemanticExtensionProvider *> SemanticExtensionProvider::getProviders() {
    static plugins::PluginExtensionProvider provider;
    return {&provider};
}

} // namespace sorbet::pipeline::semantic_extension
