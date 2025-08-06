#include "PluginCommunication.h"
#include "LogManager.h"
#include "PermissionManager.h"

#include <QRecursiveMutexLocker>

PluginCommunication::PluginCommunication()
    : m_initialized(false)
{
}

PluginCommunication::~PluginCommunication()
{
    shutdown();
}

PluginCommunication& PluginCommunication::instance()
{
    static PluginCommunication instance;
    return instance;
}

bool PluginCommunication::initialize()
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (m_initialized) {
        LOG_WARNING("PluginCommunication", "Already initialized");
        return true;
    }

    m_initialized = true;

    LOG_INFO("PluginCommunication", "Initialized");

    return true;
}

void PluginCommunication::shutdown()
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (m_initialized) {
        LOG_INFO("PluginCommunication", "Shutting down");

        m_handlers.clear();

        m_initialized = false;
    }
}

QVariant PluginCommunication::sendMessage(const QString& sender, const QString& receiver, const QString& messageType, const QVariant& data)
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginCommunication", "Not initialized");
        return QVariant();
    }

    // Check if sender has permission to send messages
    if (!PermissionManager::instance().hasPermission(sender, "communication.send")) {
        LOG_WARNING("PluginCommunication", QString("Plugin %1 does not have permission to send messages").arg(sender));
        return QVariant();
    }

    // Check if receiver has permission to receive messages
    if (!PermissionManager::instance().hasPermission(receiver, "communication.receive")) {
        LOG_WARNING("PluginCommunication", QString("Plugin %1 does not have permission to receive messages").arg(receiver));
        return QVariant();
    }

    QString handlerKey = QString("%1:%2").arg(receiver, messageType);

    if (!m_handlers.contains(handlerKey)) {
        LOG_WARNING("PluginCommunication", QString("No handler registered for message type %1 in plugin %2").arg(messageType, receiver));
        return QVariant();
    }

    LOG_DEBUG("PluginCommunication", QString("Sending message from %1 to %2: %3").arg(sender, receiver, messageType));

    emit messageSent(sender, receiver, messageType, data);

    QVariant response = m_handlers[handlerKey](sender, data);

    emit messageReceived(receiver, sender, messageType, data, response);

    return response;
}

QMap<QString, QVariant> PluginCommunication::broadcastMessage(const QString& sender, const QString& messageType, const QVariant& data)
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginCommunication", "Not initialized");
        return QMap<QString, QVariant>();
    }

    // Check if sender has permission to broadcast messages
    if (!PermissionManager::instance().hasPermission(sender, "communication.broadcast")) {
        LOG_WARNING("PluginCommunication", QString("Plugin %1 does not have permission to broadcast messages").arg(sender));
        return QMap<QString, QVariant>();
    }

    QMap<QString, QVariant> responses;

    // Find all handlers for this message type
    QStringList handlerKeys = m_handlers.keys();

    LOG_DEBUG("PluginCommunication", QString("Broadcasting message from %1: %2").arg(sender, messageType));

    emit messageBroadcast(sender, messageType, data);

    for (const QString& handlerKey : handlerKeys) {
        QStringList parts = handlerKey.split(':');
        if (parts.size() == 2 && parts[1] == messageType) {
            QString receiver = parts[0];

            // Check if receiver has permission to receive messages
            if (!PermissionManager::instance().hasPermission(receiver, "communication.receive")) {
                LOG_WARNING("PluginCommunication", QString("Plugin %1 does not have permission to receive messages").arg(receiver));
                continue;
            }

            QVariant response = m_handlers[handlerKey](sender, data);
            responses.insert(receiver, response);

            emit messageReceived(receiver, sender, messageType, data, response);
        }
    }

    return responses;
}

bool PluginCommunication::registerMessageHandler(const QString& pluginId, const QString& messageType, MessageHandlerFunc handler)
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginCommunication", "Not initialized");
        return false;
    }

    QString handlerKey = QString("%1:%2").arg(pluginId, messageType);

    if (m_handlers.contains(handlerKey)) {
        LOG_WARNING("PluginCommunication", QString("Handler already registered for message type %1 in plugin %2").arg(messageType, pluginId));
        return false;
    }

    m_handlers.insert(handlerKey, handler);

    LOG_INFO("PluginCommunication", QString("Registered handler for message type %1 in plugin %2").arg(messageType, pluginId));

    return true;
}

bool PluginCommunication::unregisterMessageHandler(const QString& pluginId, const QString& messageType)
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginCommunication", "Not initialized");
        return false;
    }

    QString handlerKey = QString("%1:%2").arg(pluginId, messageType);

    if (!m_handlers.contains(handlerKey)) {
        LOG_WARNING("PluginCommunication", QString("No handler registered for message type %1 in plugin %2").arg(messageType, pluginId));
        return false;
    }

    m_handlers.remove(handlerKey);

    LOG_INFO("PluginCommunication", QString("Unregistered handler for message type %1 in plugin %2").arg(messageType, pluginId));

    return true;
}

bool PluginCommunication::unregisterAllMessageHandlers(const QString& pluginId)
{
    QRecursiveMutexLocker locker(&m_mutex);

    if (!m_initialized) {
        LOG_ERROR("PluginCommunication", "Not initialized");
        return false;
    }

    QStringList keysToRemove;

    for (auto it = m_handlers.begin(); it != m_handlers.end(); ++it) {
        QStringList parts = it.key().split(':');
        if (parts.size() == 2 && parts[0] == pluginId) {
            keysToRemove.append(it.key());
        }
    }

    for (const QString& key : keysToRemove) {
        m_handlers.remove(key);
    }

    LOG_INFO("PluginCommunication", QString("Unregistered all handlers for plugin %1").arg(pluginId));

    return true;
}
