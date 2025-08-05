#include "PluginManagerDialog.h"
#include "../PluginCore/PluginManager.h"
#include "../PluginCore/LogManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QFile>

PluginManagerDialog::PluginManagerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Plugin Manager");
    setMinimumSize(800, 600);
    
    // Create layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create plugin table
    m_pluginTable = new QTableWidget(0, 5, this);
    m_pluginTable->setHorizontalHeaderLabels(QStringList() << "ID" << "Name" << "Version" << "Vendor" << "Status");
    m_pluginTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_pluginTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_pluginTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_pluginTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_pluginTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_pluginTable->verticalHeader()->setVisible(false);
    m_pluginTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pluginTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pluginTable->setSelectionMode(QAbstractItemView::SingleSelection);
    
    connect(m_pluginTable, &QTableWidget::itemSelectionChanged,
            this, &PluginManagerDialog::onPluginSelectionChanged);
    
    mainLayout->addWidget(m_pluginTable);
    
    // Create details group
    m_detailsGroup = new QGroupBox("Plugin Details", this);
    QVBoxLayout* detailsLayout = new QVBoxLayout(m_detailsGroup);
    
    m_detailsText = new QTextEdit(m_detailsGroup);
    m_detailsText->setReadOnly(true);
    
    detailsLayout->addWidget(m_detailsText);
    
    mainLayout->addWidget(m_detailsGroup);
    
    // Create buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_loadButton = new QPushButton("Load", this);
    m_unloadButton = new QPushButton("Unload", this);
    m_activateButton = new QPushButton("Activate", this);
    m_deactivateButton = new QPushButton("Deactivate", this);
    m_detailsButton = new QPushButton("Details", this);
    m_browseButton = new QPushButton("Browse...", this);
    m_closeButton = new QPushButton("Close", this);
    
    connect(m_loadButton, &QPushButton::clicked, this, &PluginManagerDialog::loadPlugin);
    connect(m_unloadButton, &QPushButton::clicked, this, &PluginManagerDialog::unloadPlugin);
    connect(m_activateButton, &QPushButton::clicked, this, &PluginManagerDialog::activatePlugin);
    connect(m_deactivateButton, &QPushButton::clicked, this, &PluginManagerDialog::deactivatePlugin);
    connect(m_detailsButton, &QPushButton::clicked, this, &PluginManagerDialog::showPluginDetails);
    connect(m_browseButton, &QPushButton::clicked, this, &PluginManagerDialog::browseForPlugins);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    
    buttonLayout->addWidget(m_loadButton);
    buttonLayout->addWidget(m_unloadButton);
    buttonLayout->addWidget(m_activateButton);
    buttonLayout->addWidget(m_deactivateButton);
    buttonLayout->addWidget(m_detailsButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_browseButton);
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Initialize
    refresh();
    updateButtonStates();
}

PluginManagerDialog::~PluginManagerDialog()
{
}

void PluginManagerDialog::refresh()
{
    m_pluginTable->clearContents();
    m_pluginTable->setRowCount(0);
    
    // Get available plugins
    QMap<QString, PluginMetadata> availablePlugins = PluginManager::instance().getAvailablePlugins();
    
    // Add plugins to table
    int row = 0;
    for (auto it = availablePlugins.begin(); it != availablePlugins.end(); ++it) {
        const QString& pluginId = it.key();
        const PluginMetadata& metadata = it.value();
        
        m_pluginTable->insertRow(row);
        
        m_pluginTable->setItem(row, 0, new QTableWidgetItem(pluginId));
        m_pluginTable->setItem(row, 1, new QTableWidgetItem(metadata.getPluginName()));
        m_pluginTable->setItem(row, 2, new QTableWidgetItem(metadata.getPluginVersion()));
        m_pluginTable->setItem(row, 3, new QTableWidgetItem(metadata.getPluginVendor()));
        
        QString statusText;
        PluginState state = PluginManager::instance().getPluginState(pluginId);
        
        switch (state) {
            case PluginState::NotLoaded:
                statusText = "Not Loaded";
                break;
            case PluginState::Loaded:
                statusText = "Loaded";
                break;
            case PluginState::Initialized:
                statusText = "Initialized";
                break;
            case PluginState::Active:
                statusText = "Active";
                break;
            case PluginState::Inactive:
                statusText = "Inactive";
                break;
            case PluginState::Failed:
                statusText = "Failed";
                break;
            default:
                statusText = "Unknown";
                break;
        }
        
        m_pluginTable->setItem(row, 4, new QTableWidgetItem(statusText));
        
        ++row;
    }
    
    // Update button states
    updateButtonStates();
}

void PluginManagerDialog::loadPlugin()
{
    QList<QTableWidgetItem*> selectedItems = m_pluginTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    int row = selectedItems.first()->row();
    QString pluginId = m_pluginTable->item(row, 0)->text();
    
    if (PluginManager::instance().loadPlugin(pluginId)) {
        refresh();
    } else {
        QMessageBox::warning(this, "Error", QString("Failed to load plugin: %1").arg(pluginId));
    }
}

void PluginManagerDialog::unloadPlugin()
{
    QList<QTableWidgetItem*> selectedItems = m_pluginTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    int row = selectedItems.first()->row();
    QString pluginId = m_pluginTable->item(row, 0)->text();
    
    if (PluginManager::instance().unloadPlugin(pluginId)) {
        refresh();
    } else {
        QMessageBox::warning(this, "Error", QString("Failed to unload plugin: %1").arg(pluginId));
    }
}

void PluginManagerDialog::activatePlugin()
{
    QList<QTableWidgetItem*> selectedItems = m_pluginTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    int row = selectedItems.first()->row();
    QString pluginId = m_pluginTable->item(row, 0)->text();
    
    if (PluginManager::instance().activatePlugin(pluginId)) {
        refresh();
    } else {
        QMessageBox::warning(this, "Error", QString("Failed to activate plugin: %1").arg(pluginId));
    }
}

void PluginManagerDialog::deactivatePlugin()
{
    QList<QTableWidgetItem*> selectedItems = m_pluginTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    int row = selectedItems.first()->row();
    QString pluginId = m_pluginTable->item(row, 0)->text();
    
    if (PluginManager::instance().deactivatePlugin(pluginId)) {
        refresh();
    } else {
        QMessageBox::warning(this, "Error", QString("Failed to deactivate plugin: %1").arg(pluginId));
    }
}

void PluginManagerDialog::showPluginDetails()
{
    QList<QTableWidgetItem*> selectedItems = m_pluginTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    int row = selectedItems.first()->row();
    QString pluginId = m_pluginTable->item(row, 0)->text();
    
    PluginMetadata metadata = PluginManager::instance().getPluginMetadata(pluginId);
    
    QString details = QString("<h2>%1</h2>").arg(metadata.getPluginName());
    details += QString("<p><b>ID:</b> %1</p>").arg(metadata.getPluginId());
    details += QString("<p><b>Version:</b> %1</p>").arg(metadata.getPluginVersion());
    details += QString("<p><b>Vendor:</b> %1</p>").arg(metadata.getPluginVendor());
    details += QString("<p><b>Description:</b> %1</p>").arg(metadata.getPluginDescription());
    
    QStringList dependencies = metadata.getPluginDependencies();
    if (!dependencies.isEmpty()) {
        details += "<p><b>Dependencies:</b></p><ul>";
        for (const QString& dep : dependencies) {
            details += QString("<li>%1</li>").arg(dep);
        }
        details += "</ul>";
    }
    
    QStringList permissions = metadata.getRequiredPermissions();
    if (!permissions.isEmpty()) {
        details += "<p><b>Required Permissions:</b></p><ul>";
        for (const QString& perm : permissions) {
            details += QString("<li>%1</li>").arg(perm);
        }
        details += "</ul>";
    }
    
    m_detailsText->setHtml(details);
}

void PluginManagerDialog::onPluginSelectionChanged()
{
    updateButtonStates();
    showPluginDetails();
}

void PluginManagerDialog::browseForPlugins()
{
    QString pluginDir = QApplication::applicationDirPath() + "/plugins";
    QString fileName = QFileDialog::getOpenFileName(this, "Browse for Plugins",
                                                  pluginDir,
                                                  "Plugin Files (*.dll *.so *.dylib);;All Files (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // Copy plugin to plugins directory
    QFileInfo fileInfo(fileName);
    QString destFileName = QDir(pluginDir).filePath(fileInfo.fileName());
    
    if (QFile::exists(destFileName)) {
        QMessageBox::StandardButton button = QMessageBox::question(this, "File Exists",
                                                                 "A plugin with this name already exists. Overwrite?",
                                                                 QMessageBox::Yes | QMessageBox::No);
        if (button != QMessageBox::Yes) {
            return;
        }
        
        QFile::remove(destFileName);
    }
    
    if (!QFile::copy(fileName, destFileName)) {
        QMessageBox::warning(this, "Error", "Failed to copy plugin file");
        return;
    }
    
    // Scan for plugins
    PluginManager::instance().scanForPlugins();
    
    // Refresh
    refresh();
}

void PluginManagerDialog::updateButtonStates()
{
    QList<QTableWidgetItem*> selectedItems = m_pluginTable->selectedItems();
    bool hasSelection = !selectedItems.isEmpty();
    
    m_loadButton->setEnabled(false);
    m_unloadButton->setEnabled(false);
    m_activateButton->setEnabled(false);
    m_deactivateButton->setEnabled(false);
    m_detailsButton->setEnabled(false);
    
    if (hasSelection) {
        int row = selectedItems.first()->row();
        QString pluginId = m_pluginTable->item(row, 0)->text();
        PluginState state = PluginManager::instance().getPluginState(pluginId);
        
        m_detailsButton->setEnabled(true);
        
        switch (state) {
            case PluginState::NotLoaded:
                m_loadButton->setEnabled(true);
                break;
            case PluginState::Loaded:
            case PluginState::Initialized:
                m_unloadButton->setEnabled(true);
                m_activateButton->setEnabled(true);
                break;
            case PluginState::Active:
                m_deactivateButton->setEnabled(true);
                break;
            case PluginState::Inactive:
                m_unloadButton->setEnabled(true);
                m_activateButton->setEnabled(true);
                break;
            case PluginState::Failed:
                m_unloadButton->setEnabled(true);
                break;
            default:
                break;
        }
    }
}