#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QThread>
#include <QRegularExpression>
#include <time.h>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , logProcess(this)
{
    ui->setupUi(this);
    connect(ui->actionStart, &QAction::triggered, this, &MainWindow::Start);
    model.setColumnCount(4);
    model.setHeaderData(0, Qt::Orientation::Horizontal, "Time");
    model.setHeaderData(1, Qt::Orientation::Horizontal, "Process");
    model.setHeaderData(2, Qt::Orientation::Horizontal, "Thread");
    model.setHeaderData(3, Qt::Orientation::Horizontal, "Content");
    
    ui->tableView->setModel(&model);
    ui->tableView->setWordWrap(true);
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
        arguments << "-p" << "AwemeInhouse";
        qDebug() << arguments;
        
        logProcess.start("idevicesyslog", arguments);
        if(logProcess.waitForStarted())
        {
            qDebug() << "logProcess started";
            int lineCnt = 0;
            QRegularExpression splitPattern(R"((\w+ \d+ \d{2}:\d{2}:\d{2}) ([a-zA-Z]+)\[(\d+)] <\w+>: (.*))");//(R"((\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d{6}\+\d{4}) ([a-zA-Z]+)\[(\d+:\d+)] )"); // 分隔用的正则表达式
            //splitPattern.namedCaptureGroups()
            QRegularExpression formatPattern(R"((?P<VESDK>[\S]+) )"
                                  R"((?P<Function>.*) )"
                                  R"(\[(?P<Line>\d+)] )"
                                  R"(-- (?P<Content>[\s\S]*)')");
            QString strRow;
            QList<QStandardItem *>itemRow;
            clock_t lastInsertTime = clock();
            
            while(ui->actionStart->isChecked() == true)
            {
                QByteArray arr = logProcess.readLine();
                
                if(!arr.isEmpty())
                {
                    QString str(arr);
                    bool isScrollToEnd = ui->textBrowser->verticalScrollBar()->value() > ui->textBrowser->verticalScrollBar()->maximum()-20;
                    ui->textBrowser->insertPlainText(str);
                    if(isScrollToEnd)
                        ui->textBrowser->moveCursor(QTextCursor::End);
                    
                    QRegularExpressionMatch match = splitPattern.match(str);
                    if((match.hasMatch() && match.lastCapturedIndex() == 4))
                    {
                        qDebug()<<"End   " << strRow << itemRow;
                        isScrollToEnd = ui->tableView->verticalScrollBar()->value()==ui->tableView->verticalScrollBar()->maximum();
                        
                        itemRow << new QStandardItem(strRow);
                        model.appendRow(itemRow);
                        if(strRow.count('\n')>1)
                            ui->tableView->resizeRowsToContents();
                        if(isScrollToEnd)
                            ui->tableView->scrollToBottom();
                        lastInsertTime = clock();
                        itemRow.clear();
                        
                        itemRow << new QStandardItem(match.captured(1))
                            << new QStandardItem(match.captured(2))
                            << new QStandardItem(match.captured(3));
                        

                        strRow = match.captured(match.lastCapturedIndex());
                        qDebug() << strRow << itemRow;
                        lineCnt++;
                    }
                    else
                    {
                        strRow += str;
                        qDebug()<<"======="<<strRow;
                    }
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
