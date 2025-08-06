#ifndef MYSQLBACKUPPLUGIN_H
#define MYSQLBACKUPPLUGIN_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QJsonObject>
#include <QDateTime>
#include <QTimer>

#include "../../PluginCore/IPlugin.h"

/**
 * @brief The MySqlBackupPlugin class provides MySQL database backup functionality.
 */
class MySqlBackupPlugin : public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "MySqlBackup.json")
    Q_INTERFACES(IPlugin)

public:
    /**
     * @brief Constructor
     */
    MySqlBackupPlugin();
    
    /**
     * @brief Destructor
     */
    ~MySqlBackupPlugin();

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
     * @param dbHost Database host
     * @param dbPort Database port
     * @param dbName Database name
     * @param dbUser Database user
     * @param dbPassword Database password
     * @param backupPath Path to save the backup
     * @return True if backup was successful, false otherwise
     */
    bool performBackup(const QString& dbHost, int dbPort, const QString& dbName,
                      const QString& dbUser, const QString& dbPassword,
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
    QString m_dbHost;
    int m_dbPort;
    QString m_dbName;
    QString m_dbUser;
    QString m_dbPassword;
    QString m_backupDir;
    bool m_scheduleEnabled;
    int m_scheduleInterval; // in minutes
    
    QTimer m_backupTimer;
    QDateTime m_lastBackupTime;
};

#endif // MYSQLBACKUPPLUGIN_H