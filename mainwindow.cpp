#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QThread>
#include <QRegularExpression>
#include <time.h>
#include <QScrollBar>
#include <QFile>
#include <QDateTime>

const char SETTING_FILE_PATH[] = "/setting.ini";
const char SETTING_PROCESS_NAME[] = "Process/ProcessName";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), actionGroupProcess(this), actionGroupColumn(this)
    , logProcess(this)
#if !DEBUG
    , settingFile(QString("/Users/yangyixuan/Projects/LogViewer") + SETTING_FILE_PATH, QSettings::IniFormat)
#else
    , settingFile(QCoreApplication::applicationDirPath() + SETTING_FILE_PATH, QSettings::IniFormat)
#endif
{
    ui->setupUi(this);
    
    qDebug()<<settingFile.fileName()<<settingFile.isWritable();
    
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
    
    // 数据列显示
    actionGroupColumn.addAction(ui->actionColumnTime);
    actionGroupColumn.addAction(ui->actionColumnProcess);
    actionGroupColumn.addAction(ui->actionColumnThread);
    actionGroupColumn.addAction(ui->actionColumnContent);
    actionGroupColumn.setExclusive(false);
    
    connect(ui->actionColumnTime, &QAction::toggled, [this](bool b){ui->tableView->setColumnHidden(0, !b);} );
    connect(ui->actionColumnProcess, &QAction::toggled, [this](bool b){ui->tableView->setColumnHidden(1, !b);} );
    connect(ui->actionColumnThread, &QAction::toggled, [this](bool b){ui->tableView->setColumnHidden(2, !b);} );
    connect(ui->actionColumnContent, &QAction::toggled, [this](bool b){ui->tableView->setColumnHidden(3, !b);} );
    for(auto action:actionGroupColumn.actions())
        action->setChecked(true);
    
    // 进程选择
    QString processNames[] = {"AwemeInhouse", "IESVideoEditorDemo", "XIGVideoEditor_Example"};
    for(auto processName : processNames)
    {
        QAction *action = actionGroupProcess.addAction(processName);
        action->setCheckable(true);
        ui->menu_2->addAction(action);
        
        QString settingName = settingFile.value(SETTING_PROCESS_NAME).toString();
        if(settingName.size() > 0 && settingName == processName)
            action->setChecked(true);
        
        connect(action, &QAction::toggled, this, &MainWindow::ChangeProcess);
    }
    if(actionGroupProcess.checkedAction() == nullptr && actionGroupProcess.actions().size() > 0)
        actionGroupProcess.actions()[0]->setChecked(true);
    
    
    qDebug()<<QCoreApplication::arguments();
    statusBar()->showMessage(QCoreApplication::arguments().join(' '));
}

MainWindow::~MainWindow()
{
    Start(false);
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Start(false);
    logProcess.terminate();
}

void MainWindow::Start(bool b)
{
    if(b == true)
    {
        QStringList arguments;
        if(settings.processName.isEmpty() == false)
            arguments << "-p" << settings.processName;
        qDebug() << arguments;
        
        logProcess.start("/usr/local/bin/idevicesyslog", arguments);
        if(logProcess.waitForStarted())
        {
            qDebug() << "logProcess started";
            this->statusBar()->showMessage("logProcess started");
            this->setWindowTitle(arguments.join(' '));
            
            int lineCnt = 0;
            QRegularExpression splitPattern(R"((\w+ \d+ \d{2}:\d{2}:\d{2}) (\S+)\[(\d+)] <\w+>: (.*))");//(R"((\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d{6}\+\d{4}) ([a-zA-Z]+)\[(\d+:\d+)] )"); // 分隔用的正则表达式
            //splitPattern.namedCaptureGroups()
            QRegularExpression formatPattern(R"((?P<VESDK>[\S]+) )"
                                  R"((?P<Function>.*) )"
                                  R"(\[(?P<Line>\d+)] )"
                                  R"(-- (?P<Content>[\s\S]*)')");
            QString strContent;
            QFile logFile("device.log");
            logFile.open(QFile::WriteOnly);
            
            QString filterPattern = ui->lineEdit->text();
            
            while(ui->actionStart->isChecked() == true)
            {
                QByteArray arr = logProcess.readLine();
                QDateTime readTime = QDateTime::currentDateTime();
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
                    isScrollToEnd = ui->tableView->verticalScrollBar()->value()>ui->tableView->verticalScrollBar()->maximum()-
                    ui->tableView->rowHeight(model.rowCount()-1);
                    if((match.hasMatch() && match.lastCapturedIndex() == 4)) // 成功匹配到新log的开头
                    {
                        QList<QStandardItem *>itemRow;
                        qDebug()<<"End   " << strContent << itemRow;
                        
                        //TODO:这里重新格式化一下
                        strContent = match.captured(match.lastCapturedIndex());
                        itemRow << new QStandardItem(readTime.toString("hh:mm:ss.zzz"))//new QStandardItem(match.captured(1))
                            << new QStandardItem(match.captured(2))
                            << new QStandardItem(match.captured(3))
                            << new QStandardItem(strContent);
                        model.appendRow(itemRow);
                        
                        ui->tableView->resizeRowToContents(model.rowCount()-1);
                        if(isScrollToEnd)
                            ui->tableView->scrollToBottom();
                        itemRow.clear();
                        if(filterPattern.size() > 0) {
                            if(strContent.contains(filterPattern) == true)
                                ui->tableView->hideRow(model.rowCount() - 1);
                        }

                        qDebug() << strContent << itemRow;
                        lineCnt++;
                        QThread::msleep(10);
                    }
                    else // 没有匹配到，则只是追加
                    {
                        if(str.isEmpty() == false && str!="\n")
                        {
                            strContent += str;
                            QModelIndex index = model.index(model.rowCount()-1,model.columnCount()-1);
                            
                            model.setData(index, strContent);
                            qDebug()<<"======="<<index<<strContent;
//                            if(isScrollToEnd)
//                                ui->tableView->scrollToBottom();
                        }
                    }
                }
                // 筛选符合条件的
                if(ui->lineEdit->text() != filterPattern){
                    RefreshView(model, *ui->tableView, ui->lineEdit->text());
                    filterPattern = ui->lineEdit->text();
                }
                
                QCoreApplication::processEvents();
                QThread::msleep(1);
            }
            logFile.close();
        }
    }
    logProcess.close();
    logProcess.waitForFinished();
    
    ui->actionStart->setChecked(false);
    qDebug() << "logProcess closed";
    this->statusBar()->showMessage("logProcess closed");
}

void MainWindow::RefreshView(const QStandardItemModel &model, QTableView &view, QString pattern) {
    qDebug()<<__FUNCTION__<<pattern;
    for(int i = 0; i < model.rowCount(); i++)
    {
        QModelIndex index = model.index(i, 3);
        QString str = model.data(index).toString();
        if(str.contains(pattern))
            view.showRow(i);
        else
            view.hideRow(i);
    }
}


void MainWindow::ChangeProcess(bool isChecked)
{
    if(isChecked == true && sender()!= nullptr && sender()->metaObject()!=nullptr
       && sender()->metaObject()->className() == QString("QAction"))
        settings.processName = qobject_cast<QAction*>(sender())->text();
    else
        settings.processName.clear();
    qDebug()<<"切换进程为:"<<settings.processName;
    statusBar()->showMessage(QString("切换进程为:") + settings.processName);
    
    if(settings.processName == "AwemeInhouse")
    {
        
    }
    else if(settings.processName == "IESVideoEditorDemo")
    {
        
    }
    else if(settings.processName == "AwemeInhouseRelease")
    {
        
    }
    settingFile.setValue(SETTING_PROCESS_NAME, settings.processName);
    settingFile.sync();
    Start(false);
}
