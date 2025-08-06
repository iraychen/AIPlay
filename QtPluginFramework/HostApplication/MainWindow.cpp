#include "MainWindow.h"
#include "PluginManagerDialog.h"

#include "../PluginCore/PluginManager.h"
#include "../PluginCore/LogManager.h"
#include "../PluginCore/ConfigManager.h"
#include "../PluginCore/PermissionManager.h"
#include "../PluginCore/PluginCommunication.h"

#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QCloseEvent>
#include <QDateTime>
#include <QHeaderView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTextEdit>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_pluginManagerDialog(nullptr)
{
    setWindowTitle("Enterprise Plugin Framework");
    setMinimumSize(800, 600);
    
    // Create UI elements
    createMenu();
    createToolbar();
    createStatusBar();
    createCentralWidget();
    createDockWidgets();
    
    // Connect signals
    connectSignals();
    
    // Restore window state
    QSettings settings("EnterprisePluginFramework", "HostApplication");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

MainWindow::~MainWindow()
{
    // Save window state
    QSettings settings("EnterprisePluginFramework", "HostApplication");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    
    delete m_pluginManagerDialog;
}

bool MainWindow::initialize()
{
    // Initialize plugin manager
    QString appDir = QApplication::applicationDirPath();
    QString pluginDir = QDir(appDir).filePath("plugins");
    QString metadataDir = QDir(appDir).filePath("metadata");
    QString configDir = QDir(appDir).filePath("config");
    QString logDir = QDir(appDir).filePath("logs");
    
    // Create directories if they don't exist
    QDir().mkpath(pluginDir);
    QDir().mkpath(metadataDir);
    QDir().mkpath(configDir);
    QDir().mkpath(logDir);
    
    // Initialize log manager
    QString logFile = QDir(logDir).filePath(QString("host_%1.log")
                                           .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")));
    if (!LogManager::instance().initialize(logFile, true, LogLevel::Debug)) {
        QMessageBox::critical(this, "Error", "Failed to initialize log manager");
        return false;
    }
    
    // Initialize config manager
    if (!ConfigManager::instance().initialize(configDir)) {
        LOG_ERROR("MainWindow", "Failed to initialize config manager");
        QMessageBox::critical(this, "Error", "Failed to initialize config manager");
        return false;
    }
    
    // Load framework config
    QString frameworkConfigFile = QDir(configDir).filePath("framework.json");
    if (QFile::exists(frameworkConfigFile)) {
        if (!ConfigManager::instance().loadFrameworkConfig(frameworkConfigFile)) {
            LOG_WARNING("MainWindow", "Failed to load framework config");
        }
    }
    
    // Initialize permission manager
    if (!PermissionManager::instance().initialize()) {
        LOG_ERROR("MainWindow", "Failed to initialize permission manager");
        QMessageBox::critical(this, "Error", "Failed to initialize permission manager");
        return false;
    }
    
    // Initialize plugin communication
    if (!PluginCommunication::instance().initialize()) {
        LOG_ERROR("MainWindow", "Failed to initialize plugin communication");
        QMessageBox::critical(this, "Error", "Failed to initialize plugin communication");
        return false;
    }
    
    // Initialize plugin manager
    if (!PluginManager::instance().initialize(pluginDir, metadataDir)) {
        LOG_ERROR("MainWindow", "Failed to initialize plugin manager");
        QMessageBox::critical(this, "Error", "Failed to initialize plugin manager");
        return false;
    }
    
    // Scan for plugins
    QStringList pluginIds = PluginManager::instance().scanForPlugins();
    LOG_INFO("MainWindow", QString("Found %1 plugins").arg(pluginIds.size()));
    
    // Create plugin manager dialog
    m_pluginManagerDialog = new PluginManagerDialog(this);
    
    // Refresh UI
    refreshPluginUI();
    
    LOG_INFO("MainWindow", "Initialized");
    
    return true;
}

void MainWindow::showPluginManager()
{
    if (m_pluginManagerDialog) {
        m_pluginManagerDialog->refresh();
        m_pluginManagerDialog->show();
    }
}

void MainWindow::refreshPluginUI()
{
    // Clear plugin list
    m_pluginListWidget->clear();
    
    // Clear plugin menus and actions
    for (auto it = m_pluginMenus.begin(); it != m_pluginMenus.end(); ++it) {
        m_pluginsMenu->removeAction(it.value()->menuAction());
        delete it.value();
    }
    m_pluginMenus.clear();
    
    for (auto it = m_pluginActions.begin(); it != m_pluginActions.end(); ++it) {
        for (QAction* action : it.value()) {
            delete action;
        }
    }
    m_pluginActions.clear();
    
    // Add active plugins to UI
    QMap<QString, IPlugin*> activePlugins = PluginManager::instance().getActivePlugins();
    for (auto it = activePlugins.begin(); it != activePlugins.end(); ++it) {
        addPluginToUI(it.value(), it.key());
    }
    
    // Add inactive plugins to list
    QMap<QString, IPlugin*> loadedPlugins = PluginManager::instance().getLoadedPlugins();
    for (auto it = loadedPlugins.begin(); it != loadedPlugins.end(); ++it) {
        if (!activePlugins.contains(it.key())) {
            QListWidgetItem* item = new QListWidgetItem(it.value()->getPluginName() + " (Inactive)");
            item->setData(Qt::UserRole, it.key());
            m_pluginListWidget->addItem(item);
        }
    }
    
    // Add available but not loaded plugins to list
    QMap<QString, PluginMetadata> availablePlugins = PluginManager::instance().getAvailablePlugins();
    for (auto it = availablePlugins.begin(); it != availablePlugins.end(); ++it) {
        if (!loadedPlugins.contains(it.key())) {
            QListWidgetItem* item = new QListWidgetItem(it.value().getPluginName() + " (Not Loaded)");
            item->setData(Qt::UserRole, it.key());
            m_pluginListWidget->addItem(item);
        }
    }
}

void MainWindow::onPluginLoaded(const QString& pluginId)
{
    LOG_INFO("MainWindow", QString("Plugin loaded: %1").arg(pluginId));
    refreshPluginUI();
}

void MainWindow::onPluginUnloaded(const QString& pluginId)
{
    LOG_INFO("MainWindow", QString("Plugin unloaded: %1").arg(pluginId));
    removePluginFromUI(pluginId);
    refreshPluginUI();
}

void MainWindow::onPluginActivated(const QString& pluginId)
{
    LOG_INFO("MainWindow", QString("Plugin activated: %1").arg(pluginId));
    
    IPlugin* plugin = PluginManager::instance().getPlugin(pluginId);
    if (plugin) {
        addPluginToUI(plugin, pluginId);
    }
    
    refreshPluginUI();
}

void MainWindow::onPluginDeactivated(const QString& pluginId)
{
    LOG_INFO("MainWindow", QString("Plugin deactivated: %1").arg(pluginId));
    removePluginFromUI(pluginId);
    refreshPluginUI();
}

void MainWindow::onPluginFailed(const QString& pluginId, const QString& errorMessage)
{
    LOG_ERROR("MainWindow", QString("Plugin failed: %1 - %2").arg(pluginId, errorMessage));
    QMessageBox::warning(this, "Plugin Failed", QString("Plugin %1 failed: %2").arg(pluginId, errorMessage));
    refreshPluginUI();
}

void MainWindow::onPluginStatusChanged(const QString& status)
{
    m_statusLabel->setText(status);
}

void MainWindow::onPluginEventOccurred(const QString& eventType, const QVariant& data)
{
    LOG_INFO("MainWindow", QString("Plugin event: %1 - %2").arg(eventType, data.toString()));
    
    // Add to log table
    int row = m_logTable->rowCount();
    m_logTable->insertRow(row);
    m_logTable->setItem(row, 0, new QTableWidgetItem(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    m_logTable->setItem(row, 1, new QTableWidgetItem(eventType));
    m_logTable->setItem(row, 2, new QTableWidgetItem(data.toString()));
    
    // Scroll to bottom
    m_logTable->scrollToBottom();
}

void MainWindow::executePluginAction()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    
    QString pluginId = action->data().toString().split(":").first();
    QString command = action->data().toString().split(":").last();
    
    LOG_INFO("MainWindow", QString("Executing plugin action: %1 - %2").arg(pluginId, command));
    
    QVariant result = PluginManager::instance().executePluginCommand(pluginId, command);
    
    if (result.isValid()) {
        LOG_INFO("MainWindow", QString("Plugin action result: %1").arg(result.toString()));
    }
}

void MainWindow::createMenu()
{
    // File menu
    m_fileMenu = menuBar()->addMenu("&File");
    
    QAction* exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);
    m_fileMenu->addAction(exitAction);
    
    // View menu
    m_viewMenu = menuBar()->addMenu("&View");
    
    // Plugins menu
    m_pluginsMenu = menuBar()->addMenu("&Plugins");
    
    QAction* managePluginsAction = new QAction("&Manage Plugins...", this);
    connect(managePluginsAction, &QAction::triggered, this, &MainWindow::showPluginManager);
    m_pluginsMenu->addAction(managePluginsAction);
    
    m_pluginsMenu->addSeparator();
    
    // Help menu
    m_helpMenu = menuBar()->addMenu("&Help");
    
    QAction* aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About Enterprise Plugin Framework",
                          "Enterprise Plugin Framework v1.0.0\n\n"
                          "A comprehensive plugin framework for enterprise applications.");
    });
    m_helpMenu->addAction(aboutAction);
}

void MainWindow::createToolbar()
{
    m_mainToolBar = addToolBar("Main Toolbar");
    m_mainToolBar->setMovable(false);
    
    QAction* managePluginsAction = new QAction(QIcon(":/icons/plugin.png"), "Manage Plugins", this);
    connect(managePluginsAction, &QAction::triggered, this, &MainWindow::showPluginManager);
    m_mainToolBar->addAction(managePluginsAction);
}

void MainWindow::createStatusBar()
{
    m_statusLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusLabel, 1);
}

void MainWindow::createCentralWidget()
{
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    
    QLabel* welcomeLabel = new QLabel("Welcome to Enterprise Plugin Framework", centralWidget);
    welcomeLabel->setAlignment(Qt::AlignCenter);
    QFont font = welcomeLabel->font();
    font.setPointSize(16);
    welcomeLabel->setFont(font);
    
    QTextEdit* infoText = new QTextEdit(centralWidget);
    infoText->setReadOnly(true);
    infoText->setHtml("<h2>Enterprise Plugin Framework</h2>"
                     "<p>This is a comprehensive plugin framework for enterprise applications.</p>"
                     "<p>Use the <b>Plugins</b> menu or the <b>Manage Plugins</b> button to load, activate, and manage plugins.</p>"
                     "<p>Active plugins will appear in the menu and can be accessed from there.</p>");
    
    layout->addWidget(welcomeLabel);
    layout->addWidget(infoText);
    
    setCentralWidget(centralWidget);
}

void MainWindow::createDockWidgets()
{
    // Plugin list dock
    m_pluginListDock = new QDockWidget("Plugins", this);
    m_pluginListDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    m_pluginListWidget = new QListWidget(m_pluginListDock);
    m_pluginListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    
    connect(m_pluginListWidget, &QListWidget::customContextMenuRequested, [this](const QPoint& pos) {
        QListWidgetItem* item = m_pluginListWidget->itemAt(pos);
        if (!item) {
            return;
        }
        
        QString pluginId = item->data(Qt::UserRole).toString();
        PluginState state = PluginManager::instance().getPluginState(pluginId);
        
        QMenu contextMenu;
        
        if (state == PluginState::NotLoaded) {
            QAction* loadAction = contextMenu.addAction("Load");
            connect(loadAction, &QAction::triggered, [this, pluginId]() {
                PluginManager::instance().loadPlugin(pluginId);
            });
        } else if (state == PluginState::Loaded || state == PluginState::Initialized) {
            QAction* activateAction = contextMenu.addAction("Activate");
            connect(activateAction, &QAction::triggered, [this, pluginId]() {
                PluginManager::instance().activatePlugin(pluginId);
            });
            
            QAction* unloadAction = contextMenu.addAction("Unload");
            connect(unloadAction, &QAction::triggered, [this, pluginId]() {
                PluginManager::instance().unloadPlugin(pluginId);
            });
        } else if (state == PluginState::Active) {
            QAction* deactivateAction = contextMenu.addAction("Deactivate");
            connect(deactivateAction, &QAction::triggered, [this, pluginId]() {
                PluginManager::instance().deactivatePlugin(pluginId);
            });
        }
        
        contextMenu.exec(m_pluginListWidget->mapToGlobal(pos));
    });
    
    m_pluginListDock->setWidget(m_pluginListWidget);
    addDockWidget(Qt::LeftDockWidgetArea, m_pluginListDock);
    m_viewMenu->addAction(m_pluginListDock->toggleViewAction());
    
    // Log dock
    m_logDock = new QDockWidget("Log", this);
    m_logDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    
    m_logTable = new QTableWidget(0, 3, m_logDock);
    m_logTable->setHorizontalHeaderLabels(QStringList() << "Time" << "Type" << "Message");
    m_logTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_logTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_logTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_logTable->verticalHeader()->setVisible(false);
    m_logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    m_logDock->setWidget(m_logTable);
    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);
    m_viewMenu->addAction(m_logDock->toggleViewAction());
}

void MainWindow::connectSignals()
{
    // Connect plugin manager signals
    connect(&PluginManager::instance(), &PluginManager::pluginLoaded,
            this, &MainWindow::onPluginLoaded);
    connect(&PluginManager::instance(), &PluginManager::pluginUnloaded,
            this, &MainWindow::onPluginUnloaded);
    connect(&PluginManager::instance(), &PluginManager::pluginActivated,
            this, &MainWindow::onPluginActivated);
    connect(&PluginManager::instance(), &PluginManager::pluginDeactivated,
            this, &MainWindow::onPluginDeactivated);
    connect(&PluginManager::instance(), &PluginManager::pluginFailed,
            this, &MainWindow::onPluginFailed);
}

void MainWindow::addPluginToUI(IPlugin* plugin, const QString& pluginId)
{
    if (!plugin) {
        return;
    }
    
    // Add to plugin list
    bool found = false;
    for (int i = 0; i < m_pluginListWidget->count(); ++i) {
        QListWidgetItem* item = m_pluginListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == pluginId) {
            item->setText(plugin->getPluginName() + " (Active)");
            found = true;
            break;
        }
    }
    
    if (!found) {
        QListWidgetItem* item = new QListWidgetItem(plugin->getPluginName() + " (Active)");
        item->setData(Qt::UserRole, pluginId);
        m_pluginListWidget->addItem(item);
    }
    
    // Add plugin menu
    if (!m_pluginMenus.contains(pluginId)) {
        QMenu* pluginMenu = new QMenu(plugin->getPluginName(), this);
        m_pluginsMenu->addMenu(pluginMenu);
        m_pluginMenus[pluginId] = pluginMenu;
        
        // Connect plugin signals
        connect(plugin, &IPlugin::statusChanged,
                this, &MainWindow::onPluginStatusChanged);
        connect(plugin, &IPlugin::eventOccurred,
                this, &MainWindow::onPluginEventOccurred);
        
        // Add plugin actions
        QList<QAction*> actions;
        
        // Example: Add a "Show Info" action
        QAction* infoAction = new QAction("Show Info", this);
        infoAction->setData(QString("%1:showInfo").arg(pluginId));
        connect(infoAction, &QAction::triggered, this, &MainWindow::executePluginAction);
        pluginMenu->addAction(infoAction);
        actions.append(infoAction);
        
        // Example: Add a "Configure" action
        QAction* configAction = new QAction("Configure...", this);
        configAction->setData(QString("%1:configure").arg(pluginId));
        connect(configAction, &QAction::triggered, this, &MainWindow::executePluginAction);
        pluginMenu->addAction(configAction);
        actions.append(configAction);
        
        // Add plugin-specific actions
        // In a real implementation, these would be obtained from the plugin
        
        m_pluginActions[pluginId] = actions;
    }
}

void MainWindow::removePluginFromUI(const QString& pluginId)
{
    // Remove plugin menu
    if (m_pluginMenus.contains(pluginId)) {
        m_pluginsMenu->removeAction(m_pluginMenus[pluginId]->menuAction());
        delete m_pluginMenus[pluginId];
        m_pluginMenus.remove(pluginId);
    }
    
    // Remove plugin actions
    if (m_pluginActions.contains(pluginId)) {
        for (QAction* action : m_pluginActions[pluginId]) {
            delete action;
        }
        m_pluginActions.remove(pluginId);
    }
}

void MainWindow::updatePluginInUI(IPlugin* plugin, const QString& pluginId)
{
    removePluginFromUI(pluginId);
    addPluginToUI(plugin, pluginId);
}