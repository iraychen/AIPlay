#include "PluginManager.h"
#include "ExceptionHandler.h"
#include "LogManager.h"
#include "PluginCommunication.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>

#include <QMutexLocker>
#include <QRecursiveMutex>

PluginManager::PluginManager()
    : m_initialized(false)
{
}

PluginManager::~PluginManager()
{
    shutdown();
}

PluginManager& PluginManager::instance()
{
    static PluginManager instance;
    return instance;
}

bool PluginManager::initialize(const QString& pluginDir, const QString& metadataDir)
{
    QRecursiveMutex locker(&m_mutex);

    if (m_initialized) {
        LOG_WARNING("PluginManager", "Already initialized");
        return true;
    }

    // Create plugin directory if it doesn't exist
    QDir dir(pluginDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            LOG_ERROR("PluginManager", QString("Failed to create plugin directory: %1").arg(pluginDir));
            return false;
        }
    }

    // Create metadata directory if it doesn't exist
    QDir metaDir(metadataDir);
    if (!metaDir.exists()) {
        if (!metaDir.mkpath(".")) {
            LOG_ERROR("PluginManager", QString("Failed to create metadata directory: %1").arg(metadataDir));
            return false;
        }
    }

    m_pluginDir = pluginDir;
    m_metadataDir = metadataDir;
    m_initialized = true;

    LOG_INFO("PluginManager", QString("Initialized with plugin directory: %1, metadata directory: %2").arg(pluginDir, metadataDir));

    return true;
}

void PluginManager::shutdown()
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (m_initialized) {
        LOG_INFO("PluginManager", "Shutting down");

        // Deactivate and unload all plugins in reverse dependency order
        QStringList pluginIds = m_plugins.keys();
        QStringList sortedPluginIds = sortPluginsByDependency(pluginIds);

        // Reverse the order for unloading
        for (int i = 0; i < sortedPluginIds.size() / 2; ++i) {
            sortedPluginIds.swapItemsAt(i, sortedPluginIds.size() - 1 - i);
        }

        for (const QString& pluginId : sortedPluginIds) {
            if (isPluginActive(pluginId)) {
                deactivatePlugin(pluginId);
            }
            unloadPlugin(pluginId);
        }

        m_pluginLoaders.clear();
        m_plugins.clear();
        m_pluginMetadata.clear();
        m_pluginStates.clear();

        m_initialized = false;
    }
}

QStringList PluginManager::scanForPlugins()
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return QStringList();
    }

    QStringList pluginIds;

    // Scan metadata directory for JSON files
    QDir metaDir(m_metadataDir);
    QStringList metadataFiles = metaDir.entryList(QStringList() << "*.json", QDir::Files);

    for (const QString& metadataFile : metadataFiles) {
        QString pluginId = QFileInfo(metadataFile).baseName();

        if (loadPluginMetadata(pluginId)) {
            pluginIds.append(pluginId);
        }
    }

    LOG_INFO("PluginManager", QString("Found %1 plugins").arg(pluginIds.size()));

    return pluginIds;
}

bool PluginManager::loadPlugin(const QString& pluginId)
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return false;
    }

    if (isPluginLoaded(pluginId)) {
        LOG_WARNING("PluginManager", QString("Plugin already loaded: %1").arg(pluginId));
        return true;
    }

    // Load metadata if not already loaded
    if (!m_pluginMetadata.contains(pluginId)) {
        if (!loadPluginMetadata(pluginId)) {
            LOG_ERROR("PluginManager", QString("Failed to load metadata for plugin: %1").arg(pluginId));
            return false;
        }
    }

    // Check if plugin is compatible with framework
    const PluginMetadata& metadata = m_pluginMetadata[pluginId];
    if (!metadata.isCompatibleWithFramework(m_frameworkVersion)) {
        LOG_ERROR("PluginManager", QString("Plugin %1 is not compatible with framework version %2").arg(pluginId, m_frameworkVersion));
        m_pluginStates[pluginId] = PluginState::Failed;
        emit pluginFailed(pluginId, QString("Incompatible with framework version %1").arg(m_frameworkVersion));
        return false;
    }

    // Check dependencies
    if (!checkPluginDependencies(pluginId)) {
        LOG_ERROR("PluginManager", QString("Plugin %1 has unsatisfied dependencies").arg(pluginId));
        m_pluginStates[pluginId] = PluginState::Failed;
        emit pluginFailed(pluginId, "Unsatisfied dependencies");
        return false;
    }

    // Load plugin library
    QString pluginPath = QDir(m_pluginDir).filePath(metadata.getPluginId() + ".dll"); // Windows
    if (!QFile::exists(pluginPath)) {
        pluginPath = QDir(m_pluginDir).filePath("lib" + metadata.getPluginId() + ".so"); // Linux
    }
    if (!QFile::exists(pluginPath)) {
        pluginPath = QDir(m_pluginDir).filePath("lib" + metadata.getPluginId() + ".dylib"); // macOS
    }

    if (!QFile::exists(pluginPath)) {
        LOG_ERROR("PluginManager", QString("Plugin library not found: %1").arg(pluginPath));
        m_pluginStates[pluginId] = PluginState::Failed;
        emit pluginFailed(pluginId, "Plugin library not found");
        return false;
    }

    QPluginLoader* loader = new QPluginLoader(pluginPath);

    if (!loader->load()) {
        LOG_ERROR("PluginManager", QString("Failed to load plugin %1: %2").arg(pluginId, loader->errorString()));
        delete loader;
        m_pluginStates[pluginId] = PluginState::Failed;
        emit pluginFailed(pluginId, QString("Failed to load: %1").arg(loader->errorString()));
        return false;
    }

    QObject* pluginInstance = loader->instance();
    if (!pluginInstance) {
        LOG_ERROR("PluginManager", QString("Failed to get plugin instance for %1: %2").arg(pluginId, loader->errorString()));
        loader->unload();
        delete loader;
        m_pluginStates[pluginId] = PluginState::Failed;
        emit pluginFailed(pluginId, QString("Failed to get instance: %1").arg(loader->errorString()));
        return false;
    }

    IPlugin* plugin = qobject_cast<IPlugin*>(pluginInstance);
    if (!plugin) {
        LOG_ERROR("PluginManager", QString("Plugin %1 does not implement IPlugin interface").arg(pluginId));
        loader->unload();
        delete loader;
        m_pluginStates[pluginId] = PluginState::Failed;
        emit pluginFailed(pluginId, "Does not implement IPlugin interface");
        return false;
    }

    m_pluginLoaders[pluginId] = loader;
    m_plugins[pluginId] = plugin;
    m_pluginStates[pluginId] = PluginState::Loaded;

    LOG_INFO("PluginManager", QString("Loaded plugin: %1").arg(pluginId));

    emit pluginLoaded(pluginId);

    return true;
}

bool PluginManager::unloadPlugin(const QString& pluginId)
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return false;
    }

    if (!isPluginLoaded(pluginId)) {
        LOG_WARNING("PluginManager", QString("Plugin not loaded: %1").arg(pluginId));
        return true;
    }

    // Check if other plugins depend on this one
    QStringList dependentPlugins = getDependentPlugins(pluginId);
    if (!dependentPlugins.isEmpty()) {
        LOG_ERROR("PluginManager", QString("Cannot unload plugin %1 because other plugins depend on it: %2").arg(pluginId, dependentPlugins.join(", ")));
        return false;
    }

    // Deactivate plugin if active
    if (isPluginActive(pluginId)) {
        if (!deactivatePlugin(pluginId)) {
            LOG_ERROR("PluginManager", QString("Failed to deactivate plugin: %1").arg(pluginId));
            return false;
        }
    }

    // Shutdown plugin
    IPlugin* plugin = m_plugins[pluginId];
    if (m_pluginStates[pluginId] == PluginState::Initialized) {
        if (!plugin->shutdown()) {
            LOG_ERROR("PluginManager", QString("Failed to shutdown plugin: %1").arg(pluginId));
            return false;
        }
    }

    // Unregister all message handlers
    PluginCommunication::instance().unregisterAllMessageHandlers(pluginId);

    // Unload plugin
    QPluginLoader* loader = m_pluginLoaders[pluginId];
    if (!loader->unload()) {
        LOG_ERROR("PluginManager", QString("Failed to unload plugin %1: %2").arg(pluginId, loader->errorString()));
        return false;
    }

    delete loader;
    m_pluginLoaders.remove(pluginId);
    m_plugins.remove(pluginId);
    m_pluginStates[pluginId] = PluginState::NotLoaded;

    LOG_INFO("PluginManager", QString("Unloaded plugin: %1").arg(pluginId));

    emit pluginUnloaded(pluginId);

    return true;
}

bool PluginManager::initializePlugin(const QString& pluginId)
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return false;
    }

    if (!isPluginLoaded(pluginId)) {
        LOG_ERROR("PluginManager", QString("Plugin not loaded: %1").arg(pluginId));
        return false;
    }

    if (m_pluginStates[pluginId] == PluginState::Initialized || m_pluginStates[pluginId] == PluginState::Active) {
        LOG_WARNING("PluginManager", QString("Plugin already initialized: %1").arg(pluginId));
        return true;
    }

    if (m_pluginStates[pluginId] == PluginState::Failed) {
        LOG_ERROR("PluginManager", QString("Plugin in failed state: %1").arg(pluginId));
        return false;
    }

    // Initialize dependencies first
    const PluginMetadata& metadata = m_pluginMetadata[pluginId];
    QStringList dependencies = metadata.getPluginDependencies();

    for (const QString& depId : dependencies) {
        if (!isPluginLoaded(depId)) {
            if (!loadPlugin(depId)) {
                LOG_ERROR("PluginManager", QString("Failed to load dependency %1 for plugin %2").arg(depId, pluginId));
                m_pluginStates[pluginId] = PluginState::Failed;
                emit pluginFailed(pluginId, QString("Failed to load dependency: %1").arg(depId));
                return false;
            }
        }

        if (m_pluginStates[depId] != PluginState::Initialized && m_pluginStates[depId] != PluginState::Active) {
            if (!initializePlugin(depId)) {
                LOG_ERROR("PluginManager", QString("Failed to initialize dependency %1 for plugin %2").arg(depId, pluginId));
                m_pluginStates[pluginId] = PluginState::Failed;
                emit pluginFailed(pluginId, QString("Failed to initialize dependency: %1").arg(depId));
                return false;
            }
        }
    }

    // Initialize plugin
    IPlugin* plugin = m_plugins[pluginId];

    try {
        if (!plugin->initialize()) {
            LOG_ERROR("PluginManager", QString("Failed to initialize plugin: %1").arg(pluginId));
            m_pluginStates[pluginId] = PluginState::Failed;
            emit pluginFailed(pluginId, "Failed to initialize");
            return false;
        }
    } catch (const PluginException& ex) {
        LOG_ERROR("PluginManager", QString("Exception during plugin initialization: %1").arg(ex.getMessage()));
        m_pluginStates[pluginId] = PluginState::Failed;
        emit pluginFailed(pluginId, QString("Exception during initialization: %1").arg(ex.getMessage()));
        return false;
    } catch (const std::exception& ex) {
        LOG_ERROR("PluginManager", QString("Exception during plugin initialization: %1").arg(ex.what()));
        m_pluginStates[pluginId] = PluginState::Failed;
        emit pluginFailed(pluginId, QString("Exception during initialization: %1").arg(ex.what()));
        return false;
    } catch (...) {
        LOG_ERROR("PluginManager", "Unknown exception during plugin initialization");
        m_pluginStates[pluginId] = PluginState::Failed;
        emit pluginFailed(pluginId, "Unknown exception during initialization");
        return false;
    }

    m_pluginStates[pluginId] = PluginState::Initialized;

    LOG_INFO("PluginManager", QString("Initialized plugin: %1").arg(pluginId));

    emit pluginInitialized(pluginId);

    return true;
}

bool PluginManager::activatePlugin(const QString& pluginId)
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return false;
    }

    if (!isPluginLoaded(pluginId)) {
        LOG_ERROR("PluginManager", QString("Plugin not loaded: %1").arg(pluginId));
        return false;
    }

    if (m_pluginStates[pluginId] == PluginState::Active) {
        LOG_WARNING("PluginManager", QString("Plugin already active: %1").arg(pluginId));
        return true;
    }

    if (m_pluginStates[pluginId] == PluginState::Failed) {
        LOG_ERROR("PluginManager", QString("Plugin in failed state: %1").arg(pluginId));
        return false;
    }

    // Initialize plugin if not already initialized
    if (m_pluginStates[pluginId] != PluginState::Initialized) {
        if (!initializePlugin(pluginId)) {
            LOG_ERROR("PluginManager", QString("Failed to initialize plugin: %1").arg(pluginId));
            return false;
        }
    }

    // Activate dependencies first
    const PluginMetadata& metadata = m_pluginMetadata[pluginId];
    QStringList dependencies = metadata.getPluginDependencies();

    for (const QString& depId : dependencies) {
        if (m_pluginStates[depId] != PluginState::Active) {
            if (!activatePlugin(depId)) {
                LOG_ERROR("PluginManager", QString("Failed to activate dependency %1 for plugin %2").arg(depId, pluginId));
                return false;
            }
        }
    }

    // Activate plugin
    IPlugin* plugin = m_plugins[pluginId];

    try {
        if (!plugin->activate()) {
            LOG_ERROR("PluginManager", QString("Failed to activate plugin: %1").arg(pluginId));
            m_pluginStates[pluginId] = PluginState::Failed;
            emit pluginFailed(pluginId, "Failed to activate");
            return false;
        }
    } catch (const PluginException& ex) {
        LOG_ERROR("PluginManager", QString("Exception during plugin activation: %1").arg(ex.getMessage()));
        m_pluginStates[pluginId] = PluginState::Failed;
        emit pluginFailed(pluginId, QString("Exception during activation: %1").arg(ex.getMessage()));
        return false;
    } catch (const std::exception& ex) {
        LOG_ERROR("PluginManager", QString("Exception during plugin activation: %1").arg(ex.what()));
        m_pluginStates[pluginId] = PluginState::Failed;
        emit pluginFailed(pluginId, QString("Exception during activation: %1").arg(ex.what()));
        return false;
    } catch (...) {
        LOG_ERROR("PluginManager", "Unknown exception during plugin activation");
        m_pluginStates[pluginId] = PluginState::Failed;
        emit pluginFailed(pluginId, "Unknown exception during activation");
        return false;
    }

    m_pluginStates[pluginId] = PluginState::Active;

    LOG_INFO("PluginManager", QString("Activated plugin: %1").arg(pluginId));

    emit pluginActivated(pluginId);

    return true;
}

bool PluginManager::deactivatePlugin(const QString& pluginId)
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return false;
    }

    if (!isPluginLoaded(pluginId)) {
        LOG_ERROR("PluginManager", QString("Plugin not loaded: %1").arg(pluginId));
        return false;
    }

    if (m_pluginStates[pluginId] != PluginState::Active) {
        LOG_WARNING("PluginManager", QString("Plugin not active: %1").arg(pluginId));
        return true;
    }

    // Check if other active plugins depend on this one
    QStringList dependentPlugins;
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        if (it.key() != pluginId && m_pluginStates[it.key()] == PluginState::Active) {
            const PluginMetadata& metadata = m_pluginMetadata[it.key()];
            if (metadata.dependsOn(pluginId)) {
                dependentPlugins.append(it.key());
            }
        }
    }

    // Deactivate dependent plugins first
    for (const QString& depId : dependentPlugins) {
        if (!deactivatePlugin(depId)) {
            LOG_ERROR("PluginManager", QString("Failed to deactivate dependent plugin %1 for plugin %2").arg(depId, pluginId));
            return false;
        }
    }

    // Deactivate plugin
    IPlugin* plugin = m_plugins[pluginId];

    try {
        if (!plugin->deactivate()) {
            LOG_ERROR("PluginManager", QString("Failed to deactivate plugin: %1").arg(pluginId));
            return false;
        }
    } catch (const PluginException& ex) {
        LOG_ERROR("PluginManager", QString("Exception during plugin deactivation: %1").arg(ex.getMessage()));
        return false;
    } catch (const std::exception& ex) {
        LOG_ERROR("PluginManager", QString("Exception during plugin deactivation: %1").arg(ex.what()));
        return false;
    } catch (...) {
        LOG_ERROR("PluginManager", "Unknown exception during plugin deactivation");
        return false;
    }

    m_pluginStates[pluginId] = PluginState::Initialized;

    LOG_INFO("PluginManager", QString("Deactivated plugin: %1").arg(pluginId));

    emit pluginDeactivated(pluginId);

    return true;
}

IPlugin* PluginManager::getPlugin(const QString& pluginId) const
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return nullptr;
    }

    return m_plugins.value(pluginId, nullptr);
}

QMap<QString, IPlugin*> PluginManager::getLoadedPlugins() const
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return QMap<QString, IPlugin*>();
    }

    return m_plugins;
}

QMap<QString, IPlugin*> PluginManager::getActivePlugins() const
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return QMap<QString, IPlugin*>();
    }

    QMap<QString, IPlugin*> activePlugins;

    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        if (m_pluginStates[it.key()] == PluginState::Active) {
            activePlugins.insert(it.key(), it.value());
        }
    }

    return activePlugins;
}

PluginState PluginManager::getPluginState(const QString& pluginId) const
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return PluginState::NotLoaded;
    }

    return m_pluginStates.value(pluginId, PluginState::NotLoaded);
}

PluginMetadata PluginManager::getPluginMetadata(const QString& pluginId) const
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return PluginMetadata();
    }

    return m_pluginMetadata.value(pluginId, PluginMetadata());
}

QMap<QString, PluginMetadata> PluginManager::getAvailablePlugins() const
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return QMap<QString, PluginMetadata>();
    }

    return m_pluginMetadata;
}

bool PluginManager::isPluginLoaded(const QString& pluginId) const
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return false;
    }

    return m_plugins.contains(pluginId);
}

bool PluginManager::isPluginActive(const QString& pluginId) const
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return false;
    }

    return m_pluginStates.value(pluginId, PluginState::NotLoaded) == PluginState::Active;
}

QVariant PluginManager::executePluginCommand(const QString& pluginId, const QString& command, const QVariantMap& params)
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginManager", "Not initialized");
        return QVariant();
    }

    if (!isPluginLoaded(pluginId)) {
        LOG_ERROR("PluginManager", QString("Plugin not loaded: %1").arg(pluginId));
        return QVariant();
    }

    if (!isPluginActive(pluginId)) {
        LOG_ERROR("PluginManager", QString("Plugin not active: %1").arg(pluginId));
        return QVariant();
    }

    IPlugin* plugin = m_plugins[pluginId];

    try {
        return plugin->executeCommand(command, params);
    } catch (const PluginException& ex) {
        LOG_ERROR("PluginManager", QString("Exception during command execution: %1").arg(ex.getMessage()));
        return QVariant();
    } catch (const std::exception& ex) {
        LOG_ERROR("PluginManager", QString("Exception during command execution: %1").arg(ex.what()));
        return QVariant();
    } catch (...) {
        LOG_ERROR("PluginManager", "Unknown exception during command execution");
        return QVariant();
    }
}

QString PluginManager::getFrameworkVersion() const
{
    return m_frameworkVersion;
}

bool PluginManager::loadPluginMetadata(const QString& pluginId)
{
    QString metadataPath = QDir(m_metadataDir).filePath(pluginId + ".json");

    if (!QFile::exists(metadataPath)) {
        LOG_ERROR("PluginManager", QString("Metadata file not found: %1").arg(metadataPath));
        return false;
    }

    PluginMetadata metadata;
    if (!metadata.loadFromFile(metadataPath)) {
        LOG_ERROR("PluginManager", QString("Failed to load metadata from file: %1").arg(metadataPath));
        return false;
    }

    if (!metadata.isValid()) {
        LOG_ERROR("PluginManager", QString("Invalid metadata for plugin: %1").arg(pluginId));
        return false;
    }

    if (metadata.getPluginId() != pluginId) {
        LOG_ERROR("PluginManager", QString("Metadata ID (%1) does not match plugin ID (%2)").arg(metadata.getPluginId(), pluginId));
        return false;
    }

    m_pluginMetadata[pluginId] = metadata;

    return true;
}

bool PluginManager::checkPluginDependencies(const QString& pluginId)
{
    if (!m_pluginMetadata.contains(pluginId)) {
        LOG_ERROR("PluginManager", QString("No metadata found for plugin: %1").arg(pluginId));
        return false;
    }

    const PluginMetadata& metadata = m_pluginMetadata[pluginId];
    QStringList dependencies = metadata.getPluginDependencies();

    for (const QString& depId : dependencies) {
        // Check if dependency metadata is available
        if (!m_pluginMetadata.contains(depId)) {
            if (!loadPluginMetadata(depId)) {
                LOG_ERROR("PluginManager", QString("Failed to load metadata for dependency: %1").arg(depId));
                return false;
            }
        }

        // Check if dependency is compatible with framework
        const PluginMetadata& depMetadata = m_pluginMetadata[depId];
        if (!depMetadata.isCompatibleWithFramework(m_frameworkVersion)) {
            LOG_ERROR("PluginManager", QString("Dependency %1 is not compatible with framework version %2").arg(depId, m_frameworkVersion));
            return false;
        }
    }

    return true;
}

QStringList PluginManager::getDependentPlugins(const QString& pluginId) const
{
    QStringList dependentPlugins;

    for (auto it = m_pluginMetadata.begin(); it != m_pluginMetadata.end(); ++it) {
        if (it.key() != pluginId && it.value().dependsOn(pluginId)) {
            dependentPlugins.append(it.key());
        }
    }

    return dependentPlugins;
}

QStringList PluginManager::sortPluginsByDependency(const QStringList& pluginIds)
{
    QSet<QString> visited;
    QStringList sortedPlugins;

    for (const QString& pluginId : pluginIds) {
        if (!visited.contains(pluginId)) {
            buildDependencyGraph(pluginId, visited, sortedPlugins);
        }
    }

    return sortedPlugins;
}

void PluginManager::buildDependencyGraph(const QString& pluginId, QSet<QString>& visited, QStringList& sortedPlugins)
{
    visited.insert(pluginId);

    if (m_pluginMetadata.contains(pluginId)) {
        const PluginMetadata& metadata = m_pluginMetadata[pluginId];
        QStringList dependencies = metadata.getPluginDependencies();

        for (const QString& depId : dependencies) {
            if (!visited.contains(depId)) {
                buildDependencyGraph(depId, visited, sortedPlugins);
            }
        }
    }

    sortedPlugins.append(pluginId);
}
