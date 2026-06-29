#ifndef SCREENRECORDER_H
#define SCREENRECORDER_H

#include <QObject>
#include <QRect>
#include <QString>
#include <QProcess>
#include <QTimer>

class VideoEncoder;

class ScreenRecorder : public QObject
{
    Q_OBJECT

public:
    explicit ScreenRecorder(QObject *parent = nullptr);
    ~ScreenRecorder();

    bool start();
    void stop();
    void pause();
    void resume();
    
    bool isRecording() const { return m_isRecording; }
    bool isPaused() const { return m_isPaused; }
    
    void setOutputFile(const QString &filename);
    void setRecordingRegion(const QRect &region);
    void setVideoCodec(const QString &codec);
    void setAudioCodec(const QString &codec);
    void setQuality(int quality);
    void setFrameRate(int fps);
    void setRecordAudio(bool record);
    void setAudioDevice(const QString &device);
    
    QString getOutputFile() const { return m_outputFile; }
    
    // Get available audio devices
    QStringList getAvailableAudioDevices();

signals:
    void recordingStarted();
    void recordingStopped();
    void recordingPaused();
    void recordingResumed();
    void errorOccurred(const QString &error);
    void frameRecorded(int frameNumber);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onProcessOutput();

private:
    bool startFFmpegProcess();
    QStringList buildFFmpegArguments();
    QString getFFmpegPath();
    QString detectAudioDevice();
    
    bool m_isRecording;
    bool m_isPaused;
    
    QString m_outputFile;
    QRect m_recordingRegion;
    QString m_videoCodec;
    QString m_audioCodec;
    int m_quality;
    int m_frameRate;
    bool m_recordAudio;
    QString m_audioDevice;
    
    QProcess *m_ffmpegProcess;
    VideoEncoder *m_encoder;
    
    qint64 m_pauseStartTime;
    qint64 m_totalPausedTime;
};

#endif // SCREENRECORDER_H
