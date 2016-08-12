#include "download.h"

Download::Download(int id, QObject *parent)
: QObject(parent),m_ID(id)
{
    m_State = Stoped;
    m_HaveDoneBytes = 0;
    m_StartPoint = 0;
    m_EndPoint = 0;
    m_File = NULL;
}

void Download::Start(int index,
                             QUrl &url,
                             QFile *file,
                             qint64 startPoint,
                             qint64 endPoint)
{
    if( NULL == file || endPoint <= startPoint)
        return;

    m_Index = index;
    m_HaveDoneBytes = 0;
    m_StartPoint = startPoint;
    m_EndPoint = endPoint;
    m_File = file;

    //根据HTTP协议，写入RANGE头部，说明请求文件的范围
    QNetworkRequest qheader;
    qheader.setUrl(url);
    QString range;
    range.sprintf("Bytes=%lld-%lld", m_StartPoint, m_EndPoint);
    qheader.setRawHeader("Range", range.toLatin1());

    //开始下载
    qDebug() << "Part " << m_Index << " start download";
    m_State = Downloading;
    m_Reply = m_Qnam.get(qheader);
    connect(m_Reply, SIGNAL(finished()),
        this, SLOT(FinishedSlot()));
    connect(m_Reply, SIGNAL(readyRead()),
        this, SLOT(HttpReadyReadSlot()));
    emit ProgressSignal(0);
}

/*下载结束*/
void Download::FinishedSlot()
{
    if(m_StartPoint+m_HaveDoneBytes < m_EndPoint)
        return;

    if(m_State != Downloading)      //避免Stoped时发出finish信号影响下载
        return;
    _DownloadFinished();
    emit FinishedSignal(m_ID);
}


void Download::_DownloadFinished()
{
    if(m_State != Downloading)
        return;
    m_Reply->deleteLater();
    m_Reply = 0;
    m_File = 0;
    m_State = Finished;
    qDebug() << "Part " << m_Index << " download finished" << m_HaveDoneBytes<<"Bytes";
}

void Download::HttpReadyReadSlot()
{
    if ( !m_File || m_State != Downloading)
        return;

    QByteArray buffer = m_Reply->readAll();
    m_File->seek( m_StartPoint + m_HaveDoneBytes );
    m_File->write(buffer);
    m_HaveDoneBytes += buffer.size();
    emit ProgressSignal(buffer.size());
}

/*停止*/
void Download::Stop()
{
    if(m_State != Downloading)
    {
        return ;
    }
    m_State = Stoped;
    m_Reply->abort();
    m_Reply->deleteLater();
}

