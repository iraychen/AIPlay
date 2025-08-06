#include "ExceptionHandler.h"
#include <QRecursiveMutexLocker>
#include "LogManager.h"
#include <QRecursiveMutexLocker>

// PluginException implementation
PluginException::PluginException(const QString& source, const QString& message, int code)
    : m_source(source), m_message(message), m_code(code)
{
}

PluginException::PluginException(const PluginException& other)
    : QException(other), m_source(other.m_source), m_message(other.m_message), m_code(other.m_code)
{
}

PluginException::~PluginException() noexcept
{
}

void PluginException::raise() const
{
    throw *this;
}

PluginException* PluginException::clone() const
{
    return new PluginException(*this);
}

QString PluginException::getSource() const
{
    return m_source;
}

QString PluginException::getMessage() const
{
    return m_message;
}

int PluginException::getCode() const
{
    return m_code;
}

// ExceptionHandler implementation
ExceptionHandler::ExceptionHandler() : m_initialized(false)
{
}

ExceptionHandler::~ExceptionHandler()
{
    shutdown();
}

ExceptionHandler& ExceptionHandler::instance()
{
    static ExceptionHandler instance;
    return instance;
}

bool ExceptionHandler::initialize()
{
    QRecursiveMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        LOG_WARNING("ExceptionHandler", "Already initialized");
        return true;
    }
    
    // Register default global exception handler
    registerExceptionHandler("", [](const PluginException& exception) {
        LOG_ERROR("ExceptionHandler", QString("Unhandled exception from %1: %2 (code: %3)")
                 .arg(exception.getSource())
                 .arg(exception.getMessage())
                 .arg(exception.getCode()));
    });
    
    m_initialized = true;
    
    LOG_INFO("ExceptionHandler", "Initialized");
    
    return true;
}

void ExceptionHandler::shutdown()
{
    QRecursiveMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        LOG_INFO("ExceptionHandler", "Shutting down");
        
        m_handlers.clear();
        
        m_initialized = false;
    }
}

void ExceptionHandler::handleException(const PluginException& exception)
{
    QRecursiveMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        // If not initialized, just log the exception
        qCritical() << "Exception from" << exception.getSource() << ":" << exception.getMessage() << "(code:" << exception.getCode() << ")";
        return;
    }
    
    // Check if there's a specific handler for this source
    if (m_handlers.contains(exception.getSource())) {
        m_handlers[exception.getSource()](exception);
    }
    // Otherwise use the global handler
    else if (m_handlers.contains("")) {
        m_handlers[""](exception);
    }
    // If no handler is available, just log the exception
    else {
        LOG_ERROR("ExceptionHandler", QString("Unhandled exception from %1: %2 (code: %3)")
                 .arg(exception.getSource())
                 .arg(exception.getMessage())
                 .arg(exception.getCode()));
    }
    
    emit exceptionHandled(exception.getSource(), exception.getMessage(), exception.getCode());
}

bool ExceptionHandler::registerExceptionHandler(const QString& source, ExceptionHandlerFunc handler)
{
    QRecursiveMutexLocker locker(&m_mutex);
    
    if (!m_initialized && source != "") {
        LOG_ERROR("ExceptionHandler", "Not initialized");
        return false;
    }
    
    if (m_handlers.contains(source)) {
        LOG_WARNING("ExceptionHandler", QString("Handler already registered for source: %1").arg(source.isEmpty() ? "global" : source));
        return false;
    }
    
    m_handlers.insert(source, handler);
    
    LOG_INFO("ExceptionHandler", QString("Registered handler for source: %1").arg(source.isEmpty() ? "global" : source));
    
    return true;
}

bool ExceptionHandler::unregisterExceptionHandler(const QString& source)
{
    QRecursiveMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        LOG_ERROR("ExceptionHandler", "Not initialized");
        return false;
    }
    
    if (!m_handlers.contains(source)) {
        LOG_WARNING("ExceptionHandler", QString("No handler registered for source: %1").arg(source.isEmpty() ? "global" : source));
        return false;
    }
    
    m_handlers.remove(source);
    
    LOG_INFO("ExceptionHandler", QString("Unregistered handler for source: %1").arg(source.isEmpty() ? "global" : source));
    
    return true;
}