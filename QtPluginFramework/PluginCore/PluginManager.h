#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QDir>
#include <QPluginLoader>
#include <QMutex>
#include <QSet>
#include <QJsonObject>
#include <QVariant>
#include <QVariantMap>

#include "IPlugin.h"
#include "PluginMetadata.h"

/**
 * @brief Enumeration of plugin states
 */
enum class PluginState {
    NotLoaded,
    Loaded,
    Initialized,
    Active,
    Inactive,
    Failed
};

/**
 * @brief The PluginManager class manages the loading, unloading, and lifecycle of plugins.
 * 
 * This class implements the Singleton pattern to ensure a single plugin manager
 * instance throughout the application.
 */
class PluginManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance of PluginManager
     * 
     * @return Reference to the singleton PluginManager instance
     */
    static PluginManager& instance();

    /**
     * @brief Initialize the plugin manager
     * 
     * @param pluginDir Directory where plugins are stored
     * @param metadataDir Directory where plugin metadata is stored
     * @return True if initialization was successful, false otherwise
     */
    bool initialize(const QString& pluginDir, const QString& metadataDir);

    /**
     * @brief Shutdown the plugin manager
     */
    void shutdown();

    /**
     * @brief Scan for available plugins
     * 
     * @return List of plugin IDs found
     */
    QStringList scanForPlugins();

    /**
     * @brief Load a plugin
     * 
     * @param pluginId ID of the plugin to load
     * @return True if loading was successful, false otherwise
     */
    bool loadPlugin(const QString& pluginId);

    /**
     * @brief Unload a plugin
     * 
     * @param pluginId ID of the plugin to unload
     * @return True if unloading was successful, false otherwise
     */
    bool unloadPlugin(const QString& pluginId);

    /**
     * @brief Initialize a plugin
     * 
     * @param pluginId ID of the plugin to initialize
     * @return True if initialization was successful, false otherwise
     */
    bool initializePlugin(const QString& pluginId);

    /**
     * @brief Activate a plugin
     * 
     * @param pluginId ID of the plugin to activate
     * @return True if activation was successful, false otherwise
     */
    bool activatePlugin(const QString& pluginId);

    /**
     * @brief Deactivate a plugin
     * 
     * @param pluginId ID of the plugin to deactivate
     * @return True if deactivation was successful, false otherwise
     */
    bool deactivatePlugin(const QString& pluginId);

    /**
     * @brief Get a plugin instance
     * 
     * @param pluginId ID of the plugin
     * @return Pointer to the plugin instance, or nullptr if not found
     */
    IPlugin* getPlugin(const QString& pluginId) const;

    /**
     * @brief Get all loaded plugins
     * 
     * @return Map of plugin IDs to plugin instances
     */
    QMap<QString, IPlugin*> getLoadedPlugins() const;

    /**
     * @brief Get all active plugins
     * 
     * @return Map of plugin IDs to plugin instances
     */
    QMap<QString, IPlugin*> getActivePlugins() const;

    /**
     * @brief Get the state of a plugin
     * 
     * @param pluginId ID of the plugin
     * @return State of the plugin
     */
    PluginState getPluginState(const QString& pluginId) const;

    /**
     * @brief Get the metadata of a plugin
     * 
     * @param pluginId ID of the plugin
     * @return Metadata of the plugin
     */
    PluginMetadata getPluginMetadata(const QString& pluginId) const;

    /**
     * @brief Get all available plugins
     * 
     * @return Map of plugin IDs to plugin metadata
     */
    QMap<QString, PluginMetadata> getAvailablePlugins() const;

    /**
     * @brief Check if a plugin is loaded
     * 
     * @param pluginId ID of the plugin
     * @return True if the plugin is loaded, false otherwise
     */
    bool isPluginLoaded(const QString& pluginId) const;

    /**
     * @brief Check if a plugin is active
     * 
     * @param pluginId ID of the plugin
     * @return True if the plugin is active, false otherwise
     */
    bool isPluginActive(const QString& pluginId) const;

    /**
     * @brief Execute a command on a plugin
     * 
     * @param pluginId ID of the plugin
     * @param command Command to execute
     * @param params Parameters for the command
     * @return Result of the command execution
     */
    QVariant executePluginCommand(const QString& pluginId, const QString& command, const QVariantMap& params = QVariantMap());

    /**
     * @brief Get the framework version
     * 
     * @return Framework version in format "major.minor.patch"
     */
    QString getFrameworkVersion() const;

signals:
    /**
     * @brief Signal emitted when a plugin is loaded
     * 
     * @param pluginId ID of the plugin
     */
    void pluginLoaded(const QString& pluginId);

    /**
     * @brief Signal emitted when a plugin is unloaded
     * 
     * @param pluginId ID of the plugin
     */
    void pluginUnloaded(const QString& pluginId);

    /**
     * @brief Signal emitted when a plugin is initialized
     * 
     * @param pluginId ID of the plugin
     */
    void pluginInitialized(const QString& pluginId);

    /**
     * @brief Signal emitted when a plugin is activated
     * 
     * @param pluginId ID of the plugin
     */
    void pluginActivated(const QString& pluginId);

    /**
     * @brief Signal emitted when a plugin is deactivated
     * 
     * @param pluginId ID of the plugin
     */
    void pluginDeactivated(const QString& pluginId);

    /**
     * @brief Signal emitted when a plugin fails to load, initialize, or activate
     * 
     * @param pluginId ID of the plugin
     * @param errorMessage Error message
     */
    void pluginFailed(const QString& pluginId, const QString& errorMessage);

private:
    // Private constructor for singleton pattern
    PluginManager();
    
    // Deleted copy constructor and assignment operator
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    
    // Destructor
    ~PluginManager();

    /**
     * @brief Load plugin metadata
     * 
     * @param pluginId ID of the plugin
     * @return True if loading was successful, false otherwise
     */
    bool loadPluginMetadata(const QString& pluginId);

    /**
     * @brief Check if a plugin's dependencies are satisfied
     * 
     * @param pluginId ID of the plugin
     * @return True if all dependencies are satisfied, false otherwise
     */
    bool checkPluginDependencies(const QString& pluginId);

    /**
     * @brief Get the plugins that depend on a specific plugin
     * 
     * @param pluginId ID of the plugin
     * @return List of plugin IDs that depend on the specified plugin
     */
    QStringList getDependentPlugins(const QString& pluginId) const;

    /**
     * @brief Sort plugins by dependency order
     * 
     * @param pluginIds List of plugin IDs to sort
     * @return Sorted list of plugin IDs
     */
    QStringList sortPluginsByDependency(const QStringList& pluginIds);

    /**
     * @brief Recursively build dependency graph
     * 
     * @param pluginId ID of the plugin
     * @param visited Set of visited plugin IDs
     * @param sortedPlugins List of sorted plugin IDs
     */
    void buildDependencyGraph(const QString& pluginId, QSet<QString>& visited, QStringList& sortedPlugins);

    QString m_pluginDir;
    QString m_metadataDir;
    QMap<QString, QPluginLoader*> m_pluginLoaders;
    QMap<QString, IPlugin*> m_plugins;
    QMap<QString, PluginMetadata> m_pluginMetadata;
    QMap<QString, PluginState> m_pluginStates;
    mutable QMutex m_mutex;
    bool m_initialized;
    
    // Framework version
    const QString m_frameworkVersion = "1.0.0";
};

#endif // PLUGINMANAGER_H