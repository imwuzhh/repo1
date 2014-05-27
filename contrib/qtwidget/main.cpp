#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator * translator = new QTranslator(&a);
    if (translator->load(":/lang/zh_CN"))
    {
        a.installTranslator(translator);
    }

    MainWindow w;
    w.show();

    return a.exec();
}
