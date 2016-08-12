#ifndef DOWNLOAD
#define DOWNLOAD

#include <QtCore>
#include <QtNetwork>

/*
 * Http下载线程类,只负责下载文件段s
 */
class Download : public QObject
{
    Q_OBJECT
public:
    enum State{Downloading,Finished,Stoped};        //三种状态
    const int m_ID;                                 //线程标志
    int m_Index;                                    //下载块序号
    qint64 m_HaveDoneBytes;                         //已下载大小
    qint64 m_StartPoint;                            //起始点
    qint64 m_EndPoint;                              //终止点

private:
    QNetworkAccessManager m_Qnam;
    QNetworkReply *m_Reply;
    QFile *m_File;
    State m_State;                                  //状态
public:
    Download(int id,QObject *parent = 0);
    void Start(int index, QUrl &url, QFile *file,
                       qint64 startPoint, qint64 endPoint);
    void Stop();                                    //暂停
private:
    void _DownloadFinished();                       //完成后处理变量
signals:
    void FinishedSignal(int index);                 //下载完毕信号
    void ProgressSignal(qint64 readySize);          //向上传递下载过程信号

private slots:
    void FinishedSlot();                            //下载完成
    void HttpReadyReadSlot();
};

#endif // DOWNLOAD

