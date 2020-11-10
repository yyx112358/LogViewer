#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , logProcess(this)
{
    ui->setupUi(this);
    connect(ui->actionStart, &QAction::triggered, this, &MainWindow::Start);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::Start(bool b)
{
    if(b == true)
    {
        QStringList arguments;
        qDebug() << arguments;
        
        logProcess.start("idevicesyslog", arguments);
        if(logProcess.waitForStarted())
        {
            qDebug() << "logProcess started";
            while(ui->actionStart->isChecked() == true)
            {
                QByteArray arr = logProcess.readLine();
                if(!arr.isEmpty())
                {
                    qDebug() << arr;
                    ui->textBrowser->insertPlainText(QString(arr));
                    ui->textBrowser->moveCursor(QTextCursor::End);
                }
                QCoreApplication::processEvents();
                QThread::msleep(1);
            }
        }
    }
    logProcess.close();
    logProcess.waitForFinished();
    qDebug() << "logProcess closed";
}
