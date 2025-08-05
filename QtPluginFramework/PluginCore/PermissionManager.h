#ifndef PERMISSIONMANAGER_H
#define PERMISSIONMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QSet>
#include <QMutex>

/**
 * @brief The PermissionManager class manages permissions for plugins.
 * 
 * This class implements the Singleton pattern to ensure a single permission manager
 * instance throughout the application.
 */
class PermissionManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance of PermissionManager
     * 
     * @return Reference to the singleton PermissionManager instance
     */
    static PermissionManager& instance();

    /**
     * @brief Initialize the permission manager
     * 
     * @return True if initialization was successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Shutdown the permission manager
     */
    void shutdown();

    /**
     * @brief Register a permission
     * 
     * @param permission Permission to register
     * @param description Description of the permission
     * @return True if registration was successful, false otherwise
     */
    bool registerPermission(const QString& permission, const QString& description);

    /**
     * @brief Unregister a permission
     * 
     * @param permission Permission to unregister
     * @return True if unregistration was successful, false otherwise
     */
    bool unregisterPermission(const QString& permission);

    /**
     * @brief Check if a permission is registered
     * 
     * @param permission Permission to check
     * @return True if the permission is registered, false otherwise
     */
    bool isPermissionRegistered(const QString& permission) const;

    /**
     * @brief Get all registered permissions
     * 
     * @return List of all registered permissions
     */
    QStringList getRegisteredPermissions() const;

    /**
     * @brief Get the description of a permission
     * 
     * @param permission Permission to get description for
     * @return Description of the permission
     */
    QString getPermissionDescription(const QString& permission) const;

    /**
     * @brief Grant a permission to a plugin
     * 
     * @param pluginId ID of the plugin
     * @param permission Permission to grant
     * @return True if the permission was granted, false otherwise
     */
    bool grantPermission(const QString& pluginId, const QString& permission);

    /**
     * @brief Revoke a permission from a plugin
     * 
     * @param pluginId ID of the plugin
     * @param permission Permission to revoke
     * @return True if the permission was revoked, false otherwise
     */
    bool revokePermission(const QString& pluginId, const QString& permission);

    /**
     * @brief Check if a plugin has a permission
     * 
     * @param pluginId ID of the plugin
     * @param permission Permission to check
     * @return True if the plugin has the permission, false otherwise
     */
    bool hasPermission(const QString& pluginId, const QString& permission) const;

    /**
     * @brief Get all permissions granted to a plugin
     * 
     * @param pluginId ID of the plugin
     * @return List of all permissions granted to the plugin
     */
    QStringList getPluginPermissions(const QString& pluginId) const;

    /**
     * @brief Get all plugins that have a specific permission
     * 
     * @param permission Permission to check
     * @return List of all plugin IDs that have the permission
     */
    QStringList getPluginsWithPermission(const QString& permission) const;

signals:
    /**
     * @brief Signal emitted when a permission is registered
     * 
     * @param permission Permission that was registered
     * @param description Description of the permission
     */
    void permissionRegistered(const QString& permission, const QString& description);

    /**
     * @brief Signal emitted when a permission is unregistered
     * 
     * @param permission Permission that was unregistered
     */
    void permissionUnregistered(const QString& permission);

    /**
     * @brief Signal emitted when a permission is granted to a plugin
     * 
     * @param pluginId ID of the plugin
     * @param permission Permission that was granted
     */
    void permissionGranted(const QString& pluginId, const QString& permission);

    /**
     * @brief Signal emitted when a permission is revoked from a plugin
     * 
     * @param pluginId ID of the plugin
     * @param permission Permission that was revoked
     */
    void permissionRevoked(const QString& pluginId, const QString& permission);

private:
    // Private constructor for singleton pattern
    PermissionManager();
    
    // Deleted copy constructor and assignment operator
    PermissionManager(const PermissionManager&) = delete;
    PermissionManager& operator=(const PermissionManager&) = delete;
    
    // Destructor
    ~PermissionManager();

    QMap<QString, QString> m_permissions; // Permission -> Description
    QMap<QString, QSet<QString>> m_pluginPermissions; // PluginId -> Set of permissions
    mutable QMutex m_mutex;
    bool m_initialized;
};

#endif // PERMISSIONMANAGER_H