#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    auto auth = std::make_shared<AuthManager>();

    MainWindow w(auth);
    w.show();
    return a.exec();
}
