#ifndef PLUGINMANAGERDIALOG_H
#define PLUGINMANAGERDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTextEdit>

/**
 * @brief The PluginManagerDialog class provides a dialog for managing plugins.
 */
class PluginManagerDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * 
     * @param parent Parent widget
     */
    explicit PluginManagerDialog(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~PluginManagerDialog();

    /**
     * @brief Refresh the plugin list
     */
    void refresh();

private slots:
    /**
     * @brief Load the selected plugin
     */
    void loadPlugin();

    /**
     * @brief Unload the selected plugin
     */
    void unloadPlugin();

    /**
     * @brief Activate the selected plugin
     */
    void activatePlugin();

    /**
     * @brief Deactivate the selected plugin
     */
    void deactivatePlugin();

    /**
     * @brief Show plugin details
     */
    void showPluginDetails();

    /**
     * @brief Handle plugin selection change
     */
    void onPluginSelectionChanged();

    /**
     * @brief Browse for plugins
     */
    void browseForPlugins();

private:
    /**
     * @brief Update button states based on selected plugin
     */
    void updateButtonStates();

    QTableWidget* m_pluginTable;
    
    QPushButton* m_loadButton;
    QPushButton* m_unloadButton;
    QPushButton* m_activateButton;
    QPushButton* m_deactivateButton;
    QPushButton* m_detailsButton;
    QPushButton* m_browseButton;
    QPushButton* m_closeButton;
    
    QGroupBox* m_detailsGroup;
    QTextEdit* m_detailsText;
};

#endif // PLUGINMANAGERDIALOG_H