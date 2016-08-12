#ifndef FTPTASK_H
#define FTPTASK_H

#include <QObject>
#include <QtCore>
#include <unistd.h>
#include "pthread.h"
#include "taskbase.h"
#include "curl/curl.h"
#include <QThread>

class FTPTask;
//static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
//static pthread_mutex_t g_mutex_write = PTHREAD_MUTEX_INITIALIZER;
//static pthread_mutex_t g_mutex_thread = PTHREAD_MUTEX_INITIALIZER;
/*
 * 线程变量，是下载线程的数据收集
 */
struct tNode
{
    pthread_t tid;                              //线程ID
    qint64 id;                                  //线程管理队列内ID
    qint64 index;                               //文件块ID
    qint64 startpoint;                          //起始点
    qint64 readysize;                           //已下载大小
    qint64 endpoint;                            //结束点
    void* curl;                                 //curl句柄
    FTPTask* parent;                            //指向本类
    QFile* file;                                //指向输出文件
};

/*
 * FTP下载类
 * 继承自TaskBase
 */
class FTPTask : public TaskBase
{
    Q_OBJECT
public:
    qint64 threadCnt ;
    QMutex m_Mutex;
    QMutex m_Mutex_Write;
//    QMutex m_Mutex_thread;
private:
    qint64 m_FileSize;                      //文件大小
    qint64 m_ReadySize;                     //已下载大小
    qint64 m_ThreadCount;                   //线程数
    qint64 m_MaxSpeed;                      //最大速度
    QFile* m_File;                          //输出文件
    QString m_Path;
    QUrl m_Url;                             //文件URL

    State m_State;                          //状态
    Type m_Type;                            //任务类型
    QVector<qint64> freeblocks;             //分块
    QVector<TaskBase::State> blockstates;   //块状态
    QVector<tNode*> threads;                //子线程

public:
    explicit FTPTask(Type type,QString url, QString path, QString username, QString password,int count,QObject *parent = 0);
    ~FTPTask();
    void Start();                                         //开始下载
    void Offline();                                       //离线，下载时关闭程序前调用
    void Stop();                                          //停止
    void SetMaxSpeed(qint64 _kbps);                       //设置下载速度

    qint64 GetFileSize(){return m_FileSize;}
    qint64 GetReadySize(){return m_ReadySize;}
    QString GetUrl(){return m_Url.toString();}
    QString GetPath(){return m_Path;}
    State GetState(){return m_State;}
    Type GetType(){return m_Type;}
    QString GetFilename() {return QFileInfo(m_Path).fileName();}



private:
    qint64 _GetFileSizeFromUrl() ;
    void _FixLogFile();                                 //修正记录文件
    bool _FindLogFile();                                //读取记录文件
    void _ClearThreads();
    void _RebuildDownloadBlocks();                      //重整下载空间
    void _Start();                                      //根据空闲块开始任务
    void _DownloadNext(qint64 id);                      //分派下一份任务

    /*写文件操作,libcurl库的回调函数*/
    static size_t Filewrite(void *buffer, size_t size, size_t nmemb,void *stream);
signals:
    void FinishedSignal();
    void _FinishedSignal(qint64 id);
public slots:
    void FinishedSlot(qint64 id);            //下载完成
    void DownloadNextSlot();                 //限速时候控制下载下一片

};

#endif // FTPTASK_H
