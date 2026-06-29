#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include <QObject>
#include <QImage>
#include <QRect>
#include <QString>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

class VideoEncoder : public QObject
{
    Q_OBJECT

public:
    explicit VideoEncoder(QObject *parent = nullptr);
    ~VideoEncoder();

    bool initialize(const QString &filename, int width, int height, int fps, const QString &codec);
    bool encodeFrame(const QImage &frame);
    bool finalize();
    
    bool isInitialized() const { return m_initialized; }
    QString getLastError() const { return m_lastError; }

signals:
    void encodingError(const QString &error);
    void frameEncoded(int frameNumber);

private:
    bool openCodec();
    bool openOutputFile();
    void cleanup();
    
    bool m_initialized;
    QString m_lastError;
    
    QString m_filename;
    int m_width;
    int m_height;
    int m_fps;
    QString m_codecName;
    
    AVFormatContext *m_formatContext;
    AVCodecContext *m_codecContext;
    AVStream *m_videoStream;
    AVFrame *m_frame;
    AVPacket *m_packet;
    SwsContext *m_swsContext;
    
    int m_frameCount;
    int64_t m_nextPts;
};

#endif // VIDEOENCODER_H
