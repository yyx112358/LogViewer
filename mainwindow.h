#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QStandardItemModel>
#include <QActionGroup>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QTableView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QActionGroup actionGroupProcess, actionGroupColumn;
    
    QProcess logProcess;
    QStandardItemModel model;
    struct
    {
        QString processName;
    }settings;
    
    void closeEvent(QCloseEvent *event);
    
    void RefreshView(const QStandardItemModel &model, QTableView &view, QString pattern = "");
    
    void ChangeProcess(bool isChecked);
    void Start(bool b);
};
#endif // MAINWINDOW_H
