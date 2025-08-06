#include "MySqlBackupPlugin.h"
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

MySqlBackupPlugin::MySqlBackupPlugin()
    : m_initialized(false), m_active(false),
      m_dbHost("localhost"), m_dbPort(3306), m_dbName(""),
      m_dbUser("root"), m_dbPassword(""), m_backupDir(""),
      m_scheduleEnabled(false), m_scheduleInterval(60) // 1 hour
{
    // Load metadata
    QFile metadataFile(":/MySqlBackup.json");
    if (metadataFile.open(QIODevice::ReadOnly)) {
        QByteArray metadataBytes = metadataFile.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(metadataBytes);
        m_metadata = doc.object();
        metadataFile.close();
    }
    
    // Connect timer signal
    connect(&m_backupTimer, &QTimer::timeout, this, &MySqlBackupPlugin::performScheduledBackup);
}

MySqlBackupPlugin::~MySqlBackupPlugin()
{
    if (m_active) {
        deactivate();
    }
    
    if (m_initialized) {
        shutdown();
    }
}

bool MySqlBackupPlugin::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    LOG_INFO(getPluginId(), "Initializing MySQL Backup Plugin");
    
    // Check for required permissions
    if (!PermissionManager::instance().hasPermission(getPluginId(), "file.write")) {
        LOG_ERROR(getPluginId(), "Missing required permission: file.write");
        return false;
    }
    
    if (!PermissionManager::instance().hasPermission(getPluginId(), "system.execute")) {
        LOG_ERROR(getPluginId(), "Missing required permission: system.execute");
        return false;
    }
    
    // Load configuration
    loadConfig();
    
    m_initialized = true;
    
    LOG_INFO(getPluginId(), "MySQL Backup Plugin initialized");
    
    return true;
}

bool MySqlBackupPlugin::activate()
{
    if (!m_initialized) {
        LOG_ERROR(getPluginId(), "Cannot activate: not initialized");
        return false;
    }
    
    if (m_active) {
        return true;
    }
    
    LOG_INFO(getPluginId(), "Activating MySQL Backup Plugin");
    
    // Start scheduled backups if enabled
    if (m_scheduleEnabled) {
        startScheduledBackups();
    }
    
    m_active = true;
    
    emit statusChanged("MySQL Backup Plugin activated");
    
    LOG_INFO(getPluginId(), "MySQL Backup Plugin activated");
    
    return true;
}

bool MySqlBackupPlugin::deactivate()
{
    if (!m_active) {
        return true;
    }
    
    LOG_INFO(getPluginId(), "Deactivating MySQL Backup Plugin");
    
    // Stop scheduled backups
    stopScheduledBackups();
    
    m_active = false;
    
    emit statusChanged("MySQL Backup Plugin deactivated");
    
    LOG_INFO(getPluginId(), "MySQL Backup Plugin deactivated");
    
    return true;
}

bool MySqlBackupPlugin::shutdown()
{
    if (!m_initialized) {
        return true;
    }
    
    LOG_INFO(getPluginId(), "Shutting down MySQL Backup Plugin");
    
    // Deactivate if active
    if (m_active) {
        deactivate();
    }
    
    // Save configuration
    saveConfig();
    
    m_initialized = false;
    
    LOG_INFO(getPluginId(), "MySQL Backup Plugin shut down");
    
    return true;
}

QString MySqlBackupPlugin::getPluginId() const
{
    return m_metadata.value("id").toString();
}

QString MySqlBackupPlugin::getPluginName() const
{
    return m_metadata.value("name").toString();
}

QString MySqlBackupPlugin::getPluginVersion() const
{
    return m_metadata.value("version").toString();
}

QString MySqlBackupPlugin::getPluginVendor() const
{
    return m_metadata.value("vendor").toString();
}

QString MySqlBackupPlugin::getPluginDescription() const
{
    return m_metadata.value("description").toString();
}

QStringList MySqlBackupPlugin::getPluginDependencies() const
{
    QStringList dependencies;
    QJsonArray depsArray = m_metadata.value("dependencies").toArray();
    
    for (const QJsonValue& dep : depsArray) {
        dependencies.append(dep.toString());
    }
    
    return dependencies;
}

QJsonObject MySqlBackupPlugin::getPluginMetadata() const
{
    return m_metadata;
}

QVariant MySqlBackupPlugin::executeCommand(const QString& command, const QVariantMap& params)
{
    if (!m_active) {
        LOG_ERROR(getPluginId(), QString("Cannot execute command: plugin not active - %1").arg(command));
        return QVariant();
    }
    
    LOG_INFO(getPluginId(), QString("Executing command: %1").arg(command));
    
    if (command == "showInfo") {
        // Show plugin information
        QString info = QString("MySQL Backup Plugin v%1\n\n").arg(getPluginVersion());
        info += QString("Database: %1:%2/%3\n").arg(m_dbHost).arg(m_dbPort).arg(m_dbName);
        info += QString("Backup Directory: %1\n").arg(m_backupDir);
        info += QString("Scheduled Backups: %1\n").arg(m_scheduleEnabled ? "Enabled" : "Disabled");
        
        if (m_scheduleEnabled) {
            info += QString("Backup Interval: %1 minutes\n").arg(m_scheduleInterval);
            info += QString("Last Backup: %1\n").arg(m_lastBackupTime.isValid() ? 
                                                   m_lastBackupTime.toString("yyyy-MM-dd hh:mm:ss") : 
                                                   "Never");
        }
        
        QMessageBox::information(nullptr, "MySQL Backup Plugin", info);
        
        return true;
    }
    else if (command == "configure") {
        // Configure plugin
        bool ok;
        QString host = QInputDialog::getText(nullptr, "MySQL Backup Configuration",
                                           "Database Host:", QLineEdit::Normal,
                                           m_dbHost, &ok);
        if (!ok) return false;
        
        int port = QInputDialog::getInt(nullptr, "MySQL Backup Configuration",
                                      "Database Port:", m_dbPort, 1, 65535, 1, &ok);
        if (!ok) return false;
        
        QString name = QInputDialog::getText(nullptr, "MySQL Backup Configuration",
                                           "Database Name:", QLineEdit::Normal,
                                           m_dbName, &ok);
        if (!ok) return false;
        
        QString user = QInputDialog::getText(nullptr, "MySQL Backup Configuration",
                                           "Database User:", QLineEdit::Normal,
                                           m_dbUser, &ok);
        if (!ok) return false;
        
        QString password = QInputDialog::getText(nullptr, "MySQL Backup Configuration",
                                               "Database Password:", QLineEdit::Password,
                                               m_dbPassword, &ok);
        if (!ok) return false;
        
        QString backupDir = QFileDialog::getExistingDirectory(nullptr, "Select Backup Directory",
                                                            m_backupDir.isEmpty() ? QDir::homePath() : m_backupDir);
        if (backupDir.isEmpty()) return false;
        
        bool scheduleEnabled = QMessageBox::question(nullptr, "MySQL Backup Configuration",
                                                   "Enable scheduled backups?",
                                                   QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
        
        int scheduleInterval = m_scheduleInterval;
        if (scheduleEnabled) {
            scheduleInterval = QInputDialog::getInt(nullptr, "MySQL Backup Configuration",
                                                  "Backup Interval (minutes):", m_scheduleInterval,
                                                  1, 10080, 1, &ok); // Max 1 week
            if (!ok) return false;
        }
        
        // Update configuration
        m_dbHost = host;
        m_dbPort = port;
        m_dbName = name;
        m_dbUser = user;
        m_dbPassword = password;
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
        QString backupPath = QDir(m_backupDir).filePath(QString("%1_%2.sql")
                                                      .arg(m_dbName)
                                                      .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")));
        
        bool success = performBackup(m_dbHost, m_dbPort, m_dbName, m_dbUser, m_dbPassword, backupPath);
        
        if (success) {
            QMessageBox::information(nullptr, "MySQL Backup", QString("Backup completed successfully:\n%1").arg(backupPath));
        } else {
            QMessageBox::warning(nullptr, "MySQL Backup", "Backup failed. Check the log for details.");
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

void MySqlBackupPlugin::performScheduledBackup()
{
    LOG_INFO(getPluginId(), "Performing scheduled backup");
    
    QString backupPath = QDir(m_backupDir).filePath(QString("%1_%2.sql")
                                                  .arg(m_dbName)
                                                  .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")));
    
    bool success = performBackup(m_dbHost, m_dbPort, m_dbName, m_dbUser, m_dbPassword, backupPath);
    
    if (success) {
        m_lastBackupTime = QDateTime::currentDateTime();
        LOG_INFO(getPluginId(), QString("Scheduled backup completed: %1").arg(backupPath));
        emit eventOccurred("backup.completed", backupPath);
    } else {
        LOG_ERROR(getPluginId(), "Scheduled backup failed");
        emit eventOccurred("backup.failed", "");
    }
}

bool MySqlBackupPlugin::performBackup(const QString& dbHost, int dbPort, const QString& dbName,
                                    const QString& dbUser, const QString& dbPassword,
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
    
    // Build mysqldump command
    QStringList args;
    args << "--host=" + dbHost;
    args << QString("--port=%1").arg(dbPort);
    args << "--user=" + dbUser;
    
    if (!dbPassword.isEmpty()) {
        args << "--password=" + dbPassword;
    }
    
    args << "--result-file=" + backupPath;
    args << "--databases" << dbName;
    args << "--add-drop-database";
    args << "--add-drop-table";
    args << "--comments";
    args << "--complete-insert";
    
    // Execute mysqldump
    QProcess process;
    process.start("mysqldump", args);
    
    if (!process.waitForStarted()) {
        LOG_ERROR(getPluginId(), "Failed to start mysqldump process");
        return false;
    }
    
    if (!process.waitForFinished(300000)) { // 5 minutes timeout
        LOG_ERROR(getPluginId(), "mysqldump process timed out");
        process.kill();
        return false;
    }
    
    if (process.exitCode() != 0) {
        LOG_ERROR(getPluginId(), QString("mysqldump process failed: %1").arg(QString(process.readAllStandardError())));
        return false;
    }
    
    LOG_INFO(getPluginId(), QString("Backup completed: %1").arg(backupPath));
    
    return true;
}

void MySqlBackupPlugin::loadConfig()
{
    LOG_INFO(getPluginId(), "Loading configuration");
    
    // Load plugin configuration
    QString configDir = QCoreApplication::applicationDirPath() + "/config";
    QString configFile = QDir(configDir).filePath(getPluginId() + ".json");
    
    if (QFile::exists(configFile)) {
        if (ConfigManager::instance().loadPluginConfig(getPluginId(), configFile)) {
            m_dbHost = ConfigManager::instance().getPluginValue(getPluginId(), "dbHost", m_dbHost).toString();
            m_dbPort = ConfigManager::instance().getPluginValue(getPluginId(), "dbPort", m_dbPort).toInt();
            m_dbName = ConfigManager::instance().getPluginValue(getPluginId(), "dbName", m_dbName).toString();
            m_dbUser = ConfigManager::instance().getPluginValue(getPluginId(), "dbUser", m_dbUser).toString();
            m_dbPassword = ConfigManager::instance().getPluginValue(getPluginId(), "dbPassword", m_dbPassword).toString();
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
        m_backupDir = QDir(QCoreApplication::applicationDirPath()).filePath("backups/mysql");
        QDir().mkpath(m_backupDir);
    }
}

void MySqlBackupPlugin::saveConfig()
{
    LOG_INFO(getPluginId(), "Saving configuration");
    
    // Save plugin configuration
    ConfigManager::instance().setPluginValue(getPluginId(), "dbHost", m_dbHost);
    ConfigManager::instance().setPluginValue(getPluginId(), "dbPort", m_dbPort);
    ConfigManager::instance().setPluginValue(getPluginId(), "dbName", m_dbName);
    ConfigManager::instance().setPluginValue(getPluginId(), "dbUser", m_dbUser);
    ConfigManager::instance().setPluginValue(getPluginId(), "dbPassword", m_dbPassword);
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

void MySqlBackupPlugin::startScheduledBackups()
{
    if (!m_scheduleEnabled) {
        return;
    }
    
    LOG_INFO(getPluginId(), QString("Starting scheduled backups with interval %1 minutes").arg(m_scheduleInterval));
    
    // Start timer
    m_backupTimer.start(m_scheduleInterval * 60 * 1000); // Convert minutes to milliseconds
    
    emit statusChanged(QString("MySQL Backup scheduled every %1 minutes").arg(m_scheduleInterval));
}

void MySqlBackupPlugin::stopScheduledBackups()
{
    if (m_backupTimer.isActive()) {
        LOG_INFO(getPluginId(), "Stopping scheduled backups");
        
        m_backupTimer.stop();
        
        emit statusChanged("MySQL Backup schedule stopped");
    }
}