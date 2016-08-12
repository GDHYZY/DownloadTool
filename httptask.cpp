#include "httptask.h"


HTTPTask::HTTPTask(Type type, QString url, QString path, int count, QObject *parent)
    :m_Type(type),m_Path(path),m_ThreadCount(count),TaskBase(parent)
{
    m_File = NULL;
    m_State = Stoped;
    m_MaxSpeed = -1;
    m_ReadySize = 0;
    m_FileSize = 0;
    m_Url.setUrl(url);
    if(!_FindLogFile())
        m_FileSize = _GetFileSizeFromUrl();

}

HTTPTask::~HTTPTask()
{
    if(m_File)
        delete m_File;
    foreach(Download* thread,threads)
    {
        thread->deleteLater();
    }
    threads.clear();
}

/*设置线程数*/
void HTTPTask::SetThreadCount(int count)
{
    if(count < 1 && count == m_ThreadCount)
        return;

    if(Stoped == m_State)                                             //暂停后修改了线程数
    {
        m_ThreadCount = count;
        return;
    }

    _ClearThreads();                                                  //清除下载线程

    m_ThreadCount = count;                                            //重置进程数

    _RebuildDownloadBlocks();                                         //重整下载块

    for(int i = 0;i < m_ThreadCount; i++)
    {
        qint64 startPoint = freeblocks[2*i];
        qint64 endPoint = freeblocks[2*i+1];
        blockstates[i] = Download::Downloading;                        //设置分块为正在下载

        Download *thread = new Download(i,this);
        connect(thread,SIGNAL(FinishedSignal(int)),SLOT(FinishedSlot(int)));
        connect(thread,SIGNAL(ProgressSignal(qint64)),SLOT(ProgressSlot(qint64)));
        thread->Start(i ,m_Url,m_File,startPoint,endPoint);
        threads.append(thread);
    }
}

/*设置速度上限*/
void HTTPTask::SetMaxSpeed(qint64 _kbps)
{
    if(_kbps < 0 && m_MaxSpeed < 0)
        return ;

    qint64 kbps = _kbps * 1024;

    if(kbps == m_MaxSpeed)
        return;

    if(m_State != Downloading)                                        //暂停后修改了线程数
    {
        m_MaxSpeed = kbps;
        return;
    }

    _ClearThreads();                                                  //清除下载线程

    m_MaxSpeed = kbps;                                                //重置进程数

    _RebuildDownloadBlocks();                                         //重整下载块

    for(int i = 0;i < m_ThreadCount && i < freeblocks.size()/2; i++)
    {
        qint64 startPoint = freeblocks[2*i];
        qint64 endPoint = freeblocks[2*i+1];
        blockstates[i] = Download::Downloading;                        //设置分块为正在下载

        Download *thread = new Download(i,this);
        connect(thread,SIGNAL(FinishedSignal(int)),SLOT(FinishedSlot(int)));
        connect(thread,SIGNAL(ProgressSignal(qint64)),SLOT(ProgressSlot(qint64)));
        thread->Start(i ,m_Url,m_File,startPoint,endPoint);
        threads.append(thread);
    }
}

/*重整空闲区间，修改freeblocks和blockstates信息*/
void HTTPTask::_RebuildDownloadBlocks()
{
    qint64 pear = 0;
    if(m_MaxSpeed > 0)                                                  //若有限速，根据速度切块
        pear = m_MaxSpeed / m_ThreadCount;
    else                                                                //若无限速，根据线程切块
    {
        qint64 size = 0;
        int i = 0;
        for(i = 0;i < freeblocks.size() / 2;i++)
        {
            size += (freeblocks[2*i+1] - freeblocks[2*i]);
        }
        pear = size / m_ThreadCount;
    }
    QVector<qint64> tmps;
    tmps.clear();
    for(qint64 i = 0;i < freeblocks.size()/2;)
    {
        qint64 start = freeblocks[2*i];
        qint64 end = freeblocks[2*i+1];
        if(start >= end)            //包括（0,0）的情况
        {
            ++i;
            continue;
        }
        for(qint64 j = i + 1;j < freeblocks.size()/2;j++)          //向后遍历连接分区
        {
            if(freeblocks[2*(j-1)+1] != freeblocks[2*j])
                break;
            end = freeblocks[2*j+1];
            i = j;
        }
        while(1)
        {
            if(start + pear >= end)
            {
                tmps.append(start);
                tmps.append(end);
                ++i;
                break;
            }
            else
            {
                tmps.append(start);
                tmps.append(start + pear);
                start += pear;
            }
        }
    }
    freeblocks.clear();
    blockstates.clear();
    for(qint64 i = 0;i < tmps.size();i++)      //修正块信息为全部有效,状态为暂停
    {
        freeblocks.append(tmps[i]);
        blockstates.append(Download::Stoped);
    }
    tmps.clear();
}

/*重写日志文件*/
void HTTPTask::_FixLogFile()
{
    QString iniFile = m_Path + ".setting";
    QSettings settings(iniFile,QSettings::IniFormat);

    settings.clear();

    settings.setValue("URL",m_Url);
    settings.setValue("FILEPATH",m_Path);
    settings.setValue("DOWNLOADCOUNT",m_ThreadCount);
    settings.setValue("FILESIZE",m_FileSize);
    settings.setValue("READYSIZE",m_ReadySize);
    for(int i = 0;i < freeblocks.size() / 2;i++)
    {
        QString index = QString::number(i + 1);
        settings.setValue("STARTPOINT" + index, freeblocks[2*i]);
        settings.setValue("ENDPOINT" + index, freeblocks[2*i+1]);
    }
}

/*读取日志文件*/
bool HTTPTask::_FindLogFile()
{
    if(QFile(m_Path).exists() && QFile(m_Path+".setting").exists())
    {
        QSettings settings(m_Path+".setting",QSettings::IniFormat);
        m_Url = settings.value("URL").toUrl();
        m_Path = settings.value("FILEPATH").toString();
        m_ThreadCount = settings.value("DOWNLOADCOUNT",-1).toInt();
        m_FileSize = settings.value("FILESIZE",-1).toLongLong();
        m_ReadySize = settings.value("READYSIZE").toLongLong();
        m_File = new QFile(m_Path,this);

        freeblocks.clear();
        blockstates.clear();

        for(int i = 0;;i++)
        {
            QString index = QString::number(i + 1);
            qint64 startPoint = settings.value("STARTPOINT" + index,-1).toLongLong();
            qint64 endPoint = settings.value("ENDPOINT" + index,-1).toLongLong();

            if(startPoint == -1 || endPoint == -1 )
            {
                break ;
            }
            freeblocks.append(startPoint);
            freeblocks.append(endPoint);
            blockstates.append(Download::Stoped);
        }
        return true;
    }
    return false;
}

/*获取下载文件大小*/
qint64 HTTPTask::_GetFileSizeFromUrl()
{
    qint64 size = -1;
    qDebug() << "Getting the file size...";
    QNetworkAccessManager manager;

    QEventLoop loop;
    //发出请求，获取目标地址的头部信息
    QNetworkReply *reply = manager.head(QNetworkRequest(QUrl(m_Url)));
    if(!reply)
    {
        QString error = tr("请求文件大小失败");
    }
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()), Qt::DirectConnection);
    loop.exec();
    if(reply->error() != QNetworkReply::NoError)
    {
        QString error = tr("请求文件大小失败");
    }
    QVariant var = reply->header(QNetworkRequest::ContentLengthHeader);
    reply->deleteLater();
    size = var.toLongLong();

    qDebug() << "The file size is: " << size;
    return size;
}

/*开始下载任务*/
void HTTPTask::Start()
{
    if(Downloading != m_State)
    {
        if(_FindLogFile()) //若配置文件存在
        {
            _Start();
            return;
        }

        if(m_FileSize <= 0)
        {
            qDebug() << tr("获取文件大小失败");
            return;
        }

        m_File = new QFile(m_Path,this);
        //打开文件
        if(!m_File->open(QIODevice::WriteOnly))
        {
            QString error = tr("无法打开文件!");
            m_File->close();
            m_File = NULL;
            return ;
        }
        qDebug()<<tr("打开文件");
        m_File->resize(m_FileSize);
        threads.clear();
        freeblocks.clear();
        blockstates.clear();
        qDebug() << "Start download file from " << m_Url;

        qint64 pear = m_FileSize / m_ThreadCount;

        for(int i = 0;i < m_ThreadCount;i++)
        {
            qint64 startPoint = pear * i;
            qint64 endPoint = (i==m_ThreadCount-1?m_FileSize : (pear * (i + 1)));
            freeblocks.append(startPoint);
            freeblocks.append(endPoint);
            blockstates.append(Download::Downloading);

            Download *thread = new Download(i,this);
            connect(thread,SIGNAL(FinishedSignal(int)),SLOT(FinishedSlot(int)));
            connect(thread,SIGNAL(ProgressSignal(qint64)),SLOT(ProgressSlot(qint64)));
            thread->Start(i,m_Url,m_File,startPoint,endPoint);
            threads.append(thread);
        }
        m_State = Downloading;
    }
}

/*根据块信息开始任务,用于读取日志文件后*/
void HTTPTask::_Start()
{
    if(Downloading != m_State)
    {
        m_File = new QFile(m_Path,this);
        if(!m_File->open(QFile::WriteOnly | QFile::Append))
        {
            QString error = tr("无法打开文件!");
            m_File->close();
            m_File = NULL;
            return ;
        }
        qDebug() << tr("打开文件");
        m_File->resize(m_FileSize);
        threads.clear();
        for(int i = 0;i < m_ThreadCount && i < freeblocks.size()/2; i++)
        {
            qint64 startPoint = freeblocks[2*i];
            qint64 endPoint = freeblocks[2*i+1];
            blockstates[i] = Download::Downloading;      //设置分块为正在下载

            Download *thread = new Download(i,this);
            connect(thread,SIGNAL(FinishedSignal(int)),SLOT(FinishedSlot(int)));
            connect(thread,SIGNAL(ProgressSignal(qint64)),SLOT(ProgressSlot(qint64)));
            thread->Start(i ,m_Url,m_File,startPoint,endPoint);
            threads.append(thread);
        }
        m_State = Downloading;
    }
}

/*任务离线*/
void HTTPTask::Offline()
{
    if(m_MaxSpeed > 0)
        m_MaxSpeed = -1;
    Stop();
}

/*清除下载线程,更新freeblocks块信息*/
void HTTPTask::_ClearThreads()
{
    foreach (Download *thread, threads) {
        thread->Stop();
    }

    for(int i = 0;i < threads.size();i++)
    {
        qint64 first = threads[i]->m_StartPoint + threads[i]->m_HaveDoneBytes;
        qint64 second = threads[i]->m_EndPoint;
        if(first >= second)
        {
            freeblocks[2 * threads[i]->m_Index] = 0;
            freeblocks[2 * threads[i]->m_Index + 1] = 0;
        }
        else
        {
            freeblocks[2 * threads[i]->m_Index] = first;
            freeblocks[2 * threads[i]->m_Index + 1] = second;
        }
        blockstates[threads[i]->m_Index] = Download::Stoped;        //分块状态改为暂停
    }
    foreach(Download *thread,threads)
        thread->deleteLater();
    threads.clear();
}

void HTTPTask::FinishedSlot(int index)
{
    //如果完成数等于文件段数，则说明文件下载完毕，关闭文件，发生信号
    if( m_FileSize <= m_ReadySize && m_State == Downloading)
    {
        m_State = Finished;
        m_File->flush();
        m_File->close();
        qDebug()<< tr("关闭文件");
        if(QFile::remove(m_File->fileName() + ".setting"))
            qDebug()<< tr("删除配置文件");
        delete m_File;
        m_File = NULL;
        foreach (Download *thread, threads) {
            thread->deleteLater();
        }
        threads.clear();
        blockstates.clear();
        freeblocks.clear();
        emit FinishedSignal();
        qDebug() << "Download finished";
        return;
    }

    int n = threads[index]->m_Index;
    blockstates[n] = Download::Finished;
    freeblocks[2*n] = freeblocks[2*n+1] = 0;

    if(m_MaxSpeed > 0)                                              //若限速，设置1S延时
    {
        QEventLoop loop;
        QTimer::singleShot(1000,&loop,SLOT(quit()));
        loop.exec();
        if(m_State != Downloading || m_MaxSpeed <= 0)               //状态为暂停 或 速度切换为不限速 时 忽视往后续
            return;
    }
    for(int i = m_ThreadCount; i<freeblocks.size()/2 ; i++)         //继续将线程派往后续文件段
    {
        if(blockstates[i] == Download::Stoped)
        {
            blockstates[i] = Download::Downloading;
            threads[index]->Start(i,m_Url,m_File,freeblocks[2*i],freeblocks[2*i+1]);
            return;
        }
    }
    double p = 1.0 * m_ReadySize / m_FileSize;
    if(p < 0.98)                                                    //有线程空闲并且下载量未到98%，重新划分文件块
    {
        qDebug() << tr("重新划分文件块");
        Stop();
        Start();
    }
}

void HTTPTask::ProgressSlot(qint64 readySize)
{
    m_ReadySize += readySize;
}


/*停止下载*/
void HTTPTask::Stop()
{
    if(m_State != Stoped)
    {
        _ClearThreads();                                                 //清理下载线程
        m_File->flush();                                                 //刷入文件
        _RebuildDownloadBlocks();                                        //重整下载线程
        _FixLogFile();                                                   //修正记录文件

        m_File->close();
        qDebug() << tr("关闭文件");
        delete m_File;
        m_File = NULL;
        m_State = Stoped;
    }
}

