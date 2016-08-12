#ifndef TASKBASE_H
#define TASKBASE_H

#include <QObject>
#include <QtCore>

/* 任务基类
 * 主要是统一接口
 */
class TaskBase : public QObject
{
    Q_OBJECT
public:
    enum Type{HTTP=0,FTP};
    enum State{Downloading = 0,Finished,Stoped};
    TaskBase(QObject *parent = 0);
    virtual ~TaskBase();
    virtual void SetThreadCount(int){}                          //设置下载线程数
    virtual void SetMaxSpeed(qint64){}                          //设置下载速度
    virtual void Start(){}                                      // 开始 / 继续下载
    virtual void Offline(){}                                    //离线，下载时关闭程序前调用
    virtual void FinishedSlot(int){}                            //下载完成
    virtual void ProgressSlot(qint64){}                         //下载过程
    virtual void Stop(){}                                       //停止

    virtual qint64 GetFileSize(){return 0;}
    virtual qint64 GetReadySize(){return 0;}
    virtual QString GetUrl(){return 0;}
    virtual QString GetPath(){return 0;}
    virtual QString GetFilename() {return "";}
    virtual State GetState() {return Stoped;}
    virtual Type GetType(){return HTTP;}
};

#endif // TASKBASE_H
