#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QListWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include <QList>
#include <QAction>
#include <QVariant>

#include "../PluginCore/IPlugin.h"

class PluginManagerDialog;

/**
 * @brief The MainWindow class is the main window of the host application.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * 
     * @param parent Parent widget
     */
    explicit MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~MainWindow();

    /**
     * @brief Initialize the main window
     * 
     * @return True if initialization was successful, false otherwise
     */
    bool initialize();

public slots:
    /**
     * @brief Show the plugin manager dialog
     */
    void showPluginManager();

    /**
     * @brief Refresh the UI to reflect plugin changes
     */
    void refreshPluginUI();

    /**
     * @brief Handle plugin loading
     * 
     * @param pluginId ID of the plugin
     */
    void onPluginLoaded(const QString& pluginId);

    /**
     * @brief Handle plugin unloading
     * 
     * @param pluginId ID of the plugin
     */
    void onPluginUnloaded(const QString& pluginId);

    /**
     * @brief Handle plugin activation
     * 
     * @param pluginId ID of the plugin
     */
    void onPluginActivated(const QString& pluginId);

    /**
     * @brief Handle plugin deactivation
     * 
     * @param pluginId ID of the plugin
     */
    void onPluginDeactivated(const QString& pluginId);

    /**
     * @brief Handle plugin failure
     * 
     * @param pluginId ID of the plugin
     * @param errorMessage Error message
     */
    void onPluginFailed(const QString& pluginId, const QString& errorMessage);

    /**
     * @brief Handle plugin status change
     * 
     * @param status Status message
     */
    void onPluginStatusChanged(const QString& status);

    /**
     * @brief Handle plugin event
     * 
     * @param eventType Type of the event
     * @param data Data associated with the event
     */
    void onPluginEventOccurred(const QString& eventType, const QVariant& data);

    /**
     * @brief Execute a plugin action
     */
    void executePluginAction();

private:
    /**
     * @brief Create the main menu
     */
    void createMenu();

    /**
     * @brief Create the toolbar
     */
    void createToolbar();

    /**
     * @brief Create the status bar
     */
    void createStatusBar();

    /**
     * @brief Create the central widget
     */
    void createCentralWidget();

    /**
     * @brief Create dock widgets
     */
    void createDockWidgets();

    /**
     * @brief Connect signals and slots
     */
    void connectSignals();

    /**
     * @brief Add plugin to the UI
     * 
     * @param plugin Plugin instance
     * @param pluginId ID of the plugin
     */
    void addPluginToUI(IPlugin* plugin, const QString& pluginId);

    /**
     * @brief Remove plugin from the UI
     * 
     * @param pluginId ID of the plugin
     */
    void removePluginFromUI(const QString& pluginId);

    /**
     * @brief Update plugin in the UI
     * 
     * @param plugin Plugin instance
     * @param pluginId ID of the plugin
     */
    void updatePluginInUI(IPlugin* plugin, const QString& pluginId);

    // UI elements
    QMenu* m_fileMenu;
    QMenu* m_viewMenu;
    QMenu* m_pluginsMenu;
    QMenu* m_helpMenu;
    
    QToolBar* m_mainToolBar;
    
    QDockWidget* m_pluginListDock;
    QListWidget* m_pluginListWidget;
    
    QDockWidget* m_logDock;
    QTableWidget* m_logTable;
    
    QLabel* m_statusLabel;
    
    PluginManagerDialog* m_pluginManagerDialog;
    
    // Plugin actions
    QMap<QString, QList<QAction*>> m_pluginActions;
    QMap<QString, QMenu*> m_pluginMenus;
};

#endif // MAINWINDOW_H