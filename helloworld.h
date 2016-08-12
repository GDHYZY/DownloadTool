#ifndef HELLOWORLD_H
#define HELLOWORLD_H

#include <QWidget>
#include <QCloseEvent>
#include <QMenu>
#include <QAction>
#include <QMouseEvent>
#include "taskmanager.h"

namespace Ui {
class HelloWorld;
}

/* 主界面类
 * 用于显示内容
 */
class HelloWorld : public QWidget
{
    Q_OBJECT
    enum View{DOWNLOADING = 0,FINISHED,DELETED};        //视图状态，包括下载视图，已完成视图，回收站视图
public:
    /*TabelWidget中的右键菜单以及选项*/
    QMenu* menu;
    QAction* deleteaction;
    QAction* speedaction;
    QAction* deleteallaction;
    QAction* restartaction;
private:
    Ui::HelloWorld *ui;
    /*位置*/
    QPoint mousepoint;
    TaskManager* manager;
    /*时钟*/
    QTimer timer;
    /*视图状态*/
    View view;
public:
    explicit HelloWorld(QWidget *parent = 0);
    ~HelloWorld();

    /*初始化各种控件和参数*/
    void Init();
private:
    /*
     * 重载关闭事件，鼠标事件
     */
    void closeEvent(QCloseEvent *event);
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    /*构造TabelWidget*/
    void _buildtabel();
private slots:
    /*更新界面，与时钟信号连接，每秒回调一次*/
    void updateview();
    /*完成一个任务，与任务管理器类的完成信号连接*/
    void FinishedSlot();
    /*各种按钮的回调函数*/
    void on_body_cellDoubleClicked(int row, int column);
    void on_body_customContextMenuRequested(const QPoint &pos);
    void action_setMaxSpeedSlot();
    void action_deleteTaskSlot();
    void action_deleteAllSlot();
    void action_restartSlot();
    void on_BTN_WINDOW_CLOSE_clicked();
    void on_BTN_WINDOW_MISS_clicked();
    void on_BTN_STARTTASKS_clicked();
    void on_BTN_STOPTASKS_clicked();
    void on_BTN_VIEW_DOWNLOAD_clicked();
    void on_BTN_VIEW_FINISHED_clicked();
    void on_BTN_VIEW_DELETE_clicked();
    void on_BTN_FULLSPEED_clicked();
    void on_BTN_CREATTASKS_clicked();
    void on_EDIT_SETSPEED_returnPressed();
};

#endif // HELLOWORLD_H
