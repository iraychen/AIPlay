#include "PermissionManager.h"
#include "LogManager.h"

PermissionManager::PermissionManager() : m_initialized(false)
{
}

PermissionManager::~PermissionManager()
{
    shutdown();
}

PermissionManager& PermissionManager::instance()
{
    static PermissionManager instance;
    return instance;
}

bool PermissionManager::initialize()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        LOG_WARNING("PermissionManager", "Already initialized");
        return true;
    }
    
    // Register some common permissions
    registerPermission("file.read", "Read files from the file system");
    registerPermission("file.write", "Write files to the file system");
    registerPermission("network.access", "Access network resources");
    registerPermission("database.access", "Access database resources");
    registerPermission("ui.modify", "Modify the user interface");
    registerPermission("system.execute", "Execute system commands");
    
    m_initialized = true;
    
    LOG_INFO("PermissionManager", "Initialized");
    
    return true;
}

void PermissionManager::shutdown()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        LOG_INFO("PermissionManager", "Shutting down");
        
        m_permissions.clear();
        m_pluginPermissions.clear();
        
        m_initialized = false;
    }
}

bool PermissionManager::registerPermission(const QString& permission, const QString& description)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("PermissionManager", "Not initialized");
        return false;
    }
    
    if (m_permissions.contains(permission)) {
        LOG_WARNING("PermissionManager", QString("Permission already registered: %1").arg(permission));
        return false;
    }
    
    m_permissions.insert(permission, description);
    
    LOG_INFO("PermissionManager", QString("Registered permission: %1").arg(permission));
    
    emit permissionRegistered(permission, description);
    
    return true;
}

bool PermissionManager::unregisterPermission(const QString& permission)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("PermissionManager", "Not initialized");
        return false;
    }
    
    if (!m_permissions.contains(permission)) {
        LOG_WARNING("PermissionManager", QString("Permission not registered: %1").arg(permission));
        return false;
    }
    
    // Remove the permission from all plugins
    for (auto it = m_pluginPermissions.begin(); it != m_pluginPermissions.end(); ++it) {
        it.value().remove(permission);
    }
    
    m_permissions.remove(permission);
    
    LOG_INFO("PermissionManager", QString("Unregistered permission: %1").arg(permission));
    
    emit permissionUnregistered(permission);
    
    return true;
}

bool PermissionManager::isPermissionRegistered(const QString& permission) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("PermissionManager", "Not initialized");
        return false;
    }
    
    return m_permissions.contains(permission);
}

QStringList PermissionManager::getRegisteredPermissions() const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("PermissionManager", "Not initialized");
        return QStringList();
    }
    
    return m_permissions.keys();
}

QString PermissionManager::getPermissionDescription(const QString& permission) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("PermissionManager", "Not initialized");
        return QString();
    }
    
    return m_permissions.value(permission);
}

bool PermissionManager::grantPermission(const QString& pluginId, const QString& permission)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("PermissionManager", "Not initialized");
        return false;
    }
    
    if (!m_permissions.contains(permission)) {
        LOG_WARNING("PermissionManager", QString("Permission not registered: %1").arg(permission));
        return false;
    }
    
    if (!m_pluginPermissions.contains(pluginId)) {
        m_pluginPermissions.insert(pluginId, QSet<QString>());
    }
    
    if (m_pluginPermissions[pluginId].contains(permission)) {
        LOG_WARNING("PermissionManager", QString("Plugin %1 already has permission: %2").arg(pluginId, permission));
        return true;
    }
    
    m_pluginPermissions[pluginId].insert(permission);
    
    LOG_INFO("PermissionManager", QString("Granted permission %1 to plugin %2").arg(permission, pluginId));
    
    emit permissionGranted(pluginId, permission);
    
    return true;
}

bool PermissionManager::revokePermission(const QString& pluginId, const QString& permission)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("PermissionManager", "Not initialized");
        return false;
    }
    
    if (!m_pluginPermissions.contains(pluginId)) {
        LOG_WARNING("PermissionManager", QString("Plugin not registered: %1").arg(pluginId));
        return false;
    }
    
    if (!m_pluginPermissions[pluginId].contains(permission)) {
        LOG_WARNING("PermissionManager", QString("Plugin %1 does not have permission: %2").arg(pluginId, permission));
        return false;
    }
    
    m_pluginPermissions[pluginId].remove(permission);
    
    LOG_INFO("PermissionManager", QString("Revoked permission %1 from plugin %2").arg(permission, pluginId));
    
    emit permissionRevoked(pluginId, permission);
    
    return true;
}

bool PermissionManager::hasPermission(const QString& pluginId, const QString& permission) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("PermissionManager", "Not initialized");
        return false;
    }
    
    if (!m_pluginPermissions.contains(pluginId)) {
        return false;
    }
    
    return m_pluginPermissions[pluginId].contains(permission);
}

QStringList PermissionManager::getPluginPermissions(const QString& pluginId) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("PermissionManager", "Not initialized");
        return QStringList();
    }
    
    if (!m_pluginPermissions.contains(pluginId)) {
        return QStringList();
    }
    
    // Convert QSet to QStringList manually
    QStringList result;
    const QSet<QString>& permissions = m_pluginPermissions[pluginId];
    for (const QString& permission : permissions) {
        result.append(permission);
    }
    return result;
}

QStringList PermissionManager::getPluginsWithPermission(const QString& permission) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("PermissionManager", "Not initialized");
        return QStringList();
    }
    
    QStringList plugins;
    
    for (auto it = m_pluginPermissions.begin(); it != m_pluginPermissions.end(); ++it) {
        if (it.value().contains(permission)) {
            plugins.append(it.key());
        }
    }
    
    return plugins;
}