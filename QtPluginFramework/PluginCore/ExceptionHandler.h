#ifndef EXCEPTIONHANDLER_H
#define EXCEPTIONHANDLER_H

#include <QObject>
#include <QString>
#include <QException>
#include <QMutex>
#include <functional>

/**
 * @brief Custom exception class for the plugin framework
 */
class PluginException : public QException
{
public:
    /**
     * @brief Constructor
     * 
     * @param source Source of the exception (e.g., plugin ID or component name)
     * @param message Exception message
     * @param code Error code
     */
    PluginException(const QString& source, const QString& message, int code = 0);
    
    /**
     * @brief Copy constructor
     */
    PluginException(const PluginException& other);
    
    /**
     * @brief Destructor
     */
    ~PluginException() noexcept override;
    
    /**
     * @brief Clone the exception
     * 
     * @return Pointer to a new instance of the exception
     */
    void raise() const override;
    
    /**
     * @brief Clone the exception
     * 
     * @return Pointer to a new instance of the exception
     */
    PluginException* clone() const override;
    
    /**
     * @brief Get the source of the exception
     * 
     * @return Source of the exception
     */
    QString getSource() const;
    
    /**
     * @brief Get the exception message
     * 
     * @return Exception message
     */
    QString getMessage() const;
    
    /**
     * @brief Get the error code
     * 
     * @return Error code
     */
    int getCode() const;
    
private:
    QString m_source;
    QString m_message;
    int m_code;
};

/**
 * @brief The ExceptionHandler class provides centralized exception handling for the framework and plugins.
 * 
 * This class implements the Singleton pattern to ensure a single exception handler
 * instance throughout the application.
 */
class ExceptionHandler : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Type definition for exception handler function
     */
    using ExceptionHandlerFunc = std::function<void(const PluginException&)>;
    
    /**
     * @brief Get the singleton instance of ExceptionHandler
     * 
     * @return Reference to the singleton ExceptionHandler instance
     */
    static ExceptionHandler& instance();

    /**
     * @brief Initialize the exception handler
     * 
     * @return True if initialization was successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Shutdown the exception handler
     */
    void shutdown();

    /**
     * @brief Handle an exception
     * 
     * @param exception The exception to handle
     */
    void handleException(const PluginException& exception);

    /**
     * @brief Register a custom exception handler
     * 
     * @param source Source to register handler for (empty string for global handler)
     * @param handler The handler function
     * @return True if registration was successful, false otherwise
     */
    bool registerExceptionHandler(const QString& source, ExceptionHandlerFunc handler);

    /**
     * @brief Unregister a custom exception handler
     * 
     * @param source Source to unregister handler for
     * @return True if unregistration was successful, false otherwise
     */
    bool unregisterExceptionHandler(const QString& source);

signals:
    /**
     * @brief Signal emitted when an exception is handled
     * 
     * @param source Source of the exception
     * @param message Exception message
     * @param code Error code
     */
    void exceptionHandled(const QString& source, const QString& message, int code);

private:
    // Private constructor for singleton pattern
    ExceptionHandler();
    
    // Deleted copy constructor and assignment operator
    ExceptionHandler(const ExceptionHandler&) = delete;
    ExceptionHandler& operator=(const ExceptionHandler&) = delete;
    
    // Destructor
    ~ExceptionHandler();

    QMap<QString, ExceptionHandlerFunc> m_handlers;
    QMutex m_mutex;
    bool m_initialized;
};

// Convenience macro for throwing a plugin exception
#define THROW_PLUGIN_EXCEPTION(source, message, code) \
    throw PluginException(source, message, code)

#endif // EXCEPTIONHANDLER_H