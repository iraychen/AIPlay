#include "SqlServerBackupPlugin.h"
#include "../../PluginCore/LogManager.h"
#include "../../PluginCore/ConfigManager.h"
#include "../../PluginCore/PermissionManager.h"
#include "../../PluginCore/ExceptionHandler.h"

#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

SqlServerBackupPlugin::SqlServerBackupPlugin()
    : m_initialized(false), m_active(false),
      m_serverName("localhost\\SQLEXPRESS"), m_dbName(""),
      m_useWindowsAuth(true), m_username("sa"), m_password(""),
      m_backupDir(""), m_scheduleEnabled(false), m_scheduleInterval(60) // 1 hour
{
    // Load metadata
    QFile metadataFile(":/SqlServerBackup.json");
    if (metadataFile.open(QIODevice::ReadOnly)) {
        QByteArray metadataBytes = metadataFile.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(metadataBytes);
        m_metadata = doc.object();
        metadataFile.close();
    }
    
    // Connect timer signal
    connect(&m_backupTimer, &QTimer::timeout, this, &SqlServerBackupPlugin::performScheduledBackup);
}

SqlServerBackupPlugin::~SqlServerBackupPlugin()
{
    if (m_active) {
        deactivate();
    }
    
    if (m_initialized) {
        shutdown();
    }
}

bool SqlServerBackupPlugin::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    LOG_INFO(getPluginId(), "Initializing SQL Server Backup Plugin");
    
    // Check for required permissions
    if (!PermissionManager::instance().hasPermission(getPluginId(), "file.write")) {
        LOG_ERROR(getPluginId(), "Missing required permission: file.write");
        return false;
    }
    
    if (!PermissionManager::instance().hasPermission(getPluginId(), "database.access")) {
        LOG_ERROR(getPluginId(), "Missing required permission: database.access");
        return false;
    }
    
    // Load configuration
    loadConfig();
    
    m_initialized = true;
    
    LOG_INFO(getPluginId(), "SQL Server Backup Plugin initialized");
    
    return true;
}

bool SqlServerBackupPlugin::activate()
{
    if (!m_initialized) {
        LOG_ERROR(getPluginId(), "Cannot activate: not initialized");
        return false;
    }
    
    if (m_active) {
        return true;
    }
    
    LOG_INFO(getPluginId(), "Activating SQL Server Backup Plugin");
    
    // Start scheduled backups if enabled
    if (m_scheduleEnabled) {
        startScheduledBackups();
    }
    
    m_active = true;
    
    emit statusChanged("SQL Server Backup Plugin activated");
    
    LOG_INFO(getPluginId(), "SQL Server Backup Plugin activated");
    
    return true;
}

bool SqlServerBackupPlugin::deactivate()
{
    if (!m_active) {
        return true;
    }
    
    LOG_INFO(getPluginId(), "Deactivating SQL Server Backup Plugin");
    
    // Stop scheduled backups
    stopScheduledBackups();
    
    m_active = false;
    
    emit statusChanged("SQL Server Backup Plugin deactivated");
    
    LOG_INFO(getPluginId(), "SQL Server Backup Plugin deactivated");
    
    return true;
}

bool SqlServerBackupPlugin::shutdown()
{
    if (!m_initialized) {
        return true;
    }
    
    LOG_INFO(getPluginId(), "Shutting down SQL Server Backup Plugin");
    
    // Deactivate if active
    if (m_active) {
        deactivate();
    }
    
    // Save configuration
    saveConfig();
    
    m_initialized = false;
    
    LOG_INFO(getPluginId(), "SQL Server Backup Plugin shut down");
    
    return true;
}

QString SqlServerBackupPlugin::getPluginId() const
{
    return m_metadata.value("id").toString();
}

QString SqlServerBackupPlugin::getPluginName() const
{
    return m_metadata.value("name").toString();
}

QString SqlServerBackupPlugin::getPluginVersion() const
{
    return m_metadata.value("version").toString();
}

QString SqlServerBackupPlugin::getPluginVendor() const
{
    return m_metadata.value("vendor").toString();
}

QString SqlServerBackupPlugin::getPluginDescription() const
{
    return m_metadata.value("description").toString();
}

QStringList SqlServerBackupPlugin::getPluginDependencies() const
{
    QStringList dependencies;
    QJsonArray depsArray = m_metadata.value("dependencies").toArray();
    
    for (const QJsonValue& dep : depsArray) {
        dependencies.append(dep.toString());
    }
    
    return dependencies;
}

QJsonObject SqlServerBackupPlugin::getPluginMetadata() const
{
    return m_metadata;
}

QVariant SqlServerBackupPlugin::executeCommand(const QString& command, const QVariantMap& params)
{
    if (!m_active) {
        LOG_ERROR(getPluginId(), QString("Cannot execute command: plugin not active - %1").arg(command));
        return QVariant();
    }
    
    LOG_INFO(getPluginId(), QString("Executing command: %1").arg(command));
    
    if (command == "showInfo") {
        // Show plugin information
        QString info = QString("SQL Server Backup Plugin v%1\n\n").arg(getPluginVersion());
        info += QString("Server: %1\n").arg(m_serverName);
        info += QString("Database: %1\n").arg(m_dbName);
        info += QString("Authentication: %1\n").arg(m_useWindowsAuth ? "Windows Authentication" : "SQL Server Authentication");
        if (!m_useWindowsAuth) {
            info += QString("Username: %1\n").arg(m_username);
        }
        info += QString("Backup Directory: %1\n").arg(m_backupDir);
        info += QString("Scheduled Backups: %1\n").arg(m_scheduleEnabled ? "Enabled" : "Disabled");
        
        if (m_scheduleEnabled) {
            info += QString("Backup Interval: %1 minutes\n").arg(m_scheduleInterval);
            info += QString("Last Backup: %1\n").arg(m_lastBackupTime.isValid() ? 
                                                   m_lastBackupTime.toString("yyyy-MM-dd hh:mm:ss") : 
                                                   "Never");
        }
        
        QMessageBox::information(nullptr, "SQL Server Backup Plugin", info);
        
        return true;
    }
    else if (command == "configure") {
        // Configure plugin
        bool ok;
        QString serverName = QInputDialog::getText(nullptr, "SQL Server Backup Configuration",
                                                 "Server Name:", QLineEdit::Normal,
                                                 m_serverName, &ok);
        if (!ok) return false;
        
        QString dbName = QInputDialog::getText(nullptr, "SQL Server Backup Configuration",
                                             "Database Name:", QLineEdit::Normal,
                                             m_dbName, &ok);
        if (!ok) return false;
        
        bool useWindowsAuth = QMessageBox::question(nullptr, "SQL Server Backup Configuration",
                                                  "Use Windows Authentication?",
                                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
        
        QString username = m_username;
        QString password = m_password;
        
        if (!useWindowsAuth) {
            username = QInputDialog::getText(nullptr, "SQL Server Backup Configuration",
                                           "Username:", QLineEdit::Normal,
                                           m_username, &ok);
            if (!ok) return false;
            
            password = QInputDialog::getText(nullptr, "SQL Server Backup Configuration",
                                           "Password:", QLineEdit::Password,
                                           m_password, &ok);
            if (!ok) return false;
        }
        
        QString backupDir = QFileDialog::getExistingDirectory(nullptr, "Select Backup Directory",
                                                            m_backupDir.isEmpty() ? QDir::homePath() : m_backupDir);
        if (backupDir.isEmpty()) return false;
        
        bool scheduleEnabled = QMessageBox::question(nullptr, "SQL Server Backup Configuration",
                                                   "Enable scheduled backups?",
                                                   QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
        
        int scheduleInterval = m_scheduleInterval;
        if (scheduleEnabled) {
            scheduleInterval = QInputDialog::getInt(nullptr, "SQL Server Backup Configuration",
                                                  "Backup Interval (minutes):", m_scheduleInterval,
                                                  1, 10080, 1, &ok); // Max 1 week
            if (!ok) return false;
        }
        
        // Update configuration
        m_serverName = serverName;
        m_dbName = dbName;
        m_useWindowsAuth = useWindowsAuth;
        m_username = username;
        m_password = password;
        m_backupDir = backupDir;
        m_scheduleEnabled = scheduleEnabled;
        m_scheduleInterval = scheduleInterval;
        
        // Save configuration
        saveConfig();
        
        // Update scheduled backups
        if (m_active) {
            stopScheduledBackups();
            if (m_scheduleEnabled) {
                startScheduledBackups();
            }
        }
        
        return true;
    }
    else if (command == "backup") {
        // Perform backup
        QString backupPath = QDir(m_backupDir).filePath(QString("%1_%2.bak")
                                                      .arg(m_dbName)
                                                      .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")));
        
        bool success = performBackup(m_serverName, m_dbName, m_useWindowsAuth, m_username, m_password, backupPath);
        
        if (success) {
            QMessageBox::information(nullptr, "SQL Server Backup", QString("Backup completed successfully:\n%1").arg(backupPath));
        } else {
            QMessageBox::warning(nullptr, "SQL Server Backup", "Backup failed. Check the log for details.");
        }
        
        return success;
    }
    else if (command == "enableSchedule") {
        m_scheduleEnabled = true;
        saveConfig();
        
        if (m_active) {
            startScheduledBackups();
        }
        
        return true;
    }
    else if (command == "disableSchedule") {
        m_scheduleEnabled = false;
        saveConfig();
        
        if (m_active) {
            stopScheduledBackups();
        }
        
        return true;
    }
    else if (command == "setScheduleInterval") {
        if (params.contains("interval")) {
            m_scheduleInterval = params["interval"].toInt();
            saveConfig();
            
            if (m_active && m_scheduleEnabled) {
                stopScheduledBackups();
                startScheduledBackups();
            }
            
            return true;
        }
        
        return false;
    }
    
    LOG_WARNING(getPluginId(), QString("Unknown command: %1").arg(command));
    
    return QVariant();
}

void SqlServerBackupPlugin::performScheduledBackup()
{
    LOG_INFO(getPluginId(), "Performing scheduled backup");
    
    QString backupPath = QDir(m_backupDir).filePath(QString("%1_%2.bak")
                                                  .arg(m_dbName)
                                                  .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")));
    
    bool success = performBackup(m_serverName, m_dbName, m_useWindowsAuth, m_username, m_password, backupPath);
    
    if (success) {
        m_lastBackupTime = QDateTime::currentDateTime();
        LOG_INFO(getPluginId(), QString("Scheduled backup completed: %1").arg(backupPath));
        emit eventOccurred("backup.completed", backupPath);
    } else {
        LOG_ERROR(getPluginId(), "Scheduled backup failed");
        emit eventOccurred("backup.failed", "");
    }
}

bool SqlServerBackupPlugin::performBackup(const QString& serverName, const QString& dbName,
                                        bool useWindowsAuth, const QString& username, const QString& password,
                                        const QString& backupPath)
{
    LOG_INFO(getPluginId(), QString("Backing up database %1 to %2").arg(dbName, backupPath));
    
    // Create backup directory if it doesn't exist
    QFileInfo backupFileInfo(backupPath);
    QDir backupDir = backupFileInfo.dir();
    if (!backupDir.exists()) {
        if (!backupDir.mkpath(".")) {
            LOG_ERROR(getPluginId(), QString("Failed to create backup directory: %1").arg(backupDir.path()));
            return false;
        }
    }
    
    // Connect to SQL Server
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", "SqlServerBackupConnection");
    
    QString connectionString;
    if (useWindowsAuth) {
        connectionString = QString("Driver={SQL Server};Server=%1;Database=%2;Trusted_Connection=Yes;")
                          .arg(serverName, dbName);
    } else {
        connectionString = QString("Driver={SQL Server};Server=%1;Database=%2;Uid=%3;Pwd=%4;")
                          .arg(serverName, dbName, username, password);
    }
    
    db.setDatabaseName(connectionString);
    
    if (!db.open()) {
        LOG_ERROR(getPluginId(), QString("Failed to connect to database: %1").arg(db.lastError().text()));
        return false;
    }
    
    // Execute backup query
    QSqlQuery query(db);
    QString backupQuery = QString("BACKUP DATABASE [%1] TO DISK = N'%2' WITH NOFORMAT, NOINIT, NAME = N'%1-Full Database Backup', SKIP, NOREWIND, NOUNLOAD, STATS = 10")
                         .arg(dbName, backupPath);
    
    if (!query.exec(backupQuery)) {
        LOG_ERROR(getPluginId(), QString("Backup query failed: %1").arg(query.lastError().text()));
        db.close();
        return false;
    }
    
    db.close();
    
    LOG_INFO(getPluginId(), QString("Backup completed: %1").arg(backupPath));
    
    return true;
}

void SqlServerBackupPlugin::loadConfig()
{
    LOG_INFO(getPluginId(), "Loading configuration");
    
    // Load plugin configuration
    QString configDir = QCoreApplication::applicationDirPath() + "/config";
    QString configFile = QDir(configDir).filePath(getPluginId() + ".json");
    
    if (QFile::exists(configFile)) {
        if (ConfigManager::instance().loadPluginConfig(getPluginId(), configFile)) {
            m_serverName = ConfigManager::instance().getPluginValue(getPluginId(), "serverName", m_serverName).toString();
            m_dbName = ConfigManager::instance().getPluginValue(getPluginId(), "dbName", m_dbName).toString();
            m_useWindowsAuth = ConfigManager::instance().getPluginValue(getPluginId(), "useWindowsAuth", m_useWindowsAuth).toBool();
            m_username = ConfigManager::instance().getPluginValue(getPluginId(), "username", m_username).toString();
            m_password = ConfigManager::instance().getPluginValue(getPluginId(), "password", m_password).toString();
            m_backupDir = ConfigManager::instance().getPluginValue(getPluginId(), "backupDir", m_backupDir).toString();
            m_scheduleEnabled = ConfigManager::instance().getPluginValue(getPluginId(), "scheduleEnabled", m_scheduleEnabled).toBool();
            m_scheduleInterval = ConfigManager::instance().getPluginValue(getPluginId(), "scheduleInterval", m_scheduleInterval).toInt();
            
            LOG_INFO(getPluginId(), "Configuration loaded");
        } else {
            LOG_WARNING(getPluginId(), "Failed to load configuration, using defaults");
        }
    } else {
        LOG_INFO(getPluginId(), "No configuration file found, using defaults");
    }
    
    // Set default backup directory if not configured
    if (m_backupDir.isEmpty()) {
        m_backupDir = QDir(QCoreApplication::applicationDirPath()).filePath("backups/sqlserver");
        QDir().mkpath(m_backupDir);
    }
}

void SqlServerBackupPlugin::saveConfig()
{
    LOG_INFO(getPluginId(), "Saving configuration");
    
    // Save plugin configuration
    ConfigManager::instance().setPluginValue(getPluginId(), "serverName", m_serverName);
    ConfigManager::instance().setPluginValue(getPluginId(), "dbName", m_dbName);
    ConfigManager::instance().setPluginValue(getPluginId(), "useWindowsAuth", m_useWindowsAuth);
    ConfigManager::instance().setPluginValue(getPluginId(), "username", m_username);
    ConfigManager::instance().setPluginValue(getPluginId(), "password", m_password);
    ConfigManager::instance().setPluginValue(getPluginId(), "backupDir", m_backupDir);
    ConfigManager::instance().setPluginValue(getPluginId(), "scheduleEnabled", m_scheduleEnabled);
    ConfigManager::instance().setPluginValue(getPluginId(), "scheduleInterval", m_scheduleInterval);
    
    QString configDir = QCoreApplication::applicationDirPath() + "/config";
    QString configFile = QDir(configDir).filePath(getPluginId() + ".json");
    
    if (ConfigManager::instance().savePluginConfig(getPluginId(), configFile)) {
        LOG_INFO(getPluginId(), "Configuration saved");
    } else {
        LOG_ERROR(getPluginId(), "Failed to save configuration");
    }
}

void SqlServerBackupPlugin::startScheduledBackups()
{
    if (!m_scheduleEnabled) {
        return;
    }
    
    LOG_INFO(getPluginId(), QString("Starting scheduled backups with interval %1 minutes").arg(m_scheduleInterval));
    
    // Start timer
    m_backupTimer.start(m_scheduleInterval * 60 * 1000); // Convert minutes to milliseconds
    
    emit statusChanged(QString("SQL Server Backup scheduled every %1 minutes").arg(m_scheduleInterval));
}

void SqlServerBackupPlugin::stopScheduledBackups()
{
    if (m_backupTimer.isActive()) {
        LOG_INFO(getPluginId(), "Stopping scheduled backups");
        
        m_backupTimer.stop();
        
        emit statusChanged("SQL Server Backup schedule stopped");
    }
}