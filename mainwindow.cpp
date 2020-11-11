#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QThread>
#include <QRegularExpression>
#include <time.h>
#include <QScrollBar>
#include <QFile>

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
    ui->tableView->setColumnWidth(3, 400);
    ui->textBrowser->setVisible(false);
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
            QString strContent;
            clock_t lastInsertTime = clock();
            QFile logFile("device.log");
            logFile.open(QFile::WriteOnly);
            
            while(ui->actionStart->isChecked() == true)
            {
                QByteArray arr = logProcess.readLine();
                if(logFile.isOpen())
                    logFile.write(arr);
                
                if(!arr.isEmpty())
                {
                    QString str(arr);
                    bool isScrollToEnd = ui->textBrowser->verticalScrollBar()->value() > ui->textBrowser->verticalScrollBar()->maximum()-20;
                    ui->textBrowser->insertPlainText(str);
                    if(ui->textBrowser->isVisible() == true && isScrollToEnd == true)
                        ui->textBrowser->moveCursor(QTextCursor::End);
                    
                    QRegularExpressionMatch match = splitPattern.match(str);
                    isScrollToEnd = ui->tableView->verticalScrollBar()->value()==ui->tableView->verticalScrollBar()->maximum();
                    if((match.hasMatch() && match.lastCapturedIndex() == 4))
                    {
                        QList<QStandardItem *>itemRow;
                        qDebug()<<"End   " << strContent << itemRow;
                        
                        //TODO:这里重新格式化一下
                        strContent = match.captured(match.lastCapturedIndex());
                        itemRow << new QStandardItem(match.captured(1))
                            << new QStandardItem(match.captured(2))
                            << new QStandardItem(match.captured(3))
                            << new QStandardItem(strContent);
                        model.appendRow(itemRow);
                        if(isScrollToEnd)
                            ui->tableView->scrollToBottom();
                        itemRow.clear();

                        qDebug() << strContent << itemRow;
                        lineCnt++;
                    }
                    else
                    {
                        if(str.isEmpty() == false)
                        {
                            strContent += str;
                            QModelIndex index = model.index(model.rowCount()-1,model.columnCount()-1);
                            
                            model.setData(index, strContent);
                            ui->tableView->resizeRowToContents(model.rowCount()-1);
                            qDebug()<<"======="<<index<<strContent;
                            if(isScrollToEnd)
                                ui->tableView->scrollToBottom();
                        }
                    }
                }
                QCoreApplication::processEvents();
                QThread::msleep(1);
            }
            logFile.close();
        }
    }
    logProcess.close();
    logProcess.waitForFinished();
    
    qDebug() << "logProcess closed";
}
