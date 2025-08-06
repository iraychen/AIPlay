#include "PluginMetadata.h"

PluginMetadata::PluginMetadata() : m_isValid(false)
{
}

PluginMetadata::PluginMetadata(const QJsonObject& metadataJson) : m_metadata(metadataJson)
{
    // Check if the metadata contains all required fields
    m_isValid = !m_metadata.isEmpty() &&
                m_metadata.contains("id") &&
                m_metadata.contains("name") &&
                m_metadata.contains("version") &&
                m_metadata.contains("vendor");
}

bool PluginMetadata::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    return loadFromString(QString::fromUtf8(jsonData));
}

bool PluginMetadata::loadFromString(const QString& jsonString)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        m_isValid = false;
        return false;
    }

    m_metadata = doc.object();
    
    // Check if the metadata contains all required fields
    m_isValid = !m_metadata.isEmpty() &&
                m_metadata.contains("id") &&
                m_metadata.contains("name") &&
                m_metadata.contains("version") &&
                m_metadata.contains("vendor");
    
    return m_isValid;
}

bool PluginMetadata::isValid() const
{
    return m_isValid;
}

QString PluginMetadata::getPluginId() const
{
    return m_metadata.value("id").toString();
}

QString PluginMetadata::getPluginName() const
{
    return m_metadata.value("name").toString();
}

QString PluginMetadata::getPluginVersion() const
{
    return m_metadata.value("version").toString();
}

QVersionNumber PluginMetadata::getVersionNumber() const
{
    return QVersionNumber::fromString(getPluginVersion());
}

QString PluginMetadata::getPluginVendor() const
{
    return m_metadata.value("vendor").toString();
}

QString PluginMetadata::getPluginDescription() const
{
    return m_metadata.value("description").toString();
}

QStringList PluginMetadata::getPluginDependencies() const
{
    QStringList dependencies;
    QJsonArray depsArray = m_metadata.value("dependencies").toArray();
    
    for (const QJsonValue& dep : depsArray) {
        dependencies.append(dep.toString());
    }
    
    return dependencies;
}

QString PluginMetadata::getMinFrameworkVersion() const
{
    return m_metadata.value("minFrameworkVersion").toString("1.0.0");
}

QVersionNumber PluginMetadata::getMinFrameworkVersionNumber() const
{
    return QVersionNumber::fromString(getMinFrameworkVersion());
}

QString PluginMetadata::getCategory() const
{
    return m_metadata.value("category").toString();
}

QString PluginMetadata::getIconPath() const
{
    return m_metadata.value("iconPath").toString();
}

QStringList PluginMetadata::getRequiredPermissions() const
{
    QStringList permissions;
    QJsonArray permsArray = m_metadata.value("requiredPermissions").toArray();
    
    for (const QJsonValue& perm : permsArray) {
        permissions.append(perm.toString());
    }
    
    return permissions;
}

QJsonObject PluginMetadata::getMetadataJson() const
{
    return m_metadata;
}

bool PluginMetadata::isCompatibleWithFramework(const QString& frameworkVersion) const
{
    if (!m_isValid) {
        return false;
    }
    
    QVersionNumber pluginMinVersion = getMinFrameworkVersionNumber();
    QVersionNumber currentFrameworkVersion = QVersionNumber::fromString(frameworkVersion);
    
    return currentFrameworkVersion >= pluginMinVersion;
}

bool PluginMetadata::dependsOn(const QString& pluginId) const
{
    return getPluginDependencies().contains(pluginId);
}