#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QFile>
#include <QDir>
#include <QMutex>
#include <QStringList>

/**
 * @brief The ConfigManager class manages configuration settings for the framework and plugins.
 * 
 * This class implements the Singleton pattern to ensure a single configuration manager
 * instance throughout the application.
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance of ConfigManager
     * 
     * @return Reference to the singleton ConfigManager instance
     */
    static ConfigManager& instance();

    /**
     * @brief Initialize the configuration manager
     * 
     * @param configDir Directory where configuration files are stored
     * @return True if initialization was successful, false otherwise
     */
    bool initialize(const QString& configDir);

    /**
     * @brief Shutdown the configuration manager
     */
    void shutdown();

    /**
     * @brief Load framework configuration
     * 
     * @param configFile Path to the framework configuration file
     * @return True if loading was successful, false otherwise
     */
    bool loadFrameworkConfig(const QString& configFile);

    /**
     * @brief Save framework configuration
     * 
     * @param configFile Path to the framework configuration file
     * @return True if saving was successful, false otherwise
     */
    bool saveFrameworkConfig(const QString& configFile);

    /**
     * @brief Load plugin configuration
     * 
     * @param pluginId ID of the plugin
     * @param configFile Path to the plugin configuration file
     * @return True if loading was successful, false otherwise
     */
    bool loadPluginConfig(const QString& pluginId, const QString& configFile);

    /**
     * @brief Save plugin configuration
     * 
     * @param pluginId ID of the plugin
     * @param configFile Path to the plugin configuration file
     * @return True if saving was successful, false otherwise
     */
    bool savePluginConfig(const QString& pluginId, const QString& configFile);

    /**
     * @brief Get a framework configuration value
     * 
     * @param key Configuration key
     * @param defaultValue Default value to return if key is not found
     * @return Configuration value
     */
    QVariant getFrameworkValue(const QString& key, const QVariant& defaultValue = QVariant()) const;

    /**
     * @brief Set a framework configuration value
     * 
     * @param key Configuration key
     * @param value Configuration value
     */
    void setFrameworkValue(const QString& key, const QVariant& value);

    /**
     * @brief Get a plugin configuration value
     * 
     * @param pluginId ID of the plugin
     * @param key Configuration key
     * @param defaultValue Default value to return if key is not found
     * @return Configuration value
     */
    QVariant getPluginValue(const QString& pluginId, const QString& key, const QVariant& defaultValue = QVariant()) const;

    /**
     * @brief Set a plugin configuration value
     * 
     * @param pluginId ID of the plugin
     * @param key Configuration key
     * @param value Configuration value
     */
    void setPluginValue(const QString& pluginId, const QString& key, const QVariant& value);

    /**
     * @brief Check if a framework configuration key exists
     * 
     * @param key Configuration key
     * @return True if the key exists, false otherwise
     */
    bool hasFrameworkKey(const QString& key) const;

    /**
     * @brief Check if a plugin configuration key exists
     * 
     * @param pluginId ID of the plugin
     * @param key Configuration key
     * @return True if the key exists, false otherwise
     */
    bool hasPluginKey(const QString& pluginId, const QString& key) const;

    /**
     * @brief Remove a framework configuration key
     * 
     * @param key Configuration key
     * @return True if the key was removed, false otherwise
     */
    bool removeFrameworkKey(const QString& key);

    /**
     * @brief Remove a plugin configuration key
     * 
     * @param pluginId ID of the plugin
     * @param key Configuration key
     * @return True if the key was removed, false otherwise
     */
    bool removePluginKey(const QString& pluginId, const QString& key);

    /**
     * @brief Get all framework configuration keys
     * 
     * @return List of all framework configuration keys
     */
    QStringList getFrameworkKeys() const;

    /**
     * @brief Get all plugin configuration keys
     * 
     * @param pluginId ID of the plugin
     * @return List of all plugin configuration keys
     */
    QStringList getPluginKeys(const QString& pluginId) const;

    /**
     * @brief Get all framework configuration as a JSON object
     * 
     * @return JSON object containing all framework configuration
     */
    QJsonObject getFrameworkConfigAsJson() const;

    /**
     * @brief Get all plugin configuration as a JSON object
     * 
     * @param pluginId ID of the plugin
     * @return JSON object containing all plugin configuration
     */
    QJsonObject getPluginConfigAsJson(const QString& pluginId) const;

signals:
    /**
     * @brief Signal emitted when a framework configuration value changes
     * 
     * @param key Configuration key
     * @param value New configuration value
     */
    void frameworkConfigChanged(const QString& key, const QVariant& value);

    /**
     * @brief Signal emitted when a plugin configuration value changes
     * 
     * @param pluginId ID of the plugin
     * @param key Configuration key
     * @param value New configuration value
     */
    void pluginConfigChanged(const QString& pluginId, const QString& key, const QVariant& value);

private:
    // Private constructor for singleton pattern
    ConfigManager();
    
    // Deleted copy constructor and assignment operator
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // Destructor
    ~ConfigManager();

    /**
     * @brief Convert a QVariant to a QJsonValue
     * 
     * @param value QVariant to convert
     * @return Converted QJsonValue
     */
    QJsonValue variantToJsonValue(const QVariant& value) const;

    /**
     * @brief Convert a QJsonValue to a QVariant
     * 
     * @param value QJsonValue to convert
     * @return Converted QVariant
     */
    QVariant jsonValueToVariant(const QJsonValue& value) const;

    QString m_configDir;
    QMap<QString, QVariant> m_frameworkConfig;
    QMap<QString, QMap<QString, QVariant>> m_pluginConfigs;
    mutable QMutex m_mutex;
    bool m_initialized;
};

#endif // CONFIGMANAGER_H