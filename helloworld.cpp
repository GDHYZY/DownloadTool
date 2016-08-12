#include "helloworld.h"
#include "ui_helloworld.h"
#include <QtCore>
#include<QMessageBox>
#include <QInputDialog>
#include <QProgressBar>
#include <QHeaderView>
#include <QScrollBar>

HelloWorld::HelloWorld(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HelloWorld)
{
    ui->setupUi(this);
    manager = new TaskManager(this);
    view = DOWNLOADING;
    Init();
}

void HelloWorld::Init()
{
    /*设置主界面*/
    this->setWindowFlags(Qt::FramelessWindowHint); //设置无边框
    this->setStyleSheet("QWidget{background-image: url(:/win/icon/bg.jpg);border: 1px;}");

    /*设置按钮*/
    QFile file(":/qss/qss/pushbutton.qss");
    file.open(QFile::ReadOnly);
    QString btn = file.readAll();
    file.close();
    ui->BTN_WINDOW_CLOSE->setStyleSheet(btn);
    ui->BTN_WINDOW_MISS->setStyleSheet(btn);
    ui->BTN_VIEW_DOWNLOAD->setStyleSheet(btn);
    ui->BTN_VIEW_FINISHED->setStyleSheet(btn);
    ui->BTN_VIEW_DELETE->setStyleSheet(btn);
    ui->BTN_STARTTASKS->setStyleSheet(btn);
    ui->BTN_STOPTASKS->setStyleSheet(btn);
    ui->BTN_CREATTASKS->setStyleSheet(btn);
    ui->BTN_FULLSPEED->setStyleSheet(btn);
    ui->LABEL_LINE_1->setVisible(true);
    ui->LABEL_LINE_2->setVisible(false);
    ui->LABEL_LINE_3->setVisible(false);

    /*设置限速框*/
    QIntValidator* intvalidator = new QIntValidator;
    intvalidator->setBottom(1);
    ui->EDIT_SETSPEED->setValidator(intvalidator);

    /*设置tablewidget*/
    QFile file1(":/qss/qss/tablewidget.qss");
    file1.open(QFile::ReadOnly);
    QString tabel = file1.readAll();
    ui->body->setStyleSheet(tabel);
    ui->body->setEditTriggers(QAbstractItemView::NoEditTriggers);   //禁止编辑
    ui->body->setSelectionBehavior(QAbstractItemView::SelectRows);  //整行选中的方式
    ui->body->verticalHeader()->setVisible(false);
    ui->body->horizontalHeader()->setVisible(false);
    ui->body->setFrameShape(QFrame::NoFrame);      //设置无边框
    ui->body->setShowGrid(false);                //设置不显示格子线
    ui->body->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->body->horizontalScrollBar()->setVisible(false);

    //设置右键菜单
    menu = new QMenu(ui->body);
    restartaction = new QAction(tr("重新下载"),this);
    deleteaction = new QAction(tr("删除任务"),this);
    speedaction = new QAction(tr("限速"),this);
    deleteallaction = new QAction(tr("清空列表"),this);
    menu->addAction(speedaction);
    menu->addAction(deleteaction);

    /*连接信号槽*/
    connect(manager,SIGNAL(FinishedSignal()),this,SLOT(FinishedSlot()));
    connect(&timer,SIGNAL(timeout()),this,SLOT(updateview()));
    connect(deleteaction,SIGNAL(triggered(bool)),this,SLOT(action_deleteTaskSlot()));
    connect(deleteallaction,SIGNAL(triggered(bool)),this,SLOT(action_deleteAllSlot()));
    connect(speedaction,SIGNAL(triggered(bool)),this,SLOT(action_setMaxSpeedSlot()));
    connect(restartaction,SIGNAL(triggered(bool)),this,SLOT(action_restartSlot()));

    _buildtabel();
    timer.start(1000);
}

HelloWorld::~HelloWorld()
{
    delete menu;
    delete deleteallaction;
    delete deleteaction;
    delete restartaction;
    delete speedaction;
    delete manager;
    delete ui;
}

void HelloWorld::closeEvent(QCloseEvent *event)
{
    if(manager)
        manager->Finished();
    event->accept();
}

void HelloWorld::mouseMoveEvent(QMouseEvent *e)
{
    if(e->buttons() && Qt::LeftButton)
    {
        this->move(e->globalPos() - mousepoint);
        e->accept();
    }
}

void HelloWorld::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
    {
        mousepoint = e->globalPos() - this->pos();
        e->accept();
    }
}

void HelloWorld::updateview()
{
    ui->lspeed->setText(manager->GetGloableSpeed());
    QVector<Message> ans = manager->GetDates(view);
    switch(view)
    {
    case DOWNLOADING:
        for(int i = 0;i < ans.size(); i++)
        {
            ((QProgressBar*)ui->body->cellWidget(i,1))->setValue( 1.0 * ans[i].readysize.toLongLong() / ans[i].ifilesize * 100);
            if(ans[i].state == TaskBase::Stoped)
            {
                ui->body->item(i,2)->setText(QString(tr("暂停")));
            }
            else
                ui->body->item(i,2)->setText(ans[i].speed);
            ui->body->item(i,3)->setText((tr("剩余时间：%1").arg(ans[i].date)));
        }
        break;
    default:
        break;
    }
}

void HelloWorld::FinishedSlot()
{
    _buildtabel();
}

void HelloWorld::_buildtabel()
{
    ui->lspeed->setText(manager->GetGloableSpeed());
    QVector<Message> ans = manager->GetDates(view);
    ui->body->clear();
    switch(view)
    {
    case DOWNLOADING:
        ui->body->setColumnCount(4);
        ui->body->setRowCount(ans.size());
        for(int i = 0;i < ans.size(); i++)
        {
            ui->body->setItem(i,0,new QTableWidgetItem(QString("%1\n%2").arg(ans[i].filename).arg(ans[i].filesize)));
            QProgressBar* progressbar = new QProgressBar();
            progressbar->setStyleSheet("QProgressBar{border:1px solid black;text-align:center;}"
                                       "QProgressBar::chunk{background-color: rgb(85, 170, 255);}");
            progressbar->setMaximum(100);
            progressbar->setMinimum(0);
            progressbar->setValue( 1.0 * ans[i].readysize.toLongLong() / ans[i].ifilesize * 100);
            ui->body->setCellWidget(i,1,progressbar);
            if(ans[i].state == TaskBase::Stoped)
            {
                ui->body->setItem(i,2,new QTableWidgetItem(QString(tr("暂停"))));
            }
            else
                ui->body->setItem(i,2,new QTableWidgetItem(ans[i].speed));
            ui->body->setItem(i,3,new QTableWidgetItem(tr("剩余时间：%1").arg(ans[i].date)));
        }
        break;
    default:
        ui->body->setColumnCount(2);
        ui->body->setRowCount(ans.size());
        for(int i = 0;i < ans.size(); i++)
        {
            ui->body->setItem(i,0,new QTableWidgetItem(QString("%1\n%2").arg(ans[i].filename).arg(ans[i].filesize)));
            ui->body->setItem(i,1,new QTableWidgetItem(ans[i].date));
        }
        break;
    }
    ui->body->resizeRowsToContents();
    ui->body->horizontalHeader()->resizeSections(QHeaderView::Stretch);//使列完全填充并平分
}

void HelloWorld::on_body_cellDoubleClicked(int row, int column)
{
    manager->Response(row,view);
    if(view != DOWNLOADING)
        _buildtabel();
    else
        updateview();
}


void HelloWorld::on_body_customContextMenuRequested(const QPoint &pos)
{
    if(ui->body->currentItem() && ui->body->currentItem()->isSelected())
    {
        qDebug() << tr("当前行号%1").arg(ui->body->currentRow());
        menu->clear();
        switch(view)
        {
        case DOWNLOADING:
            menu->addAction(speedaction);
            menu->addAction(deleteaction);
            menu->exec(QCursor::pos());
            break;
        default:
            menu->addAction(restartaction);
            menu->addAction(deleteaction);
            menu->addAction(deleteallaction);
            menu->exec(QCursor::pos());
            break;
        }
    }
}

void HelloWorld::action_setMaxSpeedSlot()
{
    bool ok = false;
    qint64 speed = QInputDialog::getText(this,tr("任务限速"),tr("请输入最大速度（单位为KB/S）")).toLongLong(&ok);
    if(ok)
    {
        qDebug()<< tr("第%1个任务限速为%2KB/S").arg(ui->body->currentRow()+1).arg(speed);
        manager->SetMaxSpeed(ui->body->currentRow(),speed);
    }
}

void HelloWorld::action_deleteTaskSlot()
{
    qDebug()<< tr("删除第%1个任务").arg(ui->body->currentRow()+1);
    manager->DeleteTasks(ui->body->currentRow(),(TaskManager::ROOM)view);
    _buildtabel();
}

void HelloWorld::action_deleteAllSlot()
{
    qDebug()<< tr("清空列表");
    manager->ClearTasks((TaskManager::ROOM)view);
    _buildtabel();
}

void HelloWorld::action_restartSlot()
{
    qDebug()<< tr("重新开始任务");
    manager->Response(ui->body->currentRow(),view);
    _buildtabel();
}

void HelloWorld::on_BTN_WINDOW_CLOSE_clicked()
{
    this->close();
}

void HelloWorld::on_BTN_WINDOW_MISS_clicked()
{
    this->showMinimized();
}

void HelloWorld::on_BTN_STARTTASKS_clicked()
{
    manager->StartTasks();
}

void HelloWorld::on_BTN_STOPTASKS_clicked()
{
    manager->StopTasks();
}

void HelloWorld::on_BTN_VIEW_DOWNLOAD_clicked()
{
    if(view == DOWNLOADING)
        return;
    view = DOWNLOADING;
    ui->LABEL_LINE_1->setVisible(true);
    ui->LABEL_LINE_2->setVisible(false);
    ui->LABEL_LINE_3->setVisible(false);
    _buildtabel();
}

void HelloWorld::on_BTN_VIEW_FINISHED_clicked()
{
    if(view == FINISHED)
        return;
    view = FINISHED;
    ui->LABEL_LINE_1->setVisible(false);
    ui->LABEL_LINE_2->setVisible(true);
    ui->LABEL_LINE_3->setVisible(false);
    _buildtabel();
}

void HelloWorld::on_BTN_VIEW_DELETE_clicked()
{
    if(view == DELETED)
        return;
    view = DELETED;
    ui->LABEL_LINE_1->setVisible(false);
    ui->LABEL_LINE_2->setVisible(false);
    ui->LABEL_LINE_3->setVisible(true);
    _buildtabel();
}

void HelloWorld::on_BTN_FULLSPEED_clicked()
{
    qDebug()<< tr("全速下载");
    manager->SetMaxSpeed(-1);
}

void HelloWorld::on_BTN_CREATTASKS_clicked()
{
    QString url = ui->lineURL->text();
    QString path = ui->linePath->text();
    QString count = ui->lineThreadCount->text();
    if(url == "")
    {
        QMessageBox::about(NULL,"About","请输入URL");
        return;
    }
    if(path == "")
    {
        QMessageBox::about(NULL,"About","请输入路径");
        return;
    }
    if(count == "0" || count == "")
    {
        QMessageBox::about(NULL,"About","线程数不能为0或置空");
        return;
    }
    QVector<QVector<QString> >res;
    QVector<QString> tmp;
    tmp.clear();
    res.clear();
    tmp.append(url); tmp.append(path);tmp.append(count);
    if(url[0] == 'f')
    {
        QString un = QInputDialog::getText(NULL,"username","username");
        QString pw = QInputDialog::getText(NULL,"password","password");
        tmp.append(un);
        tmp.append(pw);
    }
    res.append(tmp);
    manager->CreateTasks(res);

    view = DOWNLOADING;
    _buildtabel();
}

void HelloWorld::on_EDIT_SETSPEED_returnPressed()
{
    bool ok = false;
    qint64 speed = ui->EDIT_SETSPEED->text().toLongLong(&ok);
    if(ok)
    {
        qDebug()<< tr("全局限速为%1KB/S").arg(speed);
        manager->SetMaxSpeed(speed);
    }
}
