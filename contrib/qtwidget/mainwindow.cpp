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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_loginBtn_clicked()
{
    qDebug("");
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
