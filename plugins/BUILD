load(":plugins.bzl", "SORBET_PLUGINS")

cc_library(
    name = "api",
    hdrs = ["Plugin.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//ast",
        "//ast/treemap",
        "//core",
    ],
)

# Implements the semantic extension slot (in place of //main/pipeline/semantic_extension:none) and links in every
# plugin listed in plugins.bzl.
cc_library(
    name = "host",
    srcs = ["PluginHost.cc"],
    linkstatic = select({
        "//tools/config:linkshared": 0,
        "//conditions:default": 1,
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":api",
        "//common",
        "//main/pipeline/semantic_extension:interface",
        "@cxxopts",
    ] + ["@%s//:plugin" % name for name in SORBET_PLUGINS],
)
