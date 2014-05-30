#include "dialog.h"
#include <QApplication>
#include <QTcpServer>
#include <QTcpSocket>

int main(int argc, char *argv[])
{
    // qtwebview.exe [viewPort=60688]
    QApplication a(argc, argv);
    unsigned short viewPort = 60688;
    if (argc >= 2) viewPort = atoi(argv[1]);
    Dialog w(viewPort); w.show();
    return a.exec();
}
