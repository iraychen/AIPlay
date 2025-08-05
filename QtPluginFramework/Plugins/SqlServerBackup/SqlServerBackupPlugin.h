#ifndef SQLSERVERBACKUPPLUGIN_H
#define SQLSERVERBACKUPPLUGIN_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QJsonObject>
#include <QDateTime>
#include <QTimer>

#include "../../PluginCore/IPlugin.h"

/**
 * @brief The SqlServerBackupPlugin class provides SQL Server database backup functionality.
 */
class SqlServerBackupPlugin : public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "SqlServerBackup.json")
    Q_INTERFACES(IPlugin)

public:
    /**
     * @brief Constructor
     */
    SqlServerBackupPlugin();
    
    /**
     * @brief Destructor
     */
    ~SqlServerBackupPlugin();

    // IPlugin interface
    bool initialize() override;
    bool activate() override;
    bool deactivate() override;
    bool shutdown() override;
    
    QString getPluginId() const override;
    QString getPluginName() const override;
    QString getPluginVersion() const override;
    QString getPluginVendor() const override;
    QString getPluginDescription() const override;
    QStringList getPluginDependencies() const override;
    QJsonObject getPluginMetadata() const override;
    
    QVariant executeCommand(const QString& command, const QVariantMap& params = QVariantMap()) override;

private slots:
    /**
     * @brief Perform a scheduled backup
     */
    void performScheduledBackup();

private:
    /**
     * @brief Perform a database backup
     * 
     * @param serverName SQL Server instance name
     * @param dbName Database name
     * @param useWindowsAuth Whether to use Windows authentication
     * @param username SQL Server username (if not using Windows auth)
     * @param password SQL Server password (if not using Windows auth)
     * @param backupPath Path to save the backup
     * @return True if backup was successful, false otherwise
     */
    bool performBackup(const QString& serverName, const QString& dbName,
                      bool useWindowsAuth, const QString& username, const QString& password,
                      const QString& backupPath);

    /**
     * @brief Load plugin configuration
     */
    void loadConfig();

    /**
     * @brief Save plugin configuration
     */
    void saveConfig();

    /**
     * @brief Start scheduled backups
     */
    void startScheduledBackups();

    /**
     * @brief Stop scheduled backups
     */
    void stopScheduledBackups();

    QJsonObject m_metadata;
    bool m_initialized;
    bool m_active;
    
    // Configuration
    QString m_serverName;
    QString m_dbName;
    bool m_useWindowsAuth;
    QString m_username;
    QString m_password;
    QString m_backupDir;
    bool m_scheduleEnabled;
    int m_scheduleInterval; // in minutes
    
    QTimer m_backupTimer;
    QDateTime m_lastBackupTime;
};

#endif // SQLSERVERBACKUPPLUGIN_H