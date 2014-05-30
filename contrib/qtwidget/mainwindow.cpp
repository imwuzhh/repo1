#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QLineEdit>
#include <QFile>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QFile fin("login.conf");
    if (fin.exists()){
        QComboBox * addr = this->findChild<QComboBox*>("serverList");
        QLineEdit * user = this->findChild<QLineEdit*>("userEdit");

        fin.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream finstrm(&fin);
        do {
            QString line = finstrm.readLine();
            int pos = -1;
            if ((pos = line.indexOf("addr=")) != -1){
                QString sAddr = line.mid(pos + 5);
                sAddr.replace("\r\n", "");
                addr->insertItem(0, sAddr);
            }else if ((pos = line.indexOf("user=")) != -1){
                QString sUser = line.mid(pos + 5);
                sUser.replace("\r\n", "");
                user->setText(sUser);
            }
        }while(!finstrm.atEnd());
        fin.close();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::close()
{
    QComboBox * addr = this->findChild<QComboBox*>("serverList");
    QLineEdit * user = this->findChild<QLineEdit*>("userEdit");
    QLineEdit * pass = this->findChild<QLineEdit*>("passEdit");

    QFile fout("login.conf");
    fout.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream foutstrm(&fout);
    foutstrm<<"[Login]"<<"\r\n";
    foutstrm<<"addr="<<addr->lineEdit()->text()<<"\r\n";
    foutstrm<<"user="<<user->text()<<"\r\n";
    foutstrm<<"pass="<<pass->text()<<"\r\n";
    fout.close();

    ((QMainWindow *)this)->close();

    return true;
}

void MainWindow::on_ResetBtn_clicked()
{
    QComboBox * addr = this->findChild<QComboBox*>("serverList");
    QLineEdit * user = this->findChild<QLineEdit*>("userEdit");
    QLineEdit * pass = this->findChild<QLineEdit*>("passEdit");

    addr->lineEdit()->setText("");
    user->setText("");
    pass->setText("");
}
