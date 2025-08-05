#ifndef IPLUGIN_H
#define IPLUGIN_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QJsonObject>

/**
 * @brief The IPlugin class defines the interface for all plugins in the framework.
 * 
 * This interface provides methods for plugin lifecycle management, metadata access,
 * and core functionality that all plugins must implement.
 */
class IPlugin : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Destructor
     */
    virtual ~IPlugin() {}

    /**
     * @brief Initialize the plugin
     * 
     * This method is called after the plugin is loaded but before it's activated.
     * Use this method to perform any initialization tasks.
     * 
     * @return True if initialization was successful, false otherwise
     */
    virtual bool initialize() = 0;

    /**
     * @brief Activate the plugin
     * 
     * This method is called when the plugin should become active and start
     * providing its functionality.
     * 
     * @return True if activation was successful, false otherwise
     */
    virtual bool activate() = 0;

    /**
     * @brief Deactivate the plugin
     * 
     * This method is called when the plugin should become inactive and stop
     * providing its functionality.
     * 
     * @return True if deactivation was successful, false otherwise
     */
    virtual bool deactivate() = 0;

    /**
     * @brief Shutdown the plugin
     * 
     * This method is called before the plugin is unloaded.
     * Use this method to perform any cleanup tasks.
     * 
     * @return True if shutdown was successful, false otherwise
     */
    virtual bool shutdown() = 0;

    /**
     * @brief Get the plugin ID
     * 
     * @return The unique identifier for this plugin
     */
    virtual QString getPluginId() const = 0;

    /**
     * @brief Get the plugin name
     * 
     * @return The human-readable name of this plugin
     */
    virtual QString getPluginName() const = 0;

    /**
     * @brief Get the plugin version
     * 
     * @return The version of this plugin in format "major.minor.patch"
     */
    virtual QString getPluginVersion() const = 0;

    /**
     * @brief Get the plugin vendor
     * 
     * @return The name of the organization that created this plugin
     */
    virtual QString getPluginVendor() const = 0;

    /**
     * @brief Get the plugin description
     * 
     * @return A brief description of what this plugin does
     */
    virtual QString getPluginDescription() const = 0;

    /**
     * @brief Get the plugin dependencies
     * 
     * @return A list of plugin IDs that this plugin depends on
     */
    virtual QStringList getPluginDependencies() const = 0;

    /**
     * @brief Get the plugin metadata
     * 
     * @return A JSON object containing all metadata for this plugin
     */
    virtual QJsonObject getPluginMetadata() const = 0;

    /**
     * @brief Execute a plugin-specific command
     * 
     * This method allows other components to invoke functionality provided by this plugin.
     * 
     * @param command The command to execute
     * @param params Parameters for the command
     * @return The result of the command execution
     */
    virtual QVariant executeCommand(const QString& command, const QVariantMap& params = QVariantMap()) = 0;

signals:
    /**
     * @brief Signal emitted when the plugin status changes
     * 
     * @param status The new status message
     */
    void statusChanged(const QString& status);

    /**
     * @brief Signal emitted when the plugin wants to notify about an event
     * 
     * @param eventType The type of event
     * @param data Data associated with the event
     */
    void eventOccurred(const QString& eventType, const QVariant& data);
};

// Define the plugin interface ID for Qt's plugin system
#define PluginInterface_iid "com.enterprise.plugin.IPlugin"
Q_DECLARE_INTERFACE(IPlugin, PluginInterface_iid)

#endif // IPLUGIN_H