#include "ftptask.h"
#include "Python.h"


void *workThread(void *pData)
{
    tNode *pNode = (tNode*)pData;
    int res = curl_easy_perform(pNode->curl);

    curl_easy_cleanup(pNode->curl);

    //    pthread_mutex_lock(&g_mutex);
    pNode->parent->m_Mutex.lock();

    qDebug() << "thread "<<pNode->tid<<" exit";

    --pNode->parent->threadCnt;
    qDebug() << pNode->parent->GetFilename() << "         workthread"<<pNode->parent->threadCnt;
    if(pNode->readysize + pNode->startpoint >= pNode->endpoint)
        emit pNode->parent->_FinishedSignal(pNode->id);
    //    pthread_mutex_unlock(&g_mutex);
    pNode->parent->m_Mutex.unlock();

    pthread_exit(0);
    return NULL;
}

FTPTask::FTPTask(Type type, QString url, QString path, QString username, QString password, int count, QObject *parent)
    : m_Type(type),m_Path(path),m_ThreadCount(count),TaskBase(parent)
{
    m_State = Stoped;
    m_File = NULL;
    m_ReadySize = 0;
    m_MaxSpeed = -1;
    m_Url.setUrl(url);
    m_Url.setUserName(username);
    m_Url.setPassword(password);
    m_Url.setPort(21);
    if(!_FindLogFile())
        m_FileSize = _GetFileSizeFromUrl();
    threadCnt = 0;
    connect(this,SIGNAL(_FinishedSignal(qint64)),this,SLOT(FinishedSlot(qint64)));
}

FTPTask::~FTPTask()
{
    if(m_File)
        delete m_File;
    m_Mutex.lock();
    //    pthread_mutex_lock(&g_mutex);
    foreach(tNode* thread,threads)
        delete thread;
    //    pthread_mutex_unlock(&g_mutex);
    threads.clear();
    m_Mutex.unlock();
}


void FTPTask::Start()
{
    if(m_State != Downloading)
    {
        threadCnt = 0;
        if(_FindLogFile())              //读取文件成功
        {
            _Start();
            return;
        }
        m_State = Downloading;

        m_File = new QFile(m_Path,this);
        m_File->open(QIODevice::WriteOnly | QIODevice::Append);
        qDebug()<<tr("打开文件");
        m_File->resize(m_FileSize);

        threads.clear();
        freeblocks.clear();
        blockstates.clear();

        qint64 pear = m_FileSize / m_ThreadCount;

        for(int i = 0;i < m_ThreadCount;i++)
        {
            qint64 startPoint = pear * i;
            qint64 endPoint = (i == m_ThreadCount-1 ? m_FileSize : (pear * (i + 1)));
            freeblocks.append(startPoint);
            freeblocks.append(endPoint);
            blockstates.append(Downloading);

            tNode *pNode = new tNode();

            pNode->startpoint = startPoint;
            pNode->endpoint = endPoint;
            pNode->readysize = 0;
            pNode->parent = this;
            pNode->id = i;
            pNode->index = i;

            CURL* curl = curl_easy_init();

            pNode->curl = curl;
            pNode->file = m_File;


            curl_easy_setopt(curl,CURLOPT_URL,m_Url.toString().toStdString().c_str());
            curl_easy_setopt(curl,CURLOPT_PORT,21);
            curl_easy_setopt(curl,CURLOPT_WRITEDATA, (void*)pNode);
            curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION, Filewrite);

            struct curl_slist *paction = NULL;
            char cmd[256] = {0};
            sprintf(cmd,"REST %lld", pNode->startpoint);
            qDebug() << cmd << "            "<<pNode->startpoint<<" "<<pNode->endpoint;
            paction = curl_slist_append(paction,cmd);
            curl_easy_setopt(pNode->curl,CURLOPT_PREQUOTE,paction);

            m_Mutex.lock();
            //            pthread_mutex_lock(&g_mutex_thread);
            ++threadCnt;


            qDebug() << GetFilename() << "         start"<<threadCnt;

            threads.append(pNode);
            //            pthread_mutex_unlock(&g_mutex_thread);
            m_Mutex.unlock();
            pthread_create(&pNode->tid,NULL,workThread,pNode);
        }
    }
}

void FTPTask::Offline()
{
    if(m_MaxSpeed > 0)
        m_MaxSpeed = -1;
    Stop();
}

void FTPTask::Stop()
{
    if(m_State != Stoped)
    {
        m_State = Stoped;

        _ClearThreads();                                                 //清理线程
        _RebuildDownloadBlocks();                                        //重整下载线程
        _FixLogFile();                                                   //修正记录文件

        m_File->flush();                                                 //刷入文件
        m_File->close();
        delete m_File;
        m_File = NULL;
        qDebug() << tr("关闭文件");
    }
}

void FTPTask::SetMaxSpeed(qint64 _kbps)
{
    qint64 kbps = (_kbps <= 0 ? -1 : (_kbps * 1024));

    if(kbps == m_MaxSpeed)
        return;

    if(m_State != Downloading)                          //暂停后修改了速度
    {
        m_MaxSpeed = kbps;
        _RebuildDownloadBlocks();
        _FixLogFile();
        return;
    }

    m_State = Stoped;

    _ClearThreads();
    m_MaxSpeed = kbps;                                            //重置进程数

    _RebuildDownloadBlocks();                                         //重整下载块

    m_State = Downloading;
    threads.clear();
    for(int i = 0;i < m_ThreadCount && i < freeblocks.size()/2; i++)
    {
        qint64 startPoint = freeblocks[2*i];
        qint64 endPoint = freeblocks[2*i+1];
        blockstates[i] = Downloading;                     //设置分块为正在下载

        tNode *pNode = new tNode();

        pNode->startpoint = startPoint;
        pNode->endpoint = endPoint;
        pNode->readysize = 0;
        pNode->parent = this;
        pNode->id = i;
        pNode->index = i;

        CURL* curl = curl_easy_init();

        pNode->curl = curl;
        pNode->file = m_File;


        curl_easy_setopt(curl,CURLOPT_URL,m_Url.toString().toStdString().c_str());
        curl_easy_setopt(curl,CURLOPT_PORT,21);
        curl_easy_setopt(curl,CURLOPT_WRITEDATA, (void*)pNode);
        curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION, Filewrite);

        struct curl_slist *paction = NULL;
        char cmd[256] = {0};
        sprintf(cmd,"REST %lld", pNode->startpoint);
        qDebug() << cmd << "            "<<pNode->startpoint<<" "<<pNode->endpoint;
        paction = curl_slist_append(paction,cmd);
        curl_easy_setopt(pNode->curl,CURLOPT_PREQUOTE,paction);

        m_Mutex.lock();
        //        pthread_mutex_lock(&g_mutex_thread);
        ++threadCnt;


        qDebug() << GetFilename() << "      setmaxspeed"<<threadCnt;

        threads.append(pNode);
        //        pthread_mutex_unlock(&g_mutex_thread);
        m_Mutex.unlock();

        pthread_create(&pNode->tid,NULL,workThread,pNode);
    }
}


qint64 FTPTask::_GetFileSizeFromUrl()
{
    qint64 size = 0;
    CURLcode res;
    double gsize = 0.0;
    CURL* curl;
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, m_Url.toString().toStdString().c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

    res = curl_easy_perform(curl);
    res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &gsize);
    if(res == CURLE_OK && gsize != -1){
        qDebug() << "文件大小获取成功："<<(qint64)gsize;
        curl_easy_cleanup(curl);
        size = gsize;
        return size;
    }
    curl_easy_cleanup(curl);
    qDebug() << tr("libcurl获取大小失败，使用python脚本获取");

    /*采用Python脚本获取文件大小*/
    std::string py = "getsize";
    std::string fuc = "ftpsize";

    Py_Initialize();

    PyObject* pyMod = PyImport_ImportModule(py.c_str());
    PyObject* pyValue = PyObject_CallMethod(pyMod,fuc.c_str(),"ssss",
                                            m_Url.host().toStdString().c_str(),
                                            m_Url.path().toStdString().c_str(),
                                            m_Url.userName().toStdString().c_str(),
                                            m_Url.password().toStdString().c_str());
    qDebug() << m_Url.host();
    qDebug() << m_Url.path();

    size = PyLong_AsLongLong(pyValue);
    Py_Finalize();
    qDebug() << size;
    return size;
}

void FTPTask::_FixLogFile()
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

bool FTPTask::_FindLogFile()
{
    if(QFile(m_Path).exists() && QFile(m_Path+".setting").exists())
    {
        QSettings settings(m_Path+".setting",QSettings::IniFormat);
        m_Url = settings.value("URL").toUrl();
        m_Path = settings.value("FILEPATH").toString();
        m_ThreadCount = settings.value("DOWNLOADCOUNT",-1).toInt();
        m_FileSize = settings.value("FILESIZE",-1).toLongLong();
        m_ReadySize = settings.value("READYSIZE",0).toLongLong();

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
            blockstates.append(Stoped);
        }
        return true;
    }
    return false;
}

void FTPTask::_ClearThreads()
{
    while(threadCnt > 0)
        usleep(1000000L);

    //    pthread_mutex_lock(&g_mutex_write);
    m_Mutex.lock();

    for(int i = 0;i < threads.size();i++)
    {
        qint64 first = threads[i]->startpoint + threads[i]->readysize;
        qint64 second = threads[i]->endpoint;
        if(first >= second)
        {
            freeblocks[2 * threads[i]->index] = 0;
            freeblocks[2 * threads[i]->index + 1] = 0;
        }
        else
        {
            freeblocks[2 * threads[i]->index] = first;
            freeblocks[2 * threads[i]->index + 1] = second;
        }
        blockstates[threads[i]->index] = Stoped;        //分块状态改为暂停
    }


    foreach(tNode* thread,threads)
        delete thread;
    threads.clear();

    m_Mutex.unlock();
}


void FTPTask::_RebuildDownloadBlocks()
{
    qint64 pear = 0;
    if(m_MaxSpeed > 0)                                            //若有限速
        pear = m_MaxSpeed / m_ThreadCount;
    else                                                            //若无限速
    {
        qint64 size = 0;
        int i = 0;
        for(i = 0;i < freeblocks.size() / 2;i++)
        {
            size += (freeblocks[2*i+1]-freeblocks[2*i]);
        }
        pear = size / m_ThreadCount;
    }
    if(pear == 0)
        return;
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
        blockstates.append(Stoped);
    }
    tmps.clear();
}

void FTPTask::_Start()
{
    if(Downloading != m_State)
    {
        m_State = Downloading;
        m_File = new QFile(m_Path,this);
        m_File->open(QIODevice::WriteOnly | QIODevice::Append);
        qDebug()<<tr("打开文件");
        m_File->resize(m_FileSize);

        threads.clear();
        for(int i = 0;i < m_ThreadCount && i < freeblocks.size()/2; i++)
        {
            qint64 startPoint = freeblocks[2*i];
            qint64 endPoint = freeblocks[2*i+1];
            blockstates[i] = Downloading;           //设置分块为正在下载

            tNode *pNode = new tNode();

            pNode->startpoint = startPoint;
            pNode->endpoint = endPoint;
            pNode->readysize = 0;
            pNode->parent = this;
            pNode->id = i;
            pNode->index = i;

            CURL* curl = curl_easy_init();

            pNode->curl = curl;
            pNode->file = m_File;


            curl_easy_setopt(curl,CURLOPT_URL,m_Url.toString().toStdString().c_str());
            curl_easy_setopt(curl,CURLOPT_PORT,21);
            curl_easy_setopt(curl,CURLOPT_WRITEDATA, (void*)pNode);
            curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION, Filewrite);

            struct curl_slist *paction = NULL;
            char cmd[256] = {0};
            sprintf(cmd,"REST %lld", pNode->startpoint);
            qDebug() << cmd << "            "<<pNode->startpoint<<" "<<pNode->endpoint;
            paction = curl_slist_append(paction,cmd);
            curl_easy_setopt(pNode->curl,CURLOPT_PREQUOTE,paction);

            m_Mutex.lock();

            ++threadCnt;
            qDebug() << GetFilename() << "        _start "<<threadCnt;
            threads.append(pNode);

            m_Mutex.unlock();
            pthread_create(&pNode->tid,NULL,workThread,pNode);
        }
    }
}

void FTPTask::_DownloadNext(qint64 id)
{
    for(int i = m_ThreadCount; i < freeblocks.size()/2 ; i++)       //继续将线程派往后续文件段
    {
        if(blockstates[i] == Stoped)
        {
            blockstates[i] = Downloading;

            threads[id]->index = i;
            threads[id]->startpoint = freeblocks[2*i];
            threads[id]->endpoint = freeblocks[2*i+1];
            threads[id]->readysize = 0;
            threads[id]->parent = this;
            threads[id]->file = m_File;

            CURL* curl = curl_easy_init();
            threads[id]->curl = curl;

            curl_easy_setopt(curl,CURLOPT_URL,m_Url.toString().toStdString().c_str());
            curl_easy_setopt(curl,CURLOPT_PORT,21);
            curl_easy_setopt(curl,CURLOPT_WRITEDATA, threads[id]);
            curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION, Filewrite);

            struct curl_slist *paction = NULL;
            char cmd[256] = {0};
            sprintf(cmd,"REST %lld", threads[id]->startpoint);
            qDebug() << cmd;
            paction = curl_slist_append(paction,cmd);
            curl_easy_setopt(threads[id]->curl,CURLOPT_PREQUOTE,paction);

            m_Mutex.lock();
            //            pthread_mutex_lock(&g_mutex);
            ++threadCnt;


            qDebug() << GetFilename() << "        network"<< threadCnt;
            qDebug() << threads.size();

            //            pthread_mutex_unlock(&g_mutex);
            m_Mutex.unlock();

            pthread_create(&threads[id]->tid,NULL,workThread,threads[id]);
            return;
        }
    }
    double p = 1.0 * m_ReadySize / m_FileSize;
    if(threadCnt < m_ThreadCount/2 && p < 0.9 && m_State == Downloading)                 //有线程空闲并且下载量未到98%，重新划分文件块
    {
        qDebug() << tr("重新划分文件块");
        Stop();
        Start();
    }
}

size_t FTPTask::Filewrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
    tNode *node = (tNode *) stream;
    size_t written = 0;

    node->parent->m_Mutex_Write.lock();
    //    pthread_mutex_lock (&g_mutex/*g_mutex_write*/);


    if(node->readysize + node->startpoint >= node->endpoint || node->parent->m_State == Stoped)
    {
        node->parent->m_Mutex_Write.unlock();
        //        pthread_mutex_unlock(&g_mutex/*g_mutex_write*/);
        return -1;
    }

    written = size*nmemb;
    qint64 left = node->endpoint - node->readysize - node->startpoint;
    node->file->seek(node->startpoint + node->readysize);
    node->file->write((char*)buffer,size*nmemb);
    node->readysize += written;
    node->parent->m_ReadySize += (written>left?left:written);

    node->parent->m_Mutex_Write.unlock();
    //    pthread_mutex_unlock (&g_mutex/*g_mutex_write*/);
    return written;
}


void FTPTask::FinishedSlot(qint64 id)
{
    if(m_ReadySize >= m_FileSize && m_State == Downloading)
    {
        m_State = Finished;
        while(threadCnt > 0)
            usleep(1000000L);
        m_File->flush();
        m_File->close();
        if(QFile::remove(m_File->fileName() + ".setting"))
            qDebug()<< tr("删除配置文件");
        delete m_File;
        m_File = NULL;
        qDebug() << tr("下载完成，关闭文件");
        emit FinishedSignal();
    }

    if(m_State != Downloading)
        return;

    blockstates[threads[id]->index] = Finished;
    freeblocks[2*threads[id]->index] = freeblocks[2*threads[id]->index+1] = 0;

    if(m_MaxSpeed > 0)                              //若限速，设置1S延时
    {
        QTimer::singleShot(1000,this,SLOT(DownloadNextSlot()));
        return;
    }

    _DownloadNext(id);
}

void FTPTask::DownloadNextSlot()
{

    if(m_State != Downloading || m_MaxSpeed <= 0)
        return;

    qint64 id = 0;
    for(int i = 0; i < threads.size();i++)
    {
        if(threads[i]->startpoint + threads[i]->readysize >=threads[i]->endpoint)
        {
            id = i;
            break;
        }
    }

    _DownloadNext(id);
}

