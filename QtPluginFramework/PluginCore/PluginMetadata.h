#ifndef PLUGINMETADATA_H
#define PLUGINMETADATA_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QVersionNumber>

/**
 * @brief The PluginMetadata class manages metadata for a plugin.
 * 
 * This class handles loading, parsing, and accessing plugin metadata from JSON files.
 */
class PluginMetadata
{
public:
    /**
     * @brief Constructor
     */
    PluginMetadata();

    /**
     * @brief Constructor with JSON data
     * 
     * @param metadataJson The JSON object containing plugin metadata
     */
    explicit PluginMetadata(const QJsonObject& metadataJson);

    /**
     * @brief Load metadata from a JSON file
     * 
     * @param filePath Path to the metadata JSON file
     * @return True if loading was successful, false otherwise
     */
    bool loadFromFile(const QString& filePath);

    /**
     * @brief Load metadata from a JSON string
     * 
     * @param jsonString JSON string containing plugin metadata
     * @return True if loading was successful, false otherwise
     */
    bool loadFromString(const QString& jsonString);

    /**
     * @brief Check if the metadata is valid
     * 
     * @return True if the metadata is valid, false otherwise
     */
    bool isValid() const;

    /**
     * @brief Get the plugin ID
     * 
     * @return The unique identifier for this plugin
     */
    QString getPluginId() const;

    /**
     * @brief Get the plugin name
     * 
     * @return The human-readable name of this plugin
     */
    QString getPluginName() const;

    /**
     * @brief Get the plugin version
     * 
     * @return The version of this plugin in format "major.minor.patch"
     */
    QString getPluginVersion() const;

    /**
     * @brief Get the plugin version as a QVersionNumber
     * 
     * @return The version of this plugin as a QVersionNumber
     */
    QVersionNumber getVersionNumber() const;

    /**
     * @brief Get the plugin vendor
     * 
     * @return The name of the organization that created this plugin
     */
    QString getPluginVendor() const;

    /**
     * @brief Get the plugin description
     * 
     * @return A brief description of what this plugin does
     */
    QString getPluginDescription() const;

    /**
     * @brief Get the plugin dependencies
     * 
     * @return A list of plugin IDs that this plugin depends on
     */
    QStringList getPluginDependencies() const;

    /**
     * @brief Get the minimum framework version required by this plugin
     * 
     * @return The minimum framework version in format "major.minor.patch"
     */
    QString getMinFrameworkVersion() const;

    /**
     * @brief Get the minimum framework version as a QVersionNumber
     * 
     * @return The minimum framework version as a QVersionNumber
     */
    QVersionNumber getMinFrameworkVersionNumber() const;

    /**
     * @brief Get the plugin category
     * 
     * @return The category this plugin belongs to
     */
    QString getCategory() const;

    /**
     * @brief Get the plugin icon path
     * 
     * @return Path to the plugin's icon
     */
    QString getIconPath() const;

    /**
     * @brief Get the plugin's required permissions
     * 
     * @return List of permissions required by this plugin
     */
    QStringList getRequiredPermissions() const;

    /**
     * @brief Get the complete metadata as a JSON object
     * 
     * @return JSON object containing all metadata
     */
    QJsonObject getMetadataJson() const;

    /**
     * @brief Check if this plugin is compatible with the given framework version
     * 
     * @param frameworkVersion The framework version to check against
     * @return True if compatible, false otherwise
     */
    bool isCompatibleWithFramework(const QString& frameworkVersion) const;

    /**
     * @brief Check if this plugin depends on another plugin
     * 
     * @param pluginId The ID of the plugin to check dependency on
     * @return True if this plugin depends on the specified plugin, false otherwise
     */
    bool dependsOn(const QString& pluginId) const;

private:
    QJsonObject m_metadata;
    bool m_isValid;
};

#endif // PLUGINMETADATA_H