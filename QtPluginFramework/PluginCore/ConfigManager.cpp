#include "ConfigManager.h"
#include "LogManager.h"

ConfigManager::ConfigManager() : m_initialized(false)
{
}

ConfigManager::~ConfigManager()
{
    shutdown();
}

ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::initialize(const QString& configDir)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        LOG_WARNING("ConfigManager", "Already initialized");
        return true;
    }
    
    QDir dir(configDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            LOG_ERROR("ConfigManager", QString("Failed to create config directory: %1").arg(configDir));
            return false;
        }
    }
    
    m_configDir = configDir;
    m_initialized = true;
    
    LOG_INFO("ConfigManager", QString("Initialized with config directory: %1").arg(configDir));
    
    return true;
}

void ConfigManager::shutdown()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        LOG_INFO("ConfigManager", "Shutting down");
        m_initialized = false;
    }
}

bool ConfigManager::loadFrameworkConfig(const QString& configFile)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return false;
    }
    
    QFile file(configFile);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARNING("ConfigManager", QString("Failed to open framework config file: %1").arg(configFile));
        return false;
    }
    
    QByteArray jsonData = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull() || !doc.isObject()) {
        LOG_ERROR("ConfigManager", QString("Invalid JSON in framework config file: %1").arg(configFile));
        return false;
    }
    
    QJsonObject jsonObj = doc.object();
    
    // Clear existing framework config
    m_frameworkConfig.clear();
    
    // Convert JSON object to QMap
    for (auto it = jsonObj.begin(); it != jsonObj.end(); ++it) {
        m_frameworkConfig.insert(it.key(), jsonValueToVariant(it.value()));
    }
    
    LOG_INFO("ConfigManager", QString("Loaded framework config from: %1").arg(configFile));
    
    return true;
}

bool ConfigManager::saveFrameworkConfig(const QString& configFile)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return false;
    }
    
    QJsonObject jsonObj;
    
    // Convert QMap to JSON object
    for (auto it = m_frameworkConfig.begin(); it != m_frameworkConfig.end(); ++it) {
        jsonObj.insert(it.key(), variantToJsonValue(it.value()));
    }
    
    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    
    QFile file(configFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LOG_ERROR("ConfigManager", QString("Failed to open framework config file for writing: %1").arg(configFile));
        return false;
    }
    
    qint64 bytesWritten = file.write(jsonData);
    file.close();
    
    if (bytesWritten != jsonData.size()) {
        LOG_ERROR("ConfigManager", QString("Failed to write all data to framework config file: %1").arg(configFile));
        return false;
    }
    
    LOG_INFO("ConfigManager", QString("Saved framework config to: %1").arg(configFile));
    
    return true;
}

bool ConfigManager::loadPluginConfig(const QString& pluginId, const QString& configFile)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return false;
    }
    
    QFile file(configFile);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARNING("ConfigManager", QString("Failed to open plugin config file: %1").arg(configFile));
        return false;
    }
    
    QByteArray jsonData = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull() || !doc.isObject()) {
        LOG_ERROR("ConfigManager", QString("Invalid JSON in plugin config file: %1").arg(configFile));
        return false;
    }
    
    QJsonObject jsonObj = doc.object();
    
    // Clear existing plugin config
    m_pluginConfigs[pluginId].clear();
    
    // Convert JSON object to QMap
    for (auto it = jsonObj.begin(); it != jsonObj.end(); ++it) {
        m_pluginConfigs[pluginId].insert(it.key(), jsonValueToVariant(it.value()));
    }
    
    LOG_INFO("ConfigManager", QString("Loaded config for plugin %1 from: %2").arg(pluginId, configFile));
    
    return true;
}

bool ConfigManager::savePluginConfig(const QString& pluginId, const QString& configFile)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return false;
    }
    
    if (!m_pluginConfigs.contains(pluginId)) {
        LOG_WARNING("ConfigManager", QString("No config found for plugin: %1").arg(pluginId));
        return false;
    }
    
    QJsonObject jsonObj;
    
    // Convert QMap to JSON object
    const QMap<QString, QVariant>& pluginConfig = m_pluginConfigs[pluginId];
    for (auto it = pluginConfig.begin(); it != pluginConfig.end(); ++it) {
        jsonObj.insert(it.key(), variantToJsonValue(it.value()));
    }
    
    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    
    QFile file(configFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LOG_ERROR("ConfigManager", QString("Failed to open plugin config file for writing: %1").arg(configFile));
        return false;
    }
    
    qint64 bytesWritten = file.write(jsonData);
    file.close();
    
    if (bytesWritten != jsonData.size()) {
        LOG_ERROR("ConfigManager", QString("Failed to write all data to plugin config file: %1").arg(configFile));
        return false;
    }
    
    LOG_INFO("ConfigManager", QString("Saved config for plugin %1 to: %2").arg(pluginId, configFile));
    
    return true;
}

QVariant ConfigManager::getFrameworkValue(const QString& key, const QVariant& defaultValue) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return defaultValue;
    }
    
    return m_frameworkConfig.value(key, defaultValue);
}

void ConfigManager::setFrameworkValue(const QString& key, const QVariant& value)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return;
    }
    
    m_frameworkConfig[key] = value;
    
    emit frameworkConfigChanged(key, value);
}

QVariant ConfigManager::getPluginValue(const QString& pluginId, const QString& key, const QVariant& defaultValue) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return defaultValue;
    }
    
    if (!m_pluginConfigs.contains(pluginId)) {
        return defaultValue;
    }
    
    return m_pluginConfigs[pluginId].value(key, defaultValue);
}

void ConfigManager::setPluginValue(const QString& pluginId, const QString& key, const QVariant& value)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return;
    }
    
    m_pluginConfigs[pluginId][key] = value;
    
    emit pluginConfigChanged(pluginId, key, value);
}

bool ConfigManager::hasFrameworkKey(const QString& key) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return false;
    }
    
    return m_frameworkConfig.contains(key);
}

bool ConfigManager::hasPluginKey(const QString& pluginId, const QString& key) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return false;
    }
    
    if (!m_pluginConfigs.contains(pluginId)) {
        return false;
    }
    
    return m_pluginConfigs[pluginId].contains(key);
}

bool ConfigManager::removeFrameworkKey(const QString& key)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return false;
    }
    
    return m_frameworkConfig.remove(key) > 0;
}

bool ConfigManager::removePluginKey(const QString& pluginId, const QString& key)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return false;
    }
    
    if (!m_pluginConfigs.contains(pluginId)) {
        return false;
    }
    
    return m_pluginConfigs[pluginId].remove(key) > 0;
}

QStringList ConfigManager::getFrameworkKeys() const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return QStringList();
    }
    
    return m_frameworkConfig.keys();
}

QStringList ConfigManager::getPluginKeys(const QString& pluginId) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return QStringList();
    }
    
    if (!m_pluginConfigs.contains(pluginId)) {
        return QStringList();
    }
    
    return m_pluginConfigs[pluginId].keys();
}

QJsonObject ConfigManager::getFrameworkConfigAsJson() const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return QJsonObject();
    }
    
    QJsonObject jsonObj;
    
    // Convert QMap to JSON object
    for (auto it = m_frameworkConfig.begin(); it != m_frameworkConfig.end(); ++it) {
        jsonObj.insert(it.key(), variantToJsonValue(it.value()));
    }
    
    return jsonObj;
}

QJsonObject ConfigManager::getPluginConfigAsJson(const QString& pluginId) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ConfigManager", "Not initialized");
        return QJsonObject();
    }
    
    if (!m_pluginConfigs.contains(pluginId)) {
        return QJsonObject();
    }
    
    QJsonObject jsonObj;
    
    // Convert QMap to JSON object
    const QMap<QString, QVariant>& pluginConfig = m_pluginConfigs[pluginId];
    for (auto it = pluginConfig.begin(); it != pluginConfig.end(); ++it) {
        jsonObj.insert(it.key(), variantToJsonValue(it.value()));
    }
    
    return jsonObj;
}

QJsonValue ConfigManager::variantToJsonValue(const QVariant& value) const
{
    switch (value.type()) {
        case QVariant::Bool:
            return QJsonValue(value.toBool());
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong:
            return QJsonValue(value.toDouble());
        case QVariant::Double:
            return QJsonValue(value.toDouble());
        case QVariant::String:
            return QJsonValue(value.toString());
        case QVariant::StringList:
            {
                QJsonArray array;
                QStringList list = value.toStringList();
                for (const QString& str : list) {
                    array.append(str);
                }
                return array;
            }
        case QVariant::List:
            {
                QJsonArray array;
                QVariantList list = value.toList();
                for (const QVariant& var : list) {
                    array.append(variantToJsonValue(var));
                }
                return array;
            }
        case QVariant::Map:
            {
                QJsonObject obj;
                QVariantMap map = value.toMap();
                for (auto it = map.begin(); it != map.end(); ++it) {
                    obj.insert(it.key(), variantToJsonValue(it.value()));
                }
                return obj;
            }
        default:
            if (value.isNull()) {
                return QJsonValue(QJsonValue::Null);
            } else {
                return QJsonValue(value.toString());
            }
    }
}

QVariant ConfigManager::jsonValueToVariant(const QJsonValue& value) const
{
    switch (value.type()) {
        case QJsonValue::Bool:
            return QVariant(value.toBool());
        case QJsonValue::Double:
            return QVariant(value.toDouble());
        case QJsonValue::String:
            return QVariant(value.toString());
        case QJsonValue::Array:
            {
                QVariantList list;
                QJsonArray array = value.toArray();
                for (const QJsonValue& val : array) {
                    list.append(jsonValueToVariant(val));
                }
                return list;
            }
        case QJsonValue::Object:
            {
                QVariantMap map;
                QJsonObject obj = value.toObject();
                for (auto it = obj.begin(); it != obj.end(); ++it) {
                    map.insert(it.key(), jsonValueToVariant(it.value()));
                }
                return map;
            }
        case QJsonValue::Null:
        case QJsonValue::Undefined:
        default:
            return QVariant();
    }
}