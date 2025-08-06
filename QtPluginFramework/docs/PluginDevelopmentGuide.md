# Enterprise Plugin Framework - Plugin Development Guide

## Introduction

This guide provides instructions for developing plugins for the Enterprise Plugin Framework. It covers the plugin interface, metadata format, development workflow, and best practices.

## Plugin Interface

All plugins must implement the `IPlugin` interface defined in `PluginCore/IPlugin.h`. This interface defines methods for plugin lifecycle management and functionality.

### Key Methods

#### Lifecycle Methods

- `initialize()`: Called when the plugin is first loaded. Use this method to set up internal state and resources.
- `activate()`: Called when the plugin should become active and start providing its functionality.
- `deactivate()`: Called when the plugin should become inactive and stop providing its functionality.
- `shutdown()`: Called before the plugin is unloaded. Use this method to clean up resources.

#### Metadata Methods

- `getPluginId()`: Returns the unique identifier for the plugin.
- `getPluginName()`: Returns the human-readable name of the plugin.
- `getPluginVersion()`: Returns the version of the plugin.
- `getPluginVendor()`: Returns the name of the organization that created the plugin.
- `getPluginDescription()`: Returns a brief description of what the plugin does.
- `getPluginDependencies()`: Returns a list of plugin IDs that this plugin depends on.
- `getPluginMetadata()`: Returns a JSON object containing all metadata for the plugin.

#### Functionality Methods

- `executeCommand(const QString& command, const QVariantMap& params)`: Executes a plugin-specific command with the given parameters.

### Signals

- `statusChanged(const QString& status)`: Emitted when the plugin status changes.
- `eventOccurred(const QString& eventType, const QVariant& data)`: Emitted when the plugin wants to notify about an event.

## Plugin Metadata

Each plugin must have a JSON metadata file that describes the plugin's properties. This file should have the same name as the plugin ID with a `.json` extension.

### Metadata Format

```json
{
    "id": "MyPlugin",
    "name": "My Plugin",
    "version": "1.0.0",
    "vendor": "My Company",
    "description": "This plugin does something useful",
    "dependencies": ["OtherPlugin1", "OtherPlugin2"],
    "minFrameworkVersion": "1.0.0",
    "category": "Utility",
    "iconPath": ":/icons/myplugin.png",
    "requiredPermissions": ["file.read", "network.access"]
}
```

### Metadata Fields

- `id`: Unique identifier for the plugin. Must match the plugin's implementation of `getPluginId()`.
- `name`: Human-readable name of the plugin.
- `version`: Version of the plugin in format "major.minor.patch".
- `vendor`: Name of the organization that created the plugin.
- `description`: Brief description of what the plugin does.
- `dependencies`: List of plugin IDs that this plugin depends on.
- `minFrameworkVersion`: Minimum framework version required by this plugin.
- `category`: Category this plugin belongs to.
- `iconPath`: Path to the plugin's icon.
- `requiredPermissions`: List of permissions required by this plugin.

## Development Workflow

### 1. Create Plugin Project

Create a new Qt project with the following structure:

```
MyPlugin/
  ├── MyPlugin.pro
  ├── MyPluginPlugin.h
  ├── MyPluginPlugin.cpp
  └── MyPlugin.json
```

### 2. Configure Project File

Configure your `.pro` file to build as a plugin and link with the PluginCore library:

```qmake
QT += core gui widgets

TARGET = MyPlugin
TEMPLATE = lib
CONFIG += plugin

SOURCES += \
    MyPluginPlugin.cpp

HEADERS += \
    MyPluginPlugin.h

DISTFILES += \
    MyPlugin.json

# Link with PluginCore
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../build/release/ -lPluginCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../build/debug/ -lPluginCore
else:unix: LIBS += -L$$PWD/../../build/ -lPluginCore

INCLUDEPATH += $$PWD/../../
DEPENDPATH += $$PWD/../../

# Output directory
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/../../build/debug/plugins
} else {
    DESTDIR = $$PWD/../../build/release/plugins
}

# Copy metadata to build directory
metadata.path = $$DESTDIR/../metadata
metadata.files = MyPlugin.json
INSTALLS += metadata

# Copy metadata during build
win32 {
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$PWD/MyPlugin.json) $$shell_path($$DESTDIR/../metadata/) $$escape_expand(\\n\\t)
} else {
    QMAKE_POST_LINK += $$QMAKE_COPY $$PWD/MyPlugin.json $$DESTDIR/../metadata/ $$escape_expand(\\n\\t)
}
```

### 3. Implement Plugin Class

Create a class that implements the `IPlugin` interface:

```cpp
#ifndef MYPLUGINPLUGIN_H
#define MYPLUGINPLUGIN_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QJsonObject>

#include "../../PluginCore/IPlugin.h"

class MyPluginPlugin : public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "MyPlugin.json")
    Q_INTERFACES(IPlugin)

public:
    MyPluginPlugin();
    ~MyPluginPlugin();

    // IPlugin interface
    bool initialize() override;
    bool activate() override;
    bool deactivate() override;
    bool shutdown() override;
    
    QString getPluginId() const override;
    QString getPluginName() const override;
    QString getPluginVersion() const override;
    QString getPluginVendor() const override;
    QString getPluginDescription() const override;
    QStringList getPluginDependencies() const override;
    QJsonObject getPluginMetadata() const override;
    
    QVariant executeCommand(const QString& command, const QVariantMap& params = QVariantMap()) override;

private:
    QJsonObject m_metadata;
    bool m_initialized;
    bool m_active;
};

#endif // MYPLUGINPLUGIN_H
```

### 4. Implement Plugin Methods

Implement the methods of your plugin class:

```cpp
#include "MyPluginPlugin.h"
#include "../../PluginCore/LogManager.h"
#include "../../PluginCore/ConfigManager.h"
#include "../../PluginCore/PermissionManager.h"

MyPluginPlugin::MyPluginPlugin() : m_initialized(false), m_active(false)
{
    // Load metadata
    QFile metadataFile(":/MyPlugin.json");
    if (metadataFile.open(QIODevice::ReadOnly)) {
        QByteArray metadataBytes = metadataFile.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(metadataBytes);
        m_metadata = doc.object();
        metadataFile.close();
    }
}

MyPluginPlugin::~MyPluginPlugin()
{
    if (m_active) {
        deactivate();
    }
    
    if (m_initialized) {
        shutdown();
    }
}

bool MyPluginPlugin::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    LOG_INFO(getPluginId(), "Initializing My Plugin");
    
    // Check for required permissions
    if (!PermissionManager::instance().hasPermission(getPluginId(), "file.read")) {
        LOG_ERROR(getPluginId(), "Missing required permission: file.read");
        return false;
    }
    
    // Initialize plugin
    // ...
    
    m_initialized = true;
    
    LOG_INFO(getPluginId(), "My Plugin initialized");
    
    return true;
}

bool MyPluginPlugin::activate()
{
    if (!m_initialized) {
        LOG_ERROR(getPluginId(), "Cannot activate: not initialized");
        return false;
    }
    
    if (m_active) {
        return true;
    }
    
    LOG_INFO(getPluginId(), "Activating My Plugin");
    
    // Activate plugin
    // ...
    
    m_active = true;
    
    emit statusChanged("My Plugin activated");
    
    LOG_INFO(getPluginId(), "My Plugin activated");
    
    return true;
}

bool MyPluginPlugin::deactivate()
{
    if (!m_active) {
        return true;
    }
    
    LOG_INFO(getPluginId(), "Deactivating My Plugin");
    
    // Deactivate plugin
    // ...
    
    m_active = false;
    
    emit statusChanged("My Plugin deactivated");
    
    LOG_INFO(getPluginId(), "My Plugin deactivated");
    
    return true;
}

bool MyPluginPlugin::shutdown()
{
    if (!m_initialized) {
        return true;
    }
    
    LOG_INFO(getPluginId(), "Shutting down My Plugin");
    
    // Deactivate if active
    if (m_active) {
        deactivate();
    }
    
    // Clean up resources
    // ...
    
    m_initialized = false;
    
    LOG_INFO(getPluginId(), "My Plugin shut down");
    
    return true;
}

QString MyPluginPlugin::getPluginId() const
{
    return m_metadata.value("id").toString();
}

QString MyPluginPlugin::getPluginName() const
{
    return m_metadata.value("name").toString();
}

QString MyPluginPlugin::getPluginVersion() const
{
    return m_metadata.value("version").toString();
}

QString MyPluginPlugin::getPluginVendor() const
{
    return m_metadata.value("vendor").toString();
}

QString MyPluginPlugin::getPluginDescription() const
{
    return m_metadata.value("description").toString();
}

QStringList MyPluginPlugin::getPluginDependencies() const
{
    QStringList dependencies;
    QJsonArray depsArray = m_metadata.value("dependencies").toArray();
    
    for (const QJsonValue& dep : depsArray) {
        dependencies.append(dep.toString());
    }
    
    return dependencies;
}

QJsonObject MyPluginPlugin::getPluginMetadata() const
{
    return m_metadata;
}

QVariant MyPluginPlugin::executeCommand(const QString& command, const QVariantMap& params)
{
    if (!m_active) {
        LOG_ERROR(getPluginId(), QString("Cannot execute command: plugin not active - %1").arg(command));
        return QVariant();
    }
    
    LOG_INFO(getPluginId(), QString("Executing command: %1").arg(command));
    
    // Handle commands
    if (command == "doSomething") {
        // Do something
        return true;
    }
    
    LOG_WARNING(getPluginId(), QString("Unknown command: %1").arg(command));
    
    return QVariant();
}
```

### 5. Create Metadata File

Create a JSON metadata file for your plugin:

```json
{
    "id": "MyPlugin",
    "name": "My Plugin",
    "version": "1.0.0",
    "vendor": "My Company",
    "description": "This plugin does something useful",
    "dependencies": [],
    "minFrameworkVersion": "1.0.0",
    "category": "Utility",
    "iconPath": ":/icons/myplugin.png",
    "requiredPermissions": ["file.read"]
}
```

### 6. Build and Deploy

Build your plugin and ensure it is copied to the correct location:

- The plugin library (`.dll`, `.so`, or `.dylib`) should be in the `plugins` directory.
- The metadata file (`.json`) should be in the `metadata` directory.

## Framework Services

### Logging

Use the `LogManager` to log messages:

```cpp
LOG_DEBUG(getPluginId(), "Debug message");
LOG_INFO(getPluginId(), "Info message");
LOG_WARNING(getPluginId(), "Warning message");
LOG_ERROR(getPluginId(), "Error message");
LOG_FATAL(getPluginId(), "Fatal message");
```

### Configuration

Use the `ConfigManager` to store and retrieve configuration:

```cpp
// Get configuration value
QString value = ConfigManager::instance().getPluginValue(getPluginId(), "key", "default").toString();

// Set configuration value
ConfigManager::instance().setPluginValue(getPluginId(), "key", "value");

// Save configuration
QString configDir = QCoreApplication::applicationDirPath() + "/config";
QString configFile = QDir(configDir).filePath(getPluginId() + ".json");
ConfigManager::instance().savePluginConfig(getPluginId(), configFile);
```

### Permissions

Use the `PermissionManager` to check permissions:

```cpp
// Check if plugin has permission
if (!PermissionManager::instance().hasPermission(getPluginId(), "file.write")) {
    LOG_ERROR(getPluginId(), "Missing required permission: file.write");
    return false;
}
```

### Exception Handling

Use the `ExceptionHandler` to handle exceptions:

```cpp
try {
    // Do something that might throw
} catch (const PluginException& ex) {
    ExceptionHandler::instance().handleException(ex);
} catch (const std::exception& ex) {
    THROW_PLUGIN_EXCEPTION(getPluginId(), ex.what(), -1);
} catch (...) {
    THROW_PLUGIN_EXCEPTION(getPluginId(), "Unknown exception", -1);
}
```

### Plugin Communication

Use the `PluginCommunication` service to communicate with other plugins:

```cpp
// Register message handler
PluginCommunication::instance().registerMessageHandler(getPluginId(), "messageType",
    [this](const QString& sender, const QVariant& data) -> QVariant {
        // Handle message
        return QVariant();
    });

// Send message to another plugin
QVariant response = PluginCommunication::instance().sendMessage(getPluginId(), "otherPlugin", "messageType", data);

// Broadcast message to all plugins
QMap<QString, QVariant> responses = PluginCommunication::instance().broadcastMessage(getPluginId(), "messageType", data);
```

## UI Integration

Plugins can integrate with the host application's UI in several ways:

### Menu Integration

The host application will automatically create a menu for your plugin based on your plugin name. You can add actions to this menu by implementing the `executeCommand` method to handle specific commands.

### Custom Dialogs

Plugins can create their own dialogs and windows:

```cpp
QDialog* dialog = new QDialog();
// Configure dialog
dialog->exec();
delete dialog;
```

### Status Updates

Plugins can update their status in the host application:

```cpp
emit statusChanged("My Plugin is doing something");
```

### Event Notifications

Plugins can notify the host application about events:

```cpp
emit eventOccurred("myEvent", data);
```

## Best Practices

1. **Error Handling**: Always check return values and handle errors gracefully.
2. **Resource Management**: Clean up resources in the `shutdown` method.
3. **Thread Safety**: If your plugin uses threads, ensure proper synchronization.
4. **Configuration**: Store user preferences and settings using the ConfigManager.
5. **Logging**: Use the LogManager for all logging to ensure consistent log format.
6. **Permissions**: Request only the permissions your plugin actually needs.
7. **Dependencies**: Specify all required dependencies in your metadata.
8. **Version Compatibility**: Specify the minimum framework version your plugin requires.
9. **Documentation**: Document your plugin's commands and functionality.
10. **Testing**: Test your plugin thoroughly before deployment.

## Troubleshooting

### Plugin Not Loading

- Check that the plugin library is in the correct location.
- Check that the metadata file is in the correct location.
- Check the log for error messages.
- Verify that all dependencies are available.
- Ensure the plugin is compatible with the framework version.

### Plugin Not Activating

- Check that the plugin has been initialized.
- Check that all required permissions are granted.
- Check the log for error messages.

### Plugin Commands Not Working

- Check that the plugin is active.
- Check that the command name is correct.
- Check the log for error messages.

## Conclusion

This guide provides the basics for developing plugins for the Enterprise Plugin Framework. For more detailed information, refer to the API documentation and example plugins.