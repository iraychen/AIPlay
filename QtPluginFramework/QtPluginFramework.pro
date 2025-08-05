TEMPLATE = subdirs

SUBDIRS += \
    PluginCore \
    HostApplication \
    Plugins

HostApplication.depends = PluginCore
Plugins.depends = PluginCore

# Create build directories
system(mkdir -p build/debug)
system(mkdir -p build/release)
system(mkdir -p build/debug/plugins)
system(mkdir -p build/release/plugins)
system(mkdir -p build/debug/metadata)
system(mkdir -p build/release/metadata)
system(mkdir -p build/debug/config)
system(mkdir -p build/release/config)
system(mkdir -p build/debug/logs)
system(mkdir -p build/release/logs)