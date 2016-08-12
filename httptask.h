#ifndef HTTPTask_H
#define HTTPTask_H

#include <QtCore>
#include <QtNetwork>
#include "download.h"
#include "Taskbase.h"

/* HTTP下载类
 * 负责HTTP源的下载
 */
class HTTPTask : public TaskBase
{
    Q_OBJECT
private:
    int m_ThreadCount;                                      //下载线程数
    qint64 m_ReadySize;                                     //已下载大小
    qint64 m_FileSize;                                      //文件总大小
    qint64 m_MaxSpeed;                                      //最大速度

    QUrl m_Url;                                             //下载链接
    QString m_Path;                                         //文件路径
    QFile *m_File;                                          //输出文件
    State m_State;                                          //任务状态
    Type m_Type;                                            //任务类型
    QVector<Download*> threads;                             //下载线程

    /* 块和块状态数组是用于HTTP任务的切片和线程任务管理派发
     * 暂停或者结束都会记录这些信息以断点重传
     */
    QVector<qint64> freeblocks;                             //分块,记录尚未被下载的区间
    QVector<Download::State> blockstates;                   //块状态,记录对应区间的状态
public:
    HTTPTask(Type type,QString url, QString path, int count, QObject *parent = 0);
    ~HTTPTask();
//功能
    void Start();                                           //开始下载
    void Stop();                                            //停止下载
    void Offline();                                         //离线，下载时关闭程序前调用
    void SetThreadCount(int count);                         //设置下载线程数
    void SetMaxSpeed(qint64 _kbps);                         //设置速度上限

//信息输出接口
    qint64 GetReadySize(){return m_ReadySize;}
    qint64 GetFileSize() {return m_FileSize;}
    State GetState(){return m_State;}
    Type GetType(){return m_Type;}
    QString GetUrl(){return m_Url.toString();}
    QString GetPath(){return m_Path;}
    QString GetFilename() {return QFileInfo(m_Path).fileName();}
private:
    void _Start();                                          //通过加载配置文件续传
    qint64 _GetFileSizeFromUrl();                           //获取文件大小
    void _ClearThreads();                                   //清除下载线程
    void _RebuildDownloadBlocks();                          //用于重整下载空间
    void _FixLogFile();                                     //修正记录文件
    bool _FindLogFile();                                    //加载记录文件

signals:
    void FinishedSignal();                                  //完成任务下载信号

private slots:
    void FinishedSlot(int index);                           //下载完成
    void ProgressSlot(qint64 readySize);                    //下载过程(用于迭代已下载的大小)

};

#endif // HTTPTask_H
