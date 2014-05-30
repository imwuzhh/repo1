#include "dialog.h"
#include "ui_dialog.h"
#include <QTcpServer>
#include <QTcpSocket>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    wv = this->findChild<QWebView*>("webView");
    qb = this->findChild<QProgressBar *>("progressBar");
    lb = this->findChild<QLabel*>("label");

    qb->setValue(0);
    connect(wv, SIGNAL(loadProgress(int)), this, SLOT(loadProgress(int)));
    connect(wv, SIGNAL(linkClicked(QUrl)), this, SLOT(linkClicked(QUrl)));
    connect(wv, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));

    sv = new QTcpServer();
    sv->listen(QHostAddress::Any, 60688);
    connect(sv, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::loadProgress(int progress)
{
    qb->setValue(progress);
}

void Dialog::linkClicked(QUrl url)
{
    qDebug(url.toString().toStdString().c_str());
}

void Dialog::loadFinished(bool finished)
{
    qDebug(finished ? "done" : "loading...");
}

void Dialog::newConnection()
{
    QTcpSocket * client = sv->nextPendingConnection();
    ClientConnection * conn = new ClientConnection(client);
    connect(client, SIGNAL(readyRead()), conn, SLOT(readyRead()));
    connect(client, SIGNAL(error(QAbstractSocket::SocketError)), conn, SLOT(aboutToClose(QAbstractSocket::SocketError)));
    connect(conn, SIGNAL(done(ClientConnection*, const QString &)), this, SLOT(done(ClientConnection *, const QString &)));
}

void Dialog::done(ClientConnection * conn, const QString & req)
{
    qDebug(req.toStdString().c_str());
    browse(QUrl(req));
    delete conn;
}

void Dialog::browse(const QUrl &url)
{
    lb->setText(url.toString());
    wv->setUrl(QUrl(url));
}

ClientConnection::ClientConnection(QTcpSocket * sock)
    : m_sock(sock)
{

}

ClientConnection::~ClientConnection()
{
    //delete m_sock; m_sock = NULL;
}

void ClientConnection::readyRead()
{
    m_req += m_sock->readAll();
}

void ClientConnection::aboutToClose(QAbstractSocket::SocketError)
{
    emit done(this, m_req);
}
