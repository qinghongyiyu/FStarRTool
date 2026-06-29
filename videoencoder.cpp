#include "videoencoder.h"
#include <QDebug>

VideoEncoder::VideoEncoder(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_width(0)
    , m_height(0)
    , m_fps(30)
    , m_formatContext(nullptr)
    , m_codecContext(nullptr)
    , m_videoStream(nullptr)
    , m_frame(nullptr)
    , m_packet(nullptr)
    , m_swsContext(nullptr)
    , m_frameCount(0)
    , m_nextPts(0)
{
}

VideoEncoder::~VideoEncoder()
{
    cleanup();
}

bool VideoEncoder::initialize(const QString &filename, int width, int height, int fps, const QString &codec)
{
    if (m_initialized) {
        cleanup();
    }
    
    m_filename = filename;
    m_width = width;
    m_height = height;
    m_fps = fps;
    m_codecName = codec;
    m_frameCount = 0;
    m_nextPts = 0;
    
    // Allocate format context
    avformat_alloc_output_context2(&m_formatContext, nullptr, nullptr, filename.toUtf8().constData());
    if (!m_formatContext) {
        m_lastError = "无法创建输出格式上下文";
        emit encodingError(m_lastError);
        return false;
    }
    
    // Open codec
    if (!openCodec()) {
        cleanup();
        return false;
    }
    
    // Open output file
    if (!openOutputFile()) {
        cleanup();
        return false;
    }
    
    // Allocate frame
    m_frame = av_frame_alloc();
    if (!m_frame) {
        m_lastError = "无法分配视频帧";
        emit encodingError(m_lastError);
        cleanup();
        return false;
    }
    
    m_frame->format = m_codecContext->pix_fmt;
    m_frame->width = m_width;
    m_frame->height = m_height;
    
    int ret = av_frame_get_buffer(m_frame, 0);
    if (ret < 0) {
        m_lastError = "无法分配帧缓冲区";
        emit encodingError(m_lastError);
        cleanup();
        return false;
    }
    
    // Allocate packet
    m_packet = av_packet_alloc();
    if (!m_packet) {
        m_lastError = "无法分配数据包";
        emit encodingError(m_lastError);
        cleanup();
        return false;
    }
    
    // Initialize SWS context for color conversion
    m_swsContext = sws_getContext(
        m_width, m_height, AV_PIX_FMT_BGRA,
        m_width, m_height, m_codecContext->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    
    if (!m_swsContext) {
        m_lastError = "无法初始化图像转换上下文";
        emit encodingError(m_lastError);
        cleanup();
        return false;
    }
    
    // Write header
    ret = avformat_write_header(m_formatContext, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        m_lastError = QString("无法写入文件头: %1").arg(errbuf);
        emit encodingError(m_lastError);
        cleanup();
        return false;
    }
    
    m_initialized = true;
    return true;
}

bool VideoEncoder::encodeFrame(const QImage &frame)
{
    if (!m_initialized) {
        m_lastError = "编码器未初始化";
        return false;
    }
    
    // Convert QImage to AVFrame
    QImage convertedImage = frame.convertToFormat(QImage::Format_RGBA8888);
    
    const uint8_t *srcData[1] = { convertedImage.bits() };
    int srcLinesize[1] = { static_cast<int>(convertedImage.bytesPerLine()) };
    
    // Make frame writable
    int ret = av_frame_make_writable(m_frame);
    if (ret < 0) {
        m_lastError = "无法使帧可写";
        return false;
    }
    
    // Convert image format
    sws_scale(m_swsContext, srcData, srcLinesize, 0, m_height,
              m_frame->data, m_frame->linesize);
    
    m_frame->pts = m_nextPts++;
    
    // Send frame to encoder
    ret = avcodec_send_frame(m_codecContext, m_frame);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        m_lastError = QString("发送帧到编码器失败: %1").arg(errbuf);
        return false;
    }
    
    // Receive encoded packets
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_codecContext, m_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            m_lastError = QString("接收编码包失败: %1").arg(errbuf);
            return false;
        }
        
        // Rescale packet timestamps
        av_packet_rescale_ts(m_packet, m_codecContext->time_base, m_videoStream->time_base);
        m_packet->stream_index = m_videoStream->index;
        
        // Write packet to file
        ret = av_interleaved_write_frame(m_formatContext, m_packet);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            m_lastError = QString("写入帧失败: %1").arg(errbuf);
            av_packet_unref(m_packet);
            return false;
        }
        
        av_packet_unref(m_packet);
    }
    
    m_frameCount++;
    emit frameEncoded(m_frameCount);
    
    return true;
}

bool VideoEncoder::finalize()
{
    if (!m_initialized) {
        return false;
    }
    
    // Flush encoder
    int ret = avcodec_send_frame(m_codecContext, nullptr);
    if (ret < 0) {
        qDebug() << "Error flushing encoder";
    }
    
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_codecContext, m_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            break;
        }
        
        av_packet_rescale_ts(m_packet, m_codecContext->time_base, m_videoStream->time_base);
        m_packet->stream_index = m_videoStream->index;
        
        av_interleaved_write_frame(m_formatContext, m_packet);
        av_packet_unref(m_packet);
    }
    
    // Write trailer
    av_write_trailer(m_formatContext);
    
    cleanup();
    return true;
}

bool VideoEncoder::openCodec()
{
    // Find encoder
    const AVCodec *codec = avcodec_find_encoder_by_name(m_codecName.toUtf8().constData());
    if (!codec) {
        m_lastError = QString("找不到编码器: %1").arg(m_codecName);
        emit encodingError(m_lastError);
        return false;
    }
    
    // Create video stream
    m_videoStream = avformat_new_stream(m_formatContext, nullptr);
    if (!m_videoStream) {
        m_lastError = "无法创建视频流";
        emit encodingError(m_lastError);
        return false;
    }
    
    m_videoStream->id = m_formatContext->nb_streams - 1;
    
    // Allocate codec context
    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        m_lastError = "无法分配编码器上下文";
        emit encodingError(m_lastError);
        return false;
    }
    
    // Set codec parameters
    m_codecContext->codec_id = codec->id;
    m_codecContext->bit_rate = 4000000;
    m_codecContext->width = m_width;
    m_codecContext->height = m_height;
    m_codecContext->time_base = AVRational{1, m_fps};
    m_codecContext->framerate = AVRational{m_fps, 1};
    m_codecContext->gop_size = 10;
    m_codecContext->max_b_frames = 1;
    m_codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    
    if (m_formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        m_codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    // Open codec
    int ret = avcodec_open2(m_codecContext, codec, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        m_lastError = QString("无法打开编码器: %1").arg(errbuf);
        emit encodingError(m_lastError);
        return false;
    }
    
    // Copy codec parameters to stream
    ret = avcodec_parameters_from_context(m_videoStream->codecpar, m_codecContext);
    if (ret < 0) {
        m_lastError = "无法复制编码器参数到流";
        emit encodingError(m_lastError);
        return false;
    }
    
    m_videoStream->time_base = m_codecContext->time_base;
    
    return true;
}

bool VideoEncoder::openOutputFile()
{
    if (!(m_formatContext->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&m_formatContext->pb, m_filename.toUtf8().constData(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            m_lastError = QString("无法打开输出文件: %1").arg(errbuf);
            emit encodingError(m_lastError);
            return false;
        }
    }
    
    return true;
}

void VideoEncoder::cleanup()
{
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    
    if (m_packet) {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }
    
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }
    
    if (m_formatContext) {
        if (m_formatContext->pb && !(m_formatContext->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&m_formatContext->pb);
        }
        avformat_free_context(m_formatContext);
        m_formatContext = nullptr;
    }
    
    m_initialized = false;
}
