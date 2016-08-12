#include "taskmanager.h"

TaskManager::TaskManager(QObject *parent) : QObject(parent)
{
    m_ThreadCount = 5;
    _FindMarkFile();
    qDebug() << tr("任务管理器初始化成功");
}


/*创建任务*/
void TaskManager::CreateTasks(QVector<QVector<QString> > tasks)
{
    for(int i = 0;i < tasks.size();i++)
    {
        TaskBase* task = NULL;
        //url,path,threadscount,user,passwd
        if(tasks[i][0][0] == 'f')
            task = new FTPTask(TaskBase::FTP,tasks[i][0],tasks[i][1],tasks[i][3],tasks[i][4],tasks[i][2].toInt(),this);
        else
            task = new HTTPTask(TaskBase::HTTP,tasks[i][0],tasks[i][1],tasks[i][2].toInt(),this);
        connect(task,SIGNAL(FinishedSignal()),this,SLOT(FinishedSlot()));
        task->Start();
        download.append(task);

        Message msg = {task->GetFileSize(),task->GetFilename(),_TSize(task->GetFileSize()),
                       "",TaskBase::Stoped,QString::number(task->GetReadySize()),"0"};
        livingmsg.append(msg);
        qDebug() << tr("创建任务：")<< i+1;
    }
}

/*启动任务*/
void TaskManager::StartTasks()
{
    foreach(TaskBase* task , download)
    {
        task->Start();
    }
    qDebug() << tr("开始所有任务");
}

void TaskManager::StartTasks(int num, TaskManager::ROOM room)
{
    switch (room) {
    case LIVING:
        if(num < download.size())
            download[num]->Start();
        qDebug() << tr("第%1个任务已开始").arg(num);
        break;
    case FINISHED:
        if(num < finished.size())
        {
            TaskBase* task;
            if(finished[num]->m_Type == TaskBase::HTTP)
                task = new HTTPTask(TaskBase::HTTP,finished[num]->m_Url,finished[num]->m_Path,m_ThreadCount,this);
            else
                task = new FTPTask(TaskBase::FTP,finished[num]->m_Url,finished[num]->m_Path,
                                   QUrl(finished[num]->m_Url).userName(),QUrl(finished[num]->m_Url).password()
                                   ,m_ThreadCount,this);
            connect(task,SIGNAL(FinishedSignal()),this,SLOT(FinishedSlot()));
            finished.remove(num);
            Message msg = {task->GetFileSize(),task->GetFilename(),_TSize(task->GetFileSize()),
                           "",TaskBase::Stoped,QString::number(task->GetReadySize()),"0"};
            livingmsg.append(msg);
            task->Start();
            download.append(task);
            qDebug() << tr("重新加入下载队列");
        }
    case DELETED:
        if(num < deleted.size())
        {
            TaskBase* task;
            if(deleted[num]->m_Type == TaskBase::HTTP)
                task = new HTTPTask(TaskBase::HTTP,deleted[num]->m_Url,deleted[num]->m_Path,m_ThreadCount,this);
            else
                task = new FTPTask(TaskBase::FTP,deleted[num]->m_Url,deleted[num]->m_Path,
                                   QUrl(deleted[num]->m_Url).userName(),QUrl(deleted[num]->m_Url).password()
                                   ,m_ThreadCount,this);
            connect(task,SIGNAL(FinishedSignal()),this,SLOT(FinishedSlot()));
            deleted.remove(num);
            Message msg = {task->GetFileSize(),task->GetFilename(),_TSize(task->GetFileSize()),
                           "",TaskBase::Stoped,QString::number(task->GetReadySize()),"0"};
            livingmsg.append(msg);
            task->Start();
            download.append(task);
            qDebug() << tr("重新加入下载队列");
        }
    default:
        break;
    }
}

/*暂停任务*/
void TaskManager::StopTasks()
{
    foreach(TaskBase* task , download)
    {
        task->Stop();
    }
    qDebug() << tr("已停止所有任务");
}

void TaskManager::StopTasks(int num)
{
    if(num < download.size())
    {
        download[num]->Stop();
        qDebug() << tr("第%1个任务已停止").arg(num);
    }
}

/*删除任务*/
void TaskManager::DeleteTasks(int num, TaskManager::ROOM room)
{
    switch (room) {
    case LIVING:
        if(num < download.size())
        {
            download[num]->Offline();
            TaskMsg* tmsg = new TaskMsg(download[num]->GetType(),download[num]->GetFilename(),
                                        download[num]->GetUrl(),download[num]->GetPath(),
                                        QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                                        download[num]->GetFileSize());
            deleted.append(tmsg);
            QFile::remove(download[num]->GetPath() + ".setting");
            QFile::remove(download[num]->GetPath());
            qDebug()<< tr("删除文件及配置文件");
            download[num]->deleteLater();
            livingmsg.remove(num);
            download.remove(num);

        }
        break;
    case FINISHED:
        if(num < finished.size())
        {
            TaskMsg* task = new TaskMsg(finished[num]->m_Type,finished[num]->m_Filename,
                                        finished[num]->m_Url,finished[num]->m_Path,
                                        finished[num]->m_Time,finished[num]->m_Size);
            finished.remove(num);
            deleted.append(task);
            qDebug()<< tr("任务已删除");
        }
    case DELETED:
        if(num < deleted.size())
        {
            deleted.remove(num);
            qDebug()<< tr("任务已删除");
        }
    default:
        break;
    }
}

/*清除列表*/
void TaskManager::ClearTasks(TaskManager::ROOM room)
{
    switch(room)
    {
    case FINISHED:
        foreach(TaskMsg* tsmg,finished)
        {
            delete tsmg;
        }
        finished.clear();
        qDebug() << tr("已完成列表已清除");
        break;
    case DELETED:
        foreach (TaskMsg* tsmg, deleted)
        {
            delete tsmg;
        }
        deleted.clear();
        qDebug() << tr("回收站列表已清除");
        break;
    default:
        break;
    }
}

/*限速*/
void TaskManager::SetMaxSpeed(qint64 speed)
{
    foreach(TaskBase* task , download)
    {
        task->SetMaxSpeed(speed / download.size());
    }
}

void TaskManager::SetMaxSpeed(int num, qint64 speed)
{
    if(num < download.size())
    {
        download[num]->SetMaxSpeed(speed);
        qDebug() << tr("设置第%1个任务的最大下载速度为:").arg(num) << speed;
    }
}


/*结束下载*/
void TaskManager::Finished()
{
    foreach (TaskBase* task, download) {
        task->Offline();
    }
    _MarkTakstoFile();
    foreach (TaskBase* task, download) {
        task->deleteLater();
    }
    foreach (TaskMsg* task, finished) {
        delete task;
    }
    foreach (TaskMsg* task, deleted) {
        delete task;
    }
    qDebug() << tr("关闭程序");
}

void TaskManager::Response(int num, int tps)
{
    switch (tps) {
    case 0:
        if(download[num]->GetState() == TaskBase::Downloading)
            StopTasks(num);
        else if(download[num]->GetState() == TaskBase::Stoped)
            StartTasks(num,LIVING);
        break;
    case 1:
        StartTasks(num,FINISHED);
        break;
    case 2:
        StartTasks(num,DELETED);
        break;
    default:
        break;
    }
}

QString TaskManager::GetGloableSpeed()
{
    qint64 speed = 0;
    for(int i = 0 ;i < download.size();i++)
    {
        qint64 readysize = download[i]->GetReadySize();
        double dspeed = (readysize - livingmsg[i].readysize.toLongLong()) * 1000.0 / 1000;
        speed += dspeed;
    }
    return _TSpeed(speed);
}

/*给GUI传输数据*/
/*顺序分别为 0：LIVING 1:FINISHED 2:DELETED */
QVector<Message> TaskManager::GetDates(int state)
{
    QVector<Message> res;
    res.clear();
    switch(state)
    {
    case 0:
        for(int i = 0; i < download.size();i++)
        {
            QString filename = download[i]->GetFilename();
            qint64 filesize = download[i]->GetFileSize();
            qint64 readysize = download[i]->GetReadySize();
            TaskBase::State state = download[i]->GetState();
            double dspeed = (readysize - livingmsg[i].readysize.toLongLong()) * 1000.0 / 1000;
            double dlefttime = 1.0 * (filesize - readysize) / dspeed;
            QString speed = _TSpeed(dspeed);
            QString date = _TTimeformat(dlefttime);
            Message msg = {filesize,filename,_TSize(filesize),date,state,QString::number(readysize),speed};
            res.append(msg);
            livingmsg[i].readysize = QString::number(readysize);
            livingmsg[i].state = state;
        }
        break;
    case 1:
        for(int i = 0; i < finished.size(); i++)
        {
            Message msg = {finished[i]->m_Size,finished[i]->m_Filename,_TSize(finished[i]->m_Size),
                           finished[i]->m_Time,TaskBase::Finished,"",""};
            res.append(msg);
        }
        break;
    case 2:
        for(int i = 0; i < deleted.size(); i++)
        {
            Message msg = {deleted[i]->m_Size,deleted[i]->m_Filename,_TSize(deleted[i]->m_Size),
                           deleted[i]->m_Time,TaskBase::Finished,"",""};
            res.append(msg);
        }
        break;
    default: break;
    }
    return res;
}

/*找回下载状态*/
void TaskManager::_FindMarkFile()
{
    download.clear();
    deleted.clear();
    finished.clear();
    livingmsg.clear();

    if(!QFile("downloadtool.setting").exists())
        return;

    QSettings settings("downloadtool.setting",QSettings::IniFormat);

    for(int i = 0;;i++)
    {
        QString index = QString::number(i + 1);
        TaskBase::Type type = (TaskBase::Type)settings.value("TTYPE"+index,TaskBase::HTTP).toInt();
        QUrl url = settings.value("TURL" + index,"").toUrl();
        QString path = settings.value("TPATH" + index, "").toString();

        if(url.toString() == "" || path == "")
        {
            break ;
        }

        TaskBase* task;
        if(type == TaskBase::HTTP)
            task = new HTTPTask(type,url.toString(),path,m_ThreadCount,this);
        else
            task = new FTPTask(type,url.toString(),path,url.userName(),url.password(),m_ThreadCount,this);
        connect(task,SIGNAL(FinishedSignal()),this,SLOT(FinishedSlot()));
        download.append(task);
    }
    for(int i = 0;;i++)
    {
        QString index = QString::number(i + 1);
        TaskBase::Type type = (TaskBase::Type)settings.value("TTYPE"+index,TaskBase::HTTP).toInt();
        QUrl url = settings.value("FURL" + index,"").toUrl();
        QString path = settings.value("FPATH" + index, "").toString();
        QString filename = settings.value("FFILENAME" + index, "").toString();
        qint64 size = settings.value("FSIZE" + index, 0).toLongLong();
        QString time = settings.value("FTIME"+index,"").toString();

        if(url.toString() == "" || path == "" || filename == "" || time == "" || size == 0)
        {
            break ;
        }
        TaskMsg* tmsg = new TaskMsg(type,filename,url.toString(),path,time,size);
        finished.append(tmsg);
    }
    for(int i = 0;;i++)
    {
        QString index = QString::number(i + 1);
        TaskBase::Type type = (TaskBase::Type)settings.value("TTYPE"+index,TaskBase::HTTP).toInt();
        QUrl url = settings.value("DURL" + index,"").toUrl();
        QString path = settings.value("DPATH" + index, "").toString();
        QString filename = settings.value("DFILENAME" + index, "").toString();
        qint64 size = settings.value("DSIZE" + index, 0).toLongLong();
        QString time = settings.value("DTIME"+index,"").toString();

        if(url.toString() == "" || path == "" || filename == "" || time == "" || size == 0)
        {
            break ;
        }
        TaskMsg* tmsg = new TaskMsg(type,filename,url.toString(),path,time,size);
        deleted.append(tmsg);
    }


    for(int i = 0; i < download.size();i++)
    {
        Message msg = {download[i]->GetFileSize(),download[i]->GetFilename(),_TSize(download[i]->GetFileSize()),
                       "",TaskBase::Stoped,QString::number(download[i]->GetReadySize()),"0"};
        livingmsg.append(msg);
    }
    qDebug() << tr("引用配置文件");
}

/*写入文件*/
void TaskManager::_MarkTakstoFile()
{
    QSettings settings("downloadtool.setting",QSettings::IniFormat);
    settings.clear();

    settings.setValue("Data",QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    for(int i = 0;i < download.size() ;i++)
    {
        QString index = QString::number(i + 1);
        settings.setValue("TTYPE"+index,download[i]->GetType());
        settings.setValue("TURL" + index,QUrl(download[i]->GetUrl()));
        settings.setValue("TPATH" + index, download[i]->GetPath());
    }
    for(int i = 0;i < finished.size() ;i++)
    {
        QString index = QString::number(i + 1);
        settings.setValue("FTYPE"+index, finished[i]->m_Type);
        settings.setValue("FURL" + index,QUrl(finished[i]->m_Url));
        settings.setValue("FPATH" + index, finished[i]->m_Path);
        settings.setValue("FFILENAME" + index, finished[i]->m_Filename);
        settings.setValue("FSIZE" + index,finished[i]->m_Size);
        settings.setValue("FTIME"+index,finished[i]->m_Time);
    }
    for(int i = 0;i < deleted.size() ;i++)
    {
        QString index = QString::number(i + 1);
        settings.setValue("DTYPE"+index,deleted[i]->m_Type);
        settings.setValue("DURL" + index,QUrl(deleted[i]->m_Url));
        settings.setValue("DPATH" + index, deleted[i]->m_Path);
        settings.setValue("DFILENAME" + index, deleted[i]->m_Filename);
        settings.setValue("DSIZE" + index,deleted[i]->m_Size);
        settings.setValue("DTIME"+index,deleted[i]->m_Time);
    }
    qDebug() << tr("更新配置文件");
}

/*转换文件大小*/
QString TaskManager::_TSize(qint64 bytes)
{
    QString strUnit;
    double dSize = bytes * 1.0;
    if (dSize <= 0)
    {
        dSize = 0.0;
    }
    else if (dSize < 1024)
    {
        strUnit = "Bytes";
    }
    else if (dSize < 1024 * 1024)
    {
        dSize /= 1024;
        strUnit = "KB";
    }
    else if (dSize < 1024 * 1024 * 1024)
    {
        dSize /= (1024 * 1024);
        strUnit = "MB";
    }
    else
    {
        dSize /= (1024 * 1024 * 1024);
        strUnit = "GB";
    }

    return QString("%1 %2").arg(QString::number(dSize, 'f', 2)).arg(strUnit);
}

/*转换下载速度*/
QString TaskManager::_TSpeed(double speed)
{
    QString strUnit;
    if (speed <= 0)
    {
        speed = 0;
        strUnit = "Bytes/S";
    }
    else if (speed < 1024)
    {
        strUnit = "Bytes/S";
    }
    else if (speed < 1024 * 1024)
    {
        speed /= 1024;
        strUnit = "KB/S";
    }
    else if (speed < 1024 * 1024 * 1024)
    {
        speed /= (1024 * 1024);
        strUnit = "MB/S";
    }
    else
    {
        speed /= (1024 * 1024 * 1024);
        strUnit = "GB/S";
    }

    QString strSpeed = QString::number(speed, 'f', 2);
    return QString("%1 %2").arg(strSpeed).arg(strUnit);
}

/*转换时间*/
QString TaskManager::_TTimeformat(int seconds)
{
    QString strValue;
    QString strSpacing(" ");
    if (seconds <= 0)
    {
        strValue = QString("%1s").arg(0);
    }
    else if (seconds < 60)
    {
        strValue = QString("%1s").arg(seconds);
    }
    else if (seconds < 60 * 60)
    {
        int nMinute = seconds / 60;
        int nSecond = seconds - nMinute * 60;

        strValue = QString("%1m").arg(nMinute);

        if (nSecond > 0)
            strValue += strSpacing + QString("%1s").arg(nSecond);
    }
    else if (seconds < 60 * 60 * 24)
    {
        int nHour = seconds / (60 * 60);
        int nMinute = (seconds - nHour * 60 * 60) / 60;
        int nSecond = seconds - nHour * 60 * 60 - nMinute * 60;

        strValue = QString("%1h").arg(nHour);

        if (nMinute > 0)
            strValue += strSpacing + QString("%1m").arg(nMinute);

        if (nSecond > 0)
            strValue += strSpacing + QString("%1s").arg(nSecond);
    }
    else
    {
        int nDay = seconds / (60 * 60 * 24);
        int nHour = (seconds - nDay * 60 * 60 * 24) / (60 * 60);
        int nMinute = (seconds - nDay * 60 * 60 * 24 - nHour * 60 * 60) / 60;
        int nSecond = seconds - nDay * 60 * 60 * 24 - nHour * 60 * 60 - nMinute * 60;

        strValue = QString("%1d").arg(nDay);

        if (nHour > 0)
            strValue += strSpacing + QString("%1h").arg(nHour);

        if (nMinute > 0)
            strValue += strSpacing + QString("%1m").arg(nMinute);

        if (nSecond > 0)
            strValue += strSpacing + QString("%1s").arg(nSecond);
    }

    return strValue;
}

/*完成任务*/
void TaskManager::FinishedSlot()
{
    qDebug() << tr("一个任务完成");
    for(int i = 0; i < download.size(); i++)
    {
        if(download[i]->GetFileSize() <= download[i]->GetReadySize())
        {
            TaskMsg *msg = new TaskMsg(download[i]->GetType(),download[i]->GetFilename(),download[i]->GetUrl(),
                                       download[i]->GetPath(),QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),
                                       download[i]->GetFileSize());
            finished.append(msg);
            download[i]->deleteLater();
            download.remove(i);
            break;
        }
    }
    emit FinishedSignal();
}
