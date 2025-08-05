#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QMap>
#include <QDebug>

/**
 * @brief Enumeration of log levels
 */
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

/**
 * @brief The LogManager class provides centralized logging for the framework and plugins.
 * 
 * This class implements the Singleton pattern to ensure a single logging instance
 * throughout the application.
 */
class LogManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance of LogManager
     * 
     * @return Reference to the singleton LogManager instance
     */
    static LogManager& instance();

    /**
     * @brief Initialize the log manager
     * 
     * @param logFilePath Path to the log file
     * @param logToConsole Whether to also log to the console
     * @param maxLogLevel Maximum log level to record
     * @return True if initialization was successful, false otherwise
     */
    bool initialize(const QString& logFilePath, bool logToConsole = true, LogLevel maxLogLevel = LogLevel::Debug);

    /**
     * @brief Shutdown the log manager
     */
    void shutdown();

    /**
     * @brief Log a debug message
     * 
     * @param source Source of the log message (e.g., plugin ID or component name)
     * @param message The message to log
     */
    void debug(const QString& source, const QString& message);

    /**
     * @brief Log an info message
     * 
     * @param source Source of the log message (e.g., plugin ID or component name)
     * @param message The message to log
     */
    void info(const QString& source, const QString& message);

    /**
     * @brief Log a warning message
     * 
     * @param source Source of the log message (e.g., plugin ID or component name)
     * @param message The message to log
     */
    void warning(const QString& source, const QString& message);

    /**
     * @brief Log an error message
     * 
     * @param source Source of the log message (e.g., plugin ID or component name)
     * @param message The message to log
     */
    void error(const QString& source, const QString& message);

    /**
     * @brief Log a fatal message
     * 
     * @param source Source of the log message (e.g., plugin ID or component name)
     * @param message The message to log
     */
    void fatal(const QString& source, const QString& message);

    /**
     * @brief Set the maximum log level
     * 
     * @param level The maximum log level to record
     */
    void setMaxLogLevel(LogLevel level);

    /**
     * @brief Get the maximum log level
     * 
     * @return The current maximum log level
     */
    LogLevel getMaxLogLevel() const;

    /**
     * @brief Enable or disable console logging
     * 
     * @param enable True to enable console logging, false to disable
     */
    void setConsoleLogging(bool enable);

    /**
     * @brief Check if console logging is enabled
     * 
     * @return True if console logging is enabled, false otherwise
     */
    bool isConsoleLoggingEnabled() const;

signals:
    /**
     * @brief Signal emitted when a log message is recorded
     * 
     * @param level The log level
     * @param source Source of the log message
     * @param message The log message
     * @param timestamp The timestamp of the log message
     */
    void logMessageRecorded(LogLevel level, const QString& source, const QString& message, const QDateTime& timestamp);

private:
    // Private constructor for singleton pattern
    LogManager();
    
    // Deleted copy constructor and assignment operator
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;
    
    // Destructor
    ~LogManager();

    /**
     * @brief Log a message with the specified level
     * 
     * @param level The log level
     * @param source Source of the log message
     * @param message The message to log
     */
    void log(LogLevel level, const QString& source, const QString& message);

    /**
     * @brief Convert a log level to its string representation
     * 
     * @param level The log level
     * @return String representation of the log level
     */
    QString logLevelToString(LogLevel level) const;

    QFile m_logFile;
    QTextStream m_logStream;
    QMutex m_mutex;
    LogLevel m_maxLogLevel;
    bool m_logToConsole;
    bool m_initialized;
    
    // Map of log levels to their string representations
    const QMap<LogLevel, QString> m_logLevelStrings = {
        {LogLevel::Debug, "DEBUG"},
        {LogLevel::Info, "INFO"},
        {LogLevel::Warning, "WARNING"},
        {LogLevel::Error, "ERROR"},
        {LogLevel::Fatal, "FATAL"}
    };
};

// Convenience macros for logging
#define LOG_DEBUG(source, message) LogManager::instance().debug(source, message)
#define LOG_INFO(source, message) LogManager::instance().info(source, message)
#define LOG_WARNING(source, message) LogManager::instance().warning(source, message)
#define LOG_ERROR(source, message) LogManager::instance().error(source, message)
#define LOG_FATAL(source, message) LogManager::instance().fatal(source, message)

#endif // LOGMANAGER_H