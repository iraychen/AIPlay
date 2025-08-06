#include "MainWindow.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application information
    QApplication::setApplicationName("Enterprise Plugin Framework");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("Enterprise");
    QApplication::setOrganizationDomain("enterprise.com");
    
    // Create main window
    MainWindow mainWindow;
    
    // Initialize
    if (!mainWindow.initialize()) {
        QMessageBox::critical(nullptr, "Error", "Failed to initialize application");
        return 1;
    }
    
    // Show main window
    mainWindow.show();
    
    return app.exec();
}