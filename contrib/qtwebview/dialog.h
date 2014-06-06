#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QWebView>
#include <QProgressBar>
#include <QLabel>
#include <QTcpServer>
#include <QTcpSocket>

namespace Ui {
class Dialog;
}


class ClientConnection : public QObject
{
    Q_OBJECT
private:
    QString m_req;
    QTcpSocket * m_sock;
public:
    ClientConnection(QTcpSocket * sock);
    ~ClientConnection();
public slots:
    void readyRead();
    void aboutToClose(QAbstractSocket::SocketError);
signals:
    void done(ClientConnection * , const QString &);
};

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(unsigned short viewPort, QWidget *parent = 0);
    ~Dialog();

private:
    Ui::Dialog *ui;
    QWebView * wv;
    //QProgressBar *qb;
    //QLabel * lb;
    QTcpServer * sv;
    QList<ClientConnection*> m_clients;
public slots:
    void loadProgress(int progress);
    void linkClicked(QUrl url);
    void loadFinished(bool finished);
    void newConnection();
    void done(ClientConnection *, const QString & req);
public:
    void browse(const QUrl & url);
};


#endif // DIALOG_H
