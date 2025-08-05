#include "LogManager.h"

LogManager::LogManager() : m_maxLogLevel(LogLevel::Debug), m_logToConsole(true), m_initialized(false)
{
}

LogManager::~LogManager()
{
    shutdown();
}

LogManager& LogManager::instance()
{
    static LogManager instance;
    return instance;
}

bool LogManager::initialize(const QString& logFilePath, bool logToConsole, LogLevel maxLogLevel)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        // Already initialized, close the current log file first
        shutdown();
    }
    
    m_logFile.setFileName(logFilePath);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qDebug() << "Failed to open log file:" << logFilePath;
        return false;
    }
    
    m_logStream.setDevice(&m_logFile);
    m_maxLogLevel = maxLogLevel;
    m_logToConsole = logToConsole;
    m_initialized = true;
    
    // Log initialization message
    log(LogLevel::Info, "LogManager", "Log system initialized");
    
    return true;
}

void LogManager::shutdown()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        log(LogLevel::Info, "LogManager", "Log system shutting down");
        
        if (m_logFile.isOpen()) {
            m_logStream.flush();
            m_logFile.close();
        }
        
        m_initialized = false;
    }
}

void LogManager::debug(const QString& source, const QString& message)
{
    log(LogLevel::Debug, source, message);
}

void LogManager::info(const QString& source, const QString& message)
{
    log(LogLevel::Info, source, message);
}

void LogManager::warning(const QString& source, const QString& message)
{
    log(LogLevel::Warning, source, message);
}

void LogManager::error(const QString& source, const QString& message)
{
    log(LogLevel::Error, source, message);
}

void LogManager::fatal(const QString& source, const QString& message)
{
    log(LogLevel::Fatal, source, message);
}

void LogManager::setMaxLogLevel(LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_maxLogLevel = level;
}

LogLevel LogManager::getMaxLogLevel() const
{
    return m_maxLogLevel;
}

void LogManager::setConsoleLogging(bool enable)
{
    QMutexLocker locker(&m_mutex);
    m_logToConsole = enable;
}

bool LogManager::isConsoleLoggingEnabled() const
{
    return m_logToConsole;
}

void LogManager::log(LogLevel level, const QString& source, const QString& message)
{
    // Skip if log level is higher than maximum
    if (level < m_maxLogLevel) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    QDateTime timestamp = QDateTime::currentDateTime();
    QString timestampStr = timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = logLevelToString(level);
    QString logMessage = QString("[%1] [%2] [%3]: %4")
                            .arg(timestampStr)
                            .arg(levelStr)
                            .arg(source)
                            .arg(message);
    
    // Write to log file if initialized
    if (m_initialized && m_logFile.isOpen()) {
        m_logStream << logMessage << Qt::endl;
        m_logStream.flush();
    }
    
    // Write to console if enabled
    if (m_logToConsole) {
        switch (level) {
            case LogLevel::Debug:
                qDebug().noquote() << logMessage;
                break;
            case LogLevel::Info:
                qInfo().noquote() << logMessage;
                break;
            case LogLevel::Warning:
                qWarning().noquote() << logMessage;
                break;
            case LogLevel::Error:
            case LogLevel::Fatal:
                qCritical().noquote() << logMessage;
                break;
        }
    }
    
    // Emit signal for log message
    emit logMessageRecorded(level, source, message, timestamp);
}

QString LogManager::logLevelToString(LogLevel level) const
{
    return m_logLevelStrings.value(level, "UNKNOWN");
}