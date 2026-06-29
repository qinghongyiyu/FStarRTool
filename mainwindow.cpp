#include "mainwindow.h"
#include "screenrecorder.h"
#include "screenselector.h"
#include "settingsdialog.h"
#include "regionselectdialog.h"
#include <QApplication>
#include <QScreen>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QDateTime>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QStyle>
#include <QMenuBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , recorder(nullptr)
    , screenSelector(nullptr)
    , settingsDialog(nullptr)
    , recordingSeconds(0)
    , isPaused(false)
    , quality(23)
    , frameRate(30)
    , recordAudio(true)
    , recordMicrophone(false)
{
    setWindowTitle("FStarRTool - 屏幕录像工具");
    resize(555, 320);

    setFixedSize(555,320);

    // Initialize components
    recorder = new ScreenRecorder(this);
    screenSelector = new ScreenSelector(this);
    settingsDialog = new SettingsDialog(this);
    recordingTimer = new QTimer(this);
    
    // Setup UI
    setupUI();
    createActions();
    createMenus();
    // createToolBar();  // 工具栏已删除，功能都在菜单中
    createStatusBar();
    createTrayIcon();
    
    // Connect signals
    connect(recorder, &ScreenRecorder::recordingStarted, this, &MainWindow::onRecordingStarted);
    connect(recorder, &ScreenRecorder::recordingStopped, this, &MainWindow::onRecordingStopped);
    connect(recorder, &ScreenRecorder::recordingPaused, this, &MainWindow::onRecordingPaused);
    connect(recorder, &ScreenRecorder::recordingResumed, this, &MainWindow::onRecordingResumed);
    connect(recorder, &ScreenRecorder::errorOccurred, this, &MainWindow::onRecordingError);
    connect(recordingTimer, &QTimer::timeout, this, &MainWindow::updateRecordingTime);
    
    // Load settings
    loadSettings();
    
    // Set default output path
    outputPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (outputPath.isEmpty()) {
        outputPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    
    // Set default recording region to full screen
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        recordingRegion = screen->geometry();
    }
    
    updateUIState(false);
}

MainWindow::~MainWindow()
{
    saveSettings();
    if (recorder && recorder->isRecording()) {
        recorder->stop();
    }
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Recording area selection group
    QGroupBox *areaGroup = new QGroupBox("录制区域", centralWidget);
    QHBoxLayout *areaLayout = new QHBoxLayout(areaGroup);
    
    fullScreenButton = new QPushButton("全屏", areaGroup);
    fullScreenButton->setMinimumHeight(35);
    fullScreenButton->setToolTip("录制整个屏幕");
    connect(fullScreenButton, &QPushButton::clicked, this, &MainWindow::selectFullScreen);
    
    windowButton = new QPushButton("窗口", areaGroup);
    windowButton->setMinimumHeight(35);
    windowButton->setToolTip("选择窗口进行录制");
    connect(windowButton, &QPushButton::clicked, this, &MainWindow::selectWindow);
    
    regionButton = new QPushButton("区域", areaGroup);
    regionButton->setMinimumHeight(35);
    regionButton->setToolTip("选择自定义区域");
    connect(regionButton, &QPushButton::clicked, this, &MainWindow::selectRegion);
    
    areaLayout->addWidget(fullScreenButton);
    areaLayout->addWidget(windowButton);
    areaLayout->addWidget(regionButton);
    
    // Recording settings group
    QGroupBox *settingsGroup = new QGroupBox("录制设置", centralWidget);
    QHBoxLayout *settingsLayout = new QHBoxLayout(settingsGroup);
    
    // Video codec
    QLabel *videoCodecLabel = new QLabel("视频编码:", settingsGroup);
    videoCodecCombo = new QComboBox(settingsGroup);
    videoCodecCombo->addItem("H.264 (推荐)", "libx264");
    videoCodecCombo->addItem("H.265 (HEVC)", "libx265");
    videoCodecCombo->addItem("VP9", "libvpx-vp9");
    videoCodecCombo->addItem("AV1", "libaom-av1");
    videoCodecCombo->setCurrentIndex(0);
    connect(videoCodecCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onVideoCodecChanged);
    
    // Audio codec
    QLabel *audioCodecLabel = new QLabel("音频编码:", settingsGroup);
    audioCodecCombo = new QComboBox(settingsGroup);
    audioCodecCombo->addItem("AAC (推荐)", "aac");
    audioCodecCombo->addItem("MP3", "libmp3lame");
    audioCodecCombo->addItem("Opus", "libopus");
    audioCodecCombo->addItem("无音频", "none");
    audioCodecCombo->setCurrentIndex(0); // Default to AAC
    audioCodecCombo->setToolTip("选择AAC可录制麦克风声音");
    connect(audioCodecCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onAudioCodecChanged);
    
    // Quality
    QLabel *qualityLabel = new QLabel("质量 (CRF):", settingsGroup);
    qualitySpinBox = new QSpinBox(settingsGroup);
    qualitySpinBox->setRange(0, 51);
    qualitySpinBox->setValue(23);
    qualitySpinBox->setToolTip("0=最佳质量, 51=最低质量, 推荐18-28");
    connect(qualitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &MainWindow::onQualityChanged);
    
    // Frame rate
    QLabel *frameRateLabel = new QLabel("帧率:", settingsGroup);
    frameRateSpinBox = new QSpinBox(settingsGroup);
    frameRateSpinBox->setRange(10, 60);
    frameRateSpinBox->setValue(30);
    frameRateSpinBox->setSuffix(" fps");
    connect(frameRateSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &MainWindow::onFrameRateChanged);
    
    settingsLayout->addWidget(videoCodecLabel);
    settingsLayout->addWidget(videoCodecCombo);
    settingsLayout->addWidget(audioCodecLabel);
    settingsLayout->addWidget(audioCodecCombo);
    settingsLayout->addWidget(qualityLabel);
    settingsLayout->addWidget(qualitySpinBox);
    settingsLayout->addWidget(frameRateLabel);
    settingsLayout->addWidget(frameRateSpinBox);
    settingsLayout->addStretch();
    
    // Control buttons group
    QGroupBox *controlGroup = new QGroupBox("录制控制", centralWidget);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);
    
    recordButton = new QPushButton("开始录制", controlGroup);
    recordButton->setMinimumHeight(40);
    recordButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; border-radius: 5px; }"
                                 "QPushButton:hover { background-color: #45a049; }"
                                 "QPushButton:pressed { background-color: #3d8b40; }");
    connect(recordButton, &QPushButton::clicked, this, &MainWindow::startRecording);
    
    pauseButton = new QPushButton("暂停", controlGroup);
    pauseButton->setMinimumHeight(40);
    pauseButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-weight: bold; border-radius: 5px; }"
                                "QPushButton:hover { background-color: #e68900; }"
                                "QPushButton:pressed { background-color: #cc7a00; }");
    pauseButton->setEnabled(false);
    connect(pauseButton, &QPushButton::clicked, this, &MainWindow::pauseRecording);
    
    stopButton = new QPushButton("停止", controlGroup);
    stopButton->setMinimumHeight(40);
    stopButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; border-radius: 5px; }"
                               "QPushButton:hover { background-color: #da190b; }"
                               "QPushButton:pressed { background-color: #c41408; }");
    stopButton->setEnabled(false);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopRecording);
    
    controlLayout->addWidget(recordButton);
    controlLayout->addWidget(pauseButton);
    controlLayout->addWidget(stopButton);
    
    // Add all groups to main layout
    mainLayout->addWidget(areaGroup);
    mainLayout->addWidget(settingsGroup);
    mainLayout->addWidget(controlGroup);
    
    setCentralWidget(centralWidget);
}

void MainWindow::createActions()
{
    startAction = new QAction("开始录制", this);
    startAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(startAction, &QAction::triggered, this, &MainWindow::startRecording);
    
    stopAction = new QAction("停止录制", this);
    stopAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    stopAction->setEnabled(false);
    connect(stopAction, &QAction::triggered, this, &MainWindow::stopRecording);
    
    pauseAction = new QAction("暂停/继续", this);
    pauseAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    pauseAction->setEnabled(false);
    connect(pauseAction, &QAction::triggered, this, &MainWindow::pauseRecording);
    
    fullScreenAction = new QAction("全屏录制", this);
    fullScreenAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
    connect(fullScreenAction, &QAction::triggered, this, &MainWindow::selectFullScreen);
    
    windowAction = new QAction("窗口录制", this);
    windowAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    connect(windowAction, &QAction::triggered, this, &MainWindow::selectWindow);
    
    regionAction = new QAction("区域录制", this);
    regionAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_A));
    connect(regionAction, &QAction::triggered, this, &MainWindow::selectRegion);
    
    settingsAction = new QAction("设置", this);
    settingsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    
    exitAction = new QAction("退出", this);
    exitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    
    aboutAction = new QAction("关于", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    
    showHideAction = new QAction("显示/隐藏", this);
    connect(showHideAction, &QAction::triggered, this, [this]() {
        if (isVisible()) {
            hide();
        } else {
            show();
            activateWindow();
        }
    });
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu("文件");
    fileMenu->addAction(settingsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);
    
    recordMenu = menuBar()->addMenu("录制");
    recordMenu->addAction(startAction);
    recordMenu->addAction(pauseAction);
    recordMenu->addAction(stopAction);
    recordMenu->addSeparator();
    recordMenu->addAction(fullScreenAction);
    recordMenu->addAction(windowAction);
    recordMenu->addAction(regionAction);
    
    helpMenu = menuBar()->addMenu("帮助");
    helpMenu->addAction(aboutAction);
}

// 工具栏功能已删除，所有功能通过菜单访问
/*
void MainWindow::createToolBar()
{
    mainToolBar = addToolBar("主工具栏");
    mainToolBar->setMovable(false);
    mainToolBar->setIconSize(QSize(24, 24));
    
    mainToolBar->addAction(startAction);
    mainToolBar->addAction(pauseAction);
    mainToolBar->addAction(stopAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(fullScreenAction);
    mainToolBar->addAction(windowAction);
    mainToolBar->addAction(regionAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(settingsAction);
}
*/

void MainWindow::createStatusBar()
{
    statusLabel = new QLabel("就绪", this);
    statusLabel->setMinimumWidth(200);
    statusBar()->addWidget(statusLabel);
    
    statusBar()->addPermanentWidget(new QLabel("  |  ", this));
    
    timeLabel = new QLabel("00:00:00", this);
    timeLabel->setMinimumWidth(80);
    timeLabel->setStyleSheet("QLabel { font-weight: bold; color: #2196F3; }");
    statusBar()->addPermanentWidget(timeLabel);
}

void MainWindow::createTrayIcon()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    trayIcon->setToolTip("FStarRTool - 屏幕录像工具");
    
    trayMenu = new QMenu(this);
    trayMenu->addAction(showHideAction);
    trayMenu->addSeparator();
    trayMenu->addAction(startAction);
    trayMenu->addAction(pauseAction);
    trayMenu->addAction(stopAction);
    trayMenu->addSeparator();
    trayMenu->addAction(exitAction);
    
    trayIcon->setContextMenu(trayMenu);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayIconActivated);
    
    trayIcon->show();
}

void MainWindow::startRecording()
{
    if (recorder->isRecording()) {
        QMessageBox::warning(this, "警告", "录制已在进行中！");
        return;
    }
    
    // Generate output filename
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString filename = QString("ScreenRecord_%1.mp4").arg(timestamp);
    QString fullPath = QDir(outputPath).filePath(filename);
    
    // Ask user for save location
    QString selectedPath = QFileDialog::getSaveFileName(this, 
        "保存录像文件", 
        fullPath,
        "视频文件 (*.mp4 *.avi *.mkv);;所有文件 (*.*)");
    
    if (selectedPath.isEmpty()) {
        return;
    }
    
    // Set recording parameters
    recorder->setOutputFile(selectedPath);
    recorder->setRecordingRegion(recordingRegion);
    recorder->setVideoCodec(videoCodec);
    recorder->setAudioCodec(audioCodec);
    recorder->setQuality(quality);
    recorder->setFrameRate(frameRate);
    recorder->setRecordAudio(recordAudio);
    recorder->setAudioDevice(audioDevice);
    
    // Start recording
    if (recorder->start()) {
        recordingSeconds = 0;
        isPaused = false;
        recordingTimer->start(1000);
        updateUIState(true);
        statusLabel->setText("正在录制...");
        
        // Minimize to tray if needed
        if (settingsDialog && settingsDialog->minimizeOnRecord()) {
            hide();
        }
    } else {
        QMessageBox::critical(this, "错误", "无法启动录制！\n请检查FFmpeg配置和录制参数。");
    }
}

void MainWindow::stopRecording()
{
    if (!recorder->isRecording()) {
        return;
    }
    
    recorder->stop();
    recordingTimer->stop();
    updateUIState(false);
    statusLabel->setText("录制已停止");
    
    if (isHidden()) {
        show();
        activateWindow();
    }
}

void MainWindow::pauseRecording()
{
    if (!recorder->isRecording()) {
        return;
    }
    
    if (isPaused) {
        resumeRecording();
    } else {
        recorder->pause();
        recordingTimer->stop();
        isPaused = true;
        pauseButton->setText("继续");
        statusLabel->setText("录制已暂停");
    }
}

void MainWindow::resumeRecording()
{
    if (!recorder->isRecording() || !isPaused) {
        return;
    }
    
    recorder->resume();
    recordingTimer->start(1000);
    isPaused = false;
    pauseButton->setText("暂停");
    statusLabel->setText("正在录制...");
}

void MainWindow::selectFullScreen()
{
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        recordingRegion = screen->geometry();
        statusLabel->setText(QString("录制区域: 全屏 (%1x%2)")
            .arg(recordingRegion.width())
            .arg(recordingRegion.height()));
    }
}

void MainWindow::selectWindow()
{
    hide();
    QTimer::singleShot(500, this, [this]() {
        QRect selectedRect = screenSelector->selectWindow();
        if (!selectedRect.isNull()) {
            recordingRegion = selectedRect;
            statusLabel->setText(QString("录制区域: 窗口 (%1x%2)")
                .arg(recordingRegion.width())
                .arg(recordingRegion.height()));
        }
        show();
    });
}

void MainWindow::selectRegion()
{
    RegionSelectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QRect selectedRect = dialog.getSelectedRegion();
        if (!selectedRect.isNull() && !selectedRect.isEmpty()) {
            recordingRegion = selectedRect;
            statusLabel->setText(QString("录制区域: 自定义 (%1x%2)")
                .arg(recordingRegion.width())
                .arg(recordingRegion.height()));
        }
    }
}

void MainWindow::openSettings()
{
    if (settingsDialog->exec() == QDialog::Accepted) {
        outputPath = settingsDialog->getOutputPath();
        recordAudio = settingsDialog->getRecordAudio();
        recordMicrophone = settingsDialog->getRecordMicrophone();
        audioDevice = settingsDialog->getAudioDevice();
        saveSettings();
    }
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "关于 FStarRTool",
        "<h2>FStarRTool 屏幕录像工具</h2>"
        "<p>版本: 1.0.0</p>"
        "<p>基于 Qt 6.5 和 FFmpeg 8.0.1 开发</p>"
        "<p>仿照 FastStone Capture 功能实现</p>"
        "<p><b>功能特性:</b></p>"
        "<ul>"
        "<li>全屏、窗口、区域录制</li>"
        "<li>多种视频编码格式支持</li>"
        "<li>音频录制支持</li>"
        "<li>暂停/继续录制</li>"
        "<li>系统托盘支持</li>"
        "</ul>"
        "<p>© 2025 FStarRTool. All rights reserved.</p>");
}

void MainWindow::onRecordingStarted()
{
    statusLabel->setText("正在录制...");
    trayIcon->setToolTip("FStarRTool - 正在录制");
}

void MainWindow::onRecordingStopped()
{
    statusLabel->setText("录制已停止");
    trayIcon->setToolTip("FStarRTool - 屏幕录像工具");
    
    QString outputFile = recorder->getOutputFile();
    if (!outputFile.isEmpty()) {
        QMessageBox::information(this, "录制完成", 
            QString("录像已保存到:\n%1").arg(outputFile));
    }
}

void MainWindow::onRecordingPaused()
{
    statusLabel->setText("录制已暂停");
}

void MainWindow::onRecordingResumed()
{
    statusLabel->setText("正在录制...");
}

void MainWindow::onRecordingError(const QString &error)
{
    QMessageBox::critical(this, "录制错误", error);
    stopRecording();
}

void MainWindow::updateRecordingTime()
{
    if (!isPaused) {
        recordingSeconds++;
    }
    
    int hours = recordingSeconds / 3600;
    int minutes = (recordingSeconds % 3600) / 60;
    int seconds = recordingSeconds % 60;
    
    timeLabel->setText(QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0')));
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible()) {
            hide();
        } else {
            show();
            activateWindow();
        }
    }
}

void MainWindow::onVideoCodecChanged(int index)
{
    videoCodec = videoCodecCombo->itemData(index).toString();
}

void MainWindow::onAudioCodecChanged(int index)
{
    audioCodec = audioCodecCombo->itemData(index).toString();
    recordAudio = (audioCodec != "none");
}

void MainWindow::onQualityChanged(int value)
{
    quality = value;
}

void MainWindow::onFrameRateChanged(int value)
{
    frameRate = value;
}

void MainWindow::updateUIState(bool recording)
{
    recordButton->setEnabled(!recording);
    pauseButton->setEnabled(recording);
    stopButton->setEnabled(recording);
    
    fullScreenButton->setEnabled(!recording);
    windowButton->setEnabled(!recording);
    regionButton->setEnabled(!recording);
    
    videoCodecCombo->setEnabled(!recording);
    audioCodecCombo->setEnabled(!recording);
    qualitySpinBox->setEnabled(!recording);
    frameRateSpinBox->setEnabled(!recording);
    
    startAction->setEnabled(!recording);
    stopAction->setEnabled(recording);
    pauseAction->setEnabled(recording);
    fullScreenAction->setEnabled(!recording);
    windowAction->setEnabled(!recording);
    regionAction->setEnabled(!recording);
    
    if (!recording) {
        timeLabel->setText("00:00:00");
        pauseButton->setText("暂停");
        isPaused = false;
    }
}

void MainWindow::saveSettings()
{
    QSettings settings("FStarRTool", "ScreenRecorder");
    settings.setValue("outputPath", outputPath);
    settings.setValue("videoCodec", videoCodec);
    settings.setValue("audioCodec", audioCodec);
    settings.setValue("audioDevice", audioDevice);
    settings.setValue("quality", quality);
    settings.setValue("frameRate", frameRate);
    settings.setValue("recordAudio", recordAudio);
    settings.setValue("recordMicrophone", recordMicrophone);
    settings.setValue("geometry", saveGeometry());
}

void MainWindow::loadSettings()
{
    QSettings settings("FStarRTool", "ScreenRecorder");
    outputPath = settings.value("outputPath", 
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)).toString();
    videoCodec = settings.value("videoCodec", "libx264").toString();
    audioCodec = settings.value("audioCodec", "aac").toString(); // Default to AAC (enable audio by default)
    audioDevice = settings.value("audioDevice", "").toString();
    quality = settings.value("quality", 23).toInt();
    frameRate = settings.value("frameRate", 30).toInt();
    recordAudio = settings.value("recordAudio", true).toBool(); // Default to true (enable audio)
    recordMicrophone = settings.value("recordMicrophone", false).toBool();
    
    restoreGeometry(settings.value("geometry").toByteArray());
    
    // Update UI
    qualitySpinBox->setValue(quality);
    frameRateSpinBox->setValue(frameRate);
    
    // Set codec combo boxes
    int videoIndex = videoCodecCombo->findData(videoCodec);
    if (videoIndex >= 0) {
        videoCodecCombo->setCurrentIndex(videoIndex);
    }
    
    int audioIndex = audioCodecCombo->findData(audioCodec);
    if (audioIndex >= 0) {
        audioCodecCombo->setCurrentIndex(audioIndex);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (recorder && recorder->isRecording()) {
        QMessageBox::StandardButton reply = QMessageBox::question(this,
            "确认退出",
            "录制正在进行中，确定要退出吗？\n录制将被停止。",
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            recorder->stop();
            saveSettings();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        saveSettings();
        event->accept();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) {
            hide();
            event->ignore();
        }
    }
    QMainWindow::changeEvent(event);
}
