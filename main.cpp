#include "mainwindow.h"
#include <QApplication>
#include <QScreen>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Set application information
    QApplication::setApplicationName("FStarRTool");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("FStarRTool");
    
    // Set default font
    QFont font("Microsoft YaHei", 9);
    QApplication::setFont(font);
    
    MainWindow w;
    w.show();
    
    return a.exec();
}
