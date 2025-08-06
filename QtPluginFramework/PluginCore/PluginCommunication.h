#ifndef PLUGINCOMMUNICATION_H
#define PLUGINCOMMUNICATION_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QMutex>
#include <QRecursiveMutex>
#include <functional>

/**
 * @brief The PluginCommunication class provides a mechanism for inter-plugin communication.
 * 
 * This class implements the Singleton pattern to ensure a single communication
 * instance throughout the application.
 */
class PluginCommunication : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Type definition for message handler function
     */
    using MessageHandlerFunc = std::function<QVariant(const QString&, const QVariant&)>;
    
    /**
     * @brief Get the singleton instance of PluginCommunication
     * 
     * @return Reference to the singleton PluginCommunication instance
     */
    static PluginCommunication& instance();

    /**
     * @brief Initialize the communication system
     * 
     * @return True if initialization was successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Shutdown the communication system
     */
    void shutdown();

    /**
     * @brief Send a message to a specific plugin
     * 
     * @param sender ID of the sending plugin
     * @param receiver ID of the receiving plugin
     * @param messageType Type of the message
     * @param data Data associated with the message
     * @return Response from the receiver, or an invalid QVariant if no response
     */
    QVariant sendMessage(const QString& sender, const QString& receiver, const QString& messageType, const QVariant& data = QVariant());

    /**
     * @brief Broadcast a message to all plugins
     * 
     * @param sender ID of the sending plugin
     * @param messageType Type of the message
     * @param data Data associated with the message
     * @return Map of plugin IDs to their responses
     */
    QMap<QString, QVariant> broadcastMessage(const QString& sender, const QString& messageType, const QVariant& data = QVariant());

    /**
     * @brief Register a message handler for a plugin
     * 
     * @param pluginId ID of the plugin
     * @param messageType Type of message to handle
     * @param handler The handler function
     * @return True if registration was successful, false otherwise
     */
    bool registerMessageHandler(const QString& pluginId, const QString& messageType, MessageHandlerFunc handler);

    /**
     * @brief Unregister a message handler for a plugin
     * 
     * @param pluginId ID of the plugin
     * @param messageType Type of message to unregister handler for
     * @return True if unregistration was successful, false otherwise
     */
    bool unregisterMessageHandler(const QString& pluginId, const QString& messageType);

    /**
     * @brief Unregister all message handlers for a plugin
     * 
     * @param pluginId ID of the plugin
     * @return True if unregistration was successful, false otherwise
     */
    bool unregisterAllMessageHandlers(const QString& pluginId);

signals:
    /**
     * @brief Signal emitted when a message is sent
     * 
     * @param sender ID of the sending plugin
     * @param receiver ID of the receiving plugin
     * @param messageType Type of the message
     * @param data Data associated with the message
     */
    void messageSent(const QString& sender, const QString& receiver, const QString& messageType, const QVariant& data);

    /**
     * @brief Signal emitted when a message is received
     * 
     * @param receiver ID of the receiving plugin
     * @param sender ID of the sending plugin
     * @param messageType Type of the message
     * @param data Data associated with the message
     * @param response Response to the message
     */
    void messageReceived(const QString& receiver, const QString& sender, const QString& messageType, const QVariant& data, const QVariant& response);

    /**
     * @brief Signal emitted when a message is broadcast
     * 
     * @param sender ID of the sending plugin
     * @param messageType Type of the message
     * @param data Data associated with the message
     */
    void messageBroadcast(const QString& sender, const QString& messageType, const QVariant& data);

private:
    // Private constructor for singleton pattern
    PluginCommunication();
    
    // Deleted copy constructor and assignment operator
    PluginCommunication(const PluginCommunication&) = delete;
    PluginCommunication& operator=(const PluginCommunication&) = delete;
    
    // Destructor
    ~PluginCommunication();

    // Key: pluginId:messageType
    QMap<QString, MessageHandlerFunc> m_handlers;
    mutable QRecursiveMutex m_mutex;
    bool m_initialized;
};

#endif // PLUGINCOMMUNICATION_H