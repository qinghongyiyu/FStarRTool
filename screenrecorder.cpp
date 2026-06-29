#include "screenrecorder.h"
#include "videoencoder.h"
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QDir>
#include <QDateTime>

ScreenRecorder::ScreenRecorder(QObject *parent)
    : QObject(parent)
    , m_isRecording(false)
    , m_isPaused(false)
    , m_quality(23)
    , m_frameRate(30)
    , m_recordAudio(true)
    , m_videoCodec("libx264")
    , m_audioCodec("aac")
    , m_ffmpegProcess(nullptr)
    , m_encoder(nullptr)
    , m_pauseStartTime(0)
    , m_totalPausedTime(0)
{
    m_ffmpegProcess = new QProcess(this);
    m_encoder = new VideoEncoder(this);
    
    connect(m_ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ScreenRecorder::onProcessFinished);
    connect(m_ffmpegProcess, &QProcess::errorOccurred,
            this, &ScreenRecorder::onProcessError);
    connect(m_ffmpegProcess, &QProcess::readyReadStandardOutput,
            this, &ScreenRecorder::onProcessOutput);
    connect(m_ffmpegProcess, &QProcess::readyReadStandardError,
            this, &ScreenRecorder::onProcessOutput);
    
    // Set default recording region to full screen
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        m_recordingRegion = screen->geometry();
    }
}

ScreenRecorder::~ScreenRecorder()
{
    if (m_isRecording) {
        stop();
    }
}

bool ScreenRecorder::start()
{
    if (m_isRecording) {
        return false;
    }
    
    if (m_outputFile.isEmpty()) {
        emit errorOccurred("输出文件未设置");
        return false;
    }
    
    if (m_recordingRegion.isEmpty()) {
        emit errorOccurred("录制区域未设置");
        return false;
    }
    
    // Debug: Show recording settings
    qDebug() << "=== Recording Settings ===";
    qDebug() << "Output file:" << m_outputFile;
    qDebug() << "Video codec:" << m_videoCodec;
    qDebug() << "Audio codec:" << m_audioCodec;
    qDebug() << "Record audio:" << m_recordAudio;
    qDebug() << "Audio device:" << m_audioDevice;
    qDebug() << "Quality:" << m_quality;
    qDebug() << "Frame rate:" << m_frameRate;
    qDebug() << "Recording region:" << m_recordingRegion;
    qDebug() << "========================";
    
    // Start FFmpeg process
    if (!startFFmpegProcess()) {
        return false;
    }
    
    m_isRecording = true;
    m_isPaused = false;
    m_totalPausedTime = 0;
    
    emit recordingStarted();
    return true;
}

void ScreenRecorder::stop()
{
    if (!m_isRecording) {
        return;
    }
    
    m_isRecording = false;
    m_isPaused = false;
    
    // Send 'q' to FFmpeg to stop gracefully
    if (m_ffmpegProcess && m_ffmpegProcess->state() == QProcess::Running) {
        m_ffmpegProcess->write("q\n");
        m_ffmpegProcess->waitForFinished(5000);
        
        if (m_ffmpegProcess->state() == QProcess::Running) {
            m_ffmpegProcess->kill();
        }
    }
    
    emit recordingStopped();
}

void ScreenRecorder::pause()
{
    if (!m_isRecording || m_isPaused) {
        return;
    }
    
    m_isPaused = true;
    m_pauseStartTime = QDateTime::currentMSecsSinceEpoch();
    
    // Note: FFmpeg doesn't support pause/resume directly
    // This is a placeholder for future implementation
    // You would need to stop and restart with proper timestamp handling
    
    emit recordingPaused();
}

void ScreenRecorder::resume()
{
    if (!m_isRecording || !m_isPaused) {
        return;
    }
    
    m_isPaused = false;
    
    if (m_pauseStartTime > 0) {
        qint64 pauseDuration = QDateTime::currentMSecsSinceEpoch() - m_pauseStartTime;
        m_totalPausedTime += pauseDuration;
        m_pauseStartTime = 0;
    }
    
    emit recordingResumed();
}

void ScreenRecorder::setOutputFile(const QString &filename)
{
    m_outputFile = filename;
}

void ScreenRecorder::setRecordingRegion(const QRect &region)
{
    m_recordingRegion = region;
}

void ScreenRecorder::setVideoCodec(const QString &codec)
{
    m_videoCodec = codec;
}

void ScreenRecorder::setAudioCodec(const QString &codec)
{
    m_audioCodec = codec;
}

void ScreenRecorder::setQuality(int quality)
{
    m_quality = quality;
}

void ScreenRecorder::setFrameRate(int fps)
{
    m_frameRate = fps;
}

void ScreenRecorder::setRecordAudio(bool record)
{
    m_recordAudio = record;
}

void ScreenRecorder::setAudioDevice(const QString &device)
{
    m_audioDevice = device;
}

QStringList ScreenRecorder::getAvailableAudioDevices()
{
    QStringList devices;
    QString ffmpegPath = getFFmpegPath();
    
    if (ffmpegPath.isEmpty()) {
        qDebug() << "FFmpeg path is empty, cannot detect audio devices";
        return devices;
    }
    
    QProcess process;
    QStringList args;
    args << "-list_devices" << "true" << "-f" << "dshow" << "-i" << "dummy";
    
    qDebug() << "Running FFmpeg to detect audio devices...";
    
    process.start(ffmpegPath, args);
    process.waitForFinished(5000);
    
    QString output = process.readAllStandardError();
    
    qDebug() << "=== FFmpeg Device Detection Output ===";
    qDebug() << "Output length:" << output.length();
    
    // Parse audio devices from output
    // Format: [dshow @ ...] "Device Name" (audio)
    QStringList lines = output.split('\n');
    
    for (const QString &line : lines) {
        // Look for lines with (audio) marker
        if (line.contains("(audio)", Qt::CaseInsensitive)) {
            // Extract device name from quotes
            int firstQuote = line.indexOf('"');
            int secondQuote = line.indexOf('"', firstQuote + 1);
            
            if (firstQuote >= 0 && secondQuote > firstQuote) {
                QString device = line.mid(firstQuote + 1, secondQuote - firstQuote - 1);
                if (!device.isEmpty()) {
                    devices.append(device);
                    qDebug() << "Found audio device:" << device;
                }
            }
        }
    }
    
    qDebug() << "Total audio devices found:" << devices.count();
    return devices;
}

QString ScreenRecorder::detectAudioDevice()
{
    QStringList devices = getAvailableAudioDevices();
    
    qDebug() << "Available audio devices:" << devices;
    
    // Try to find common audio device names
    // For laptops, prioritize built-in microphone as it's most commonly available
    QStringList preferredNames = {
        // Built-in microphone (most common in laptops)
        "Microphone Array",
        "麦克风阵列",
        "Internal Microphone",
        "内置麦克风",
        "Microphone",
        "麦克风",
        "Array",
        "阵列",
        
        // Realtek devices (common in ThinkPad)
        "Realtek",
        "瑞昱",
        
        // Conexant devices (also common in ThinkPad)
        "Conexant",
        
        // Stereo Mix (system audio) - lower priority as it needs to be enabled
        "立体声混音",
        "Stereo Mix",
        "混音",
        "Wave Out Mix",
        "CABLE Output",
        "What U Hear",
        "您听到的声音"
    };
    
    for (const QString &preferred : preferredNames) {
        for (const QString &device : devices) {
            if (device.contains(preferred, Qt::CaseInsensitive)) {
                qDebug() << "Selected audio device:" << device;
                return device;
            }
        }
    }
    
    // Return first available device if no preferred match
    if (!devices.isEmpty()) {
        qDebug() << "Using first available audio device:" << devices.first();
        return devices.first();
    }
    
    qDebug() << "No audio device found";
    return QString();
}

bool ScreenRecorder::startFFmpegProcess()
{
    QString ffmpegPath = getFFmpegPath();
    if (ffmpegPath.isEmpty()) {
        emit errorOccurred("找不到 FFmpeg 可执行文件");
        return false;
    }
    
    QStringList args = buildFFmpegArguments();
    if (args.isEmpty()) {
        emit errorOccurred("无法构建 FFmpeg 命令");
        return false;
    }
    
    qDebug() << "FFmpeg path:" << ffmpegPath;
    qDebug() << "FFmpeg arguments:" << args.join(" ");
    
    // Start FFmpeg process
    m_ffmpegProcess->start(ffmpegPath, args);
    
    if (!m_ffmpegProcess->waitForStarted(3000)) {
        QString error = m_ffmpegProcess->errorString();
        emit errorOccurred(QString("无法启动 FFmpeg 进程: %1").arg(error));
        return false;
    }
    
    return true;
}

QStringList ScreenRecorder::buildFFmpegArguments()
{
    QStringList args;
    
    // Input format for Windows (GDI grab)
    args << "-f" << "gdigrab";
    args << "-framerate" << QString::number(m_frameRate);
    
    // Recording region
    args << "-offset_x" << QString::number(m_recordingRegion.x());
    args << "-offset_y" << QString::number(m_recordingRegion.y());
    args << "-video_size" << QString("%1x%2")
        .arg(m_recordingRegion.width())
        .arg(m_recordingRegion.height());
    
    // Input source (desktop)
    args << "-i" << "desktop";
    
    // Audio input (if enabled and device is set)
    bool tryAudio = (m_recordAudio && m_audioCodec != "none");
    qDebug() << "Try audio:" << tryAudio << "m_recordAudio:" << m_recordAudio << "m_audioCodec:" << m_audioCodec;
    
    if (tryAudio) {
        QString audioDevice = m_audioDevice;
        
        // If no device specified, try to detect one
        if (audioDevice.isEmpty()) {
            qDebug() << "No audio device specified, attempting to detect...";
            audioDevice = detectAudioDevice();
        }
        
        if (!audioDevice.isEmpty()) {
            qDebug() << "=== AUDIO ENABLED ===";
            qDebug() << "Recording audio from:" << audioDevice;
            qDebug() << "Audio codec:" << m_audioCodec;
            // Use DirectShow to capture audio from specified device
            args << "-f" << "dshow";
            args << "-i" << QString("audio=%1").arg(audioDevice);
        } else {
            qDebug() << "!!! WARNING: No audio device found, recording without audio !!!";
            tryAudio = false;
        }
    } else {
        qDebug() << "Audio recording disabled";
    }
    
    // Video codec settings
    args << "-c:v" << m_videoCodec;
    
    // Quality settings (CRF)
    if (m_videoCodec == "libx264" || m_videoCodec == "libx265") {
        args << "-crf" << QString::number(m_quality);
        args << "-preset" << "ultrafast";
    } else if (m_videoCodec == "libvpx-vp9") {
        args << "-crf" << QString::number(m_quality);
        args << "-b:v" << "0";
    }
    
    // Pixel format
    args << "-pix_fmt" << "yuv420p";
    
    // Audio codec settings
    if (tryAudio) {
        args << "-c:a" << m_audioCodec;
        args << "-b:a" << "192k";
        args << "-ar" << "44100";
        qDebug() << "Audio encoding enabled with codec:" << m_audioCodec;
    } else {
        args << "-an";
        qDebug() << "Audio encoding disabled (-an)";
    }
    
    // Output file
    args << "-y"; // Overwrite output file
    args << m_outputFile;
    
    qDebug() << "=== FFmpeg Command ===";
    qDebug() << "Arguments:" << args.join(" ");
    qDebug() << "=====================";
    
    return args;
}

QString ScreenRecorder::getFFmpegPath()
{
    // Try application directory first (for deployed version)
    QString appDirPath = QApplication::applicationDirPath() + "/ffmpeg.exe";
    QFileInfo appDirInfo(appDirPath);
    if (appDirInfo.exists()) {
        qDebug() << "Found FFmpeg in application directory:" << appDirPath;
        return QDir::toNativeSeparators(appDirInfo.absoluteFilePath());
    }
    
    // Try relative path from build directory (for development)
    QString relativePath = QApplication::applicationDirPath() + 
        "/../../../ffmpeg-8.0.1-full_build-shared/bin/ffmpeg.exe";
    
    QFileInfo relativeInfo(relativePath);
    if (relativeInfo.exists()) {
        qDebug() << "Found FFmpeg in relative path:" << relativeInfo.absoluteFilePath();
        return QDir::toNativeSeparators(relativeInfo.absoluteFilePath());
    }
    
    // Try current directory structure
    QString currentPath = QDir::currentPath() + 
        "/ffmpeg-8.0.1-full_build-shared/bin/ffmpeg.exe";
    
    QFileInfo currentInfo(currentPath);
    if (currentInfo.exists()) {
        qDebug() << "Found FFmpeg in current directory:" << currentInfo.absoluteFilePath();
        return QDir::toNativeSeparators(currentInfo.absoluteFilePath());
    }
    
    // Try project root
    QString projectPath = "E:/2025-TTTT/53--Qt Project Practical Recommendations--/FStarRTool/ffmpeg-8.0.1-full_build-shared/bin/ffmpeg.exe";
    QFileInfo projectInfo(projectPath);
    if (projectInfo.exists()) {
        qDebug() << "Found FFmpeg in project directory:" << projectInfo.absoluteFilePath();
        return QDir::toNativeSeparators(projectInfo.absoluteFilePath());
    }
    
    qDebug() << "FFmpeg not found in any known location";
    qDebug() << "Application dir:" << QApplication::applicationDirPath();
    qDebug() << "Current dir:" << QDir::currentPath();
    
    // Try system PATH as last resort
    return "ffmpeg";
}

void ScreenRecorder::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "FFmpeg process finished with exit code:" << exitCode;
    qDebug() << "Exit status:" << exitStatus;
    
    QString errorOutput = m_ffmpegProcess->readAllStandardError();
    qDebug() << "FFmpeg stderr:" << errorOutput;
    
    if (m_isRecording) {
        m_isRecording = false;
        emit recordingStopped();
    }
    
    if (exitStatus == QProcess::CrashExit) {
        emit errorOccurred("FFmpeg 进程崩溃");
    } else if (exitCode != 0 && exitCode != 255) {
        // Exit code 255 is normal when user stops recording with 'q'
        // Check if it's an audio device error
        if (errorOutput.contains("audio") || errorOutput.contains("dshow")) {
            emit errorOccurred(QString("FFmpeg 错误 (代码 %1):\n音频设备不可用，请检查是否启用了\"立体声混音\"或使用麦克风。\n\n详细信息:\n%2")
                .arg(exitCode)
                .arg(errorOutput.left(500)));
        } else {
            emit errorOccurred(QString("FFmpeg 错误 (代码 %1):\n%2")
                .arg(exitCode)
                .arg(errorOutput.left(500)));
        }
    }
}

void ScreenRecorder::onProcessError(QProcess::ProcessError error)
{
    QString errorMsg;
    
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = "FFmpeg 启动失败。请检查 FFmpeg 是否正确安装。";
        break;
    case QProcess::Crashed:
        errorMsg = "FFmpeg 进程崩溃。";
        break;
    case QProcess::Timedout:
        errorMsg = "FFmpeg 进程超时。";
        break;
    case QProcess::WriteError:
        errorMsg = "写入 FFmpeg 进程时出错。";
        break;
    case QProcess::ReadError:
        errorMsg = "读取 FFmpeg 进程输出时出错。";
        break;
    default:
        errorMsg = "FFmpeg 未知错误。";
        break;
    }
    
    qDebug() << "FFmpeg error:" << errorMsg;
    emit errorOccurred(errorMsg);
}

void ScreenRecorder::onProcessOutput()
{
    QString output = m_ffmpegProcess->readAllStandardOutput();
    QString error = m_ffmpegProcess->readAllStandardError();
    
    if (!output.isEmpty()) {
        qDebug() << "FFmpeg output:" << output;
    }
    
    if (!error.isEmpty()) {
        qDebug() << "FFmpeg error output:" << error;
    }
}
