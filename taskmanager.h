#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QtCore>
#include <QSettings>
#include "taskbase.h"
#include "httptask.h"
#include "ftptask.h"

/*用于传输到GUI显示的信息
 * ifilesize主要用于记录下载大小，便于计算下载速度，不用于显示
 * filename,filesize，readysize为文件大小和已下载大小
 * date 在下载队列中表示剩余时间，在完成队列及垃圾箱中表示完成/删除日期
 * speed 在下载队列中表示下载速度，特别的，在任务暂停时显示为“暂停”。
 *
 * 完成队列和垃圾箱只显示文件名称、文件大小以及处理日期
 */
struct Message{
    qint64 ifilesize;
    QString filename;
    QString filesize;
    QString date;
    TaskBase::State state;
    QString readysize;
    QString speed;
};

/* 任务管理器类
 * 主要用于处理协调各个下载任务，采集下载信息，向GUI层提供信息
 */
class TaskManager : public QObject
{
    Q_OBJECT
public:
    /*ROOM枚举，用于识别下载队列、完成队列、垃圾箱*/
    enum ROOM{LIVING = 0,FINISHED,DELETED};
private:
    /*完成队列及垃圾箱中任务的必要信息，管理类可以拿这些信息重新开始任务*/
    class TaskMsg
    {
    public:
        QString m_Filename;             //文件名
        qint64 m_Size;                  //文件大小
        QString m_Time;                 //处理日期
        TaskBase::Type m_Type;          //任务类型
        QString m_Url;                  //文件url
        QString m_Path;                 //输出文件路径
        TaskMsg(TaskBase::Type type,QString filename,QString url,QString path,QString time,qint64 size)
            : m_Type(type),m_Filename(filename),m_Url(url),m_Path(path),m_Time(time),m_Size(size){}
    };

    int m_ThreadCount;                  //线程数
    QVector<TaskBase*> download;        //下载列表中的任务
    QVector<TaskMsg*> finished;         //已完成的任务
    QVector<TaskMsg*> deleted;          //垃圾箱中的任务

    QVector<Message> livingmsg;         //用于GUI显示的数据
public:
    explicit TaskManager(QObject *parent = 0);
//接口
    /*创建任务（支持多任务创建）*/
    void CreateTasks(QVector<QVector<QString> >tasks);          // 0:url 1:path 2:threadcount
    /*开始任务*/
    void StartTasks();
    void StartTasks(int num,ROOM room);
    /*停止任务*/
    void StopTasks();
    void StopTasks(int num);
    /*删除任务或表项*/
    void DeleteTasks(int num,ROOM room);
    /*清除列表*/
    void ClearTasks(ROOM room);
    /*限速*/
    void SetMaxSpeed(qint64 speed);
    void SetMaxSpeed(int num, qint64 speed);
    /*结束程序*/
    void Finished();
    /*用于响应开始/暂停的切换*/
    void Response(int num , int tps);

//GUI交互
    QString GetGloableSpeed();
    QVector<Message> GetDates(int state);                  //获取任务信息

private:
    void _FindMarkFile();                                   //找回下载状态
    void _MarkTakstoFile();                                 //将状态记录进文件
//单位转换小工具
    QString _TSize(qint64 bytes);
    QString _TSpeed(double speed);
    QString _TTimeformat(int seconds);
signals:
    void FinishedSignal();                                  //任务完成信号
    void ProgressSignal();                                  //过程信号
public slots:
    void FinishedSlot();            //一个任务完成
};

#endif // TASKMANAGER_H
