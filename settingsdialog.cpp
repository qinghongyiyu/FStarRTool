#include "settingsdialog.h"
#include "screenrecorder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSettings>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QProcess>
#include <QApplication>
#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("设置");
    setMinimumWidth(500);
    setMinimumHeight(400);
    
    setupUI();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    QTabWidget *tabWidget = new QTabWidget(this);
    
    // Output settings tab
    QWidget *outputTab = new QWidget();
    QVBoxLayout *outputLayout = new QVBoxLayout(outputTab);
    
    QGroupBox *outputGroup = new QGroupBox("输出设置", outputTab);
    QFormLayout *outputFormLayout = new QFormLayout(outputGroup);
    
    QHBoxLayout *pathLayout = new QHBoxLayout();
    m_outputPathEdit = new QLineEdit(outputGroup);
    m_outputPathEdit->setPlaceholderText("选择输出文件夹");
    m_browseButton = new QPushButton("浏览...", outputGroup);
    connect(m_browseButton, &QPushButton::clicked, this, &SettingsDialog::browseOutputPath);
    pathLayout->addWidget(m_outputPathEdit);
    pathLayout->addWidget(m_browseButton);
    outputFormLayout->addRow("输出路径:", pathLayout);
    
    m_formatCombo = new QComboBox(outputGroup);
    m_formatCombo->addItem("MP4 (推荐)", "mp4");
    m_formatCombo->addItem("AVI", "avi");
    m_formatCombo->addItem("MKV", "mkv");
    m_formatCombo->addItem("MOV", "mov");
    m_formatCombo->addItem("FLV", "flv");
    outputFormLayout->addRow("视频格式:", m_formatCombo);
    
    outputLayout->addWidget(outputGroup);
    outputLayout->addStretch();
    
    // Video settings tab
    QWidget *videoTab = new QWidget();
    QVBoxLayout *videoLayout = new QVBoxLayout(videoTab);
    
    QGroupBox *videoGroup = new QGroupBox("视频设置", videoTab);
    QFormLayout *videoFormLayout = new QFormLayout(videoGroup);
    
    m_bitrateSpinBox = new QSpinBox(videoGroup);
    m_bitrateSpinBox->setRange(500, 50000);
    m_bitrateSpinBox->setValue(4000);
    m_bitrateSpinBox->setSuffix(" kbps");
    m_bitrateSpinBox->setSingleStep(500);
    videoFormLayout->addRow("比特率:", m_bitrateSpinBox);
    
    m_hardwareAccelCheckBox = new QCheckBox("启用硬件加速 (如果可用)", videoGroup);
    m_hardwareAccelCheckBox->setChecked(true);
    videoFormLayout->addRow("", m_hardwareAccelCheckBox);
    
    videoLayout->addWidget(videoGroup);
    videoLayout->addStretch();
    
    // Audio settings tab
    QWidget *audioTab = new QWidget();
    QVBoxLayout *audioLayout = new QVBoxLayout(audioTab);
    
    QGroupBox *audioGroup = new QGroupBox("音频设置", audioTab);
    QVBoxLayout *audioGroupLayout = new QVBoxLayout(audioGroup);
    
    m_recordAudioCheckBox = new QCheckBox("录制系统音频", audioGroup);
    m_recordAudioCheckBox->setChecked(true); // Default to enabled
    audioGroupLayout->addWidget(m_recordAudioCheckBox);
    
    m_recordMicrophoneCheckBox = new QCheckBox("录制麦克风", audioGroup);
    m_recordMicrophoneCheckBox->setChecked(false);
    audioGroupLayout->addWidget(m_recordMicrophoneCheckBox);
    
    QFormLayout *audioFormLayout = new QFormLayout();
    
    QHBoxLayout *deviceLayout = new QHBoxLayout();
    m_audioDeviceCombo = new QComboBox(audioGroup);
    m_audioDeviceCombo->setMinimumWidth(300);
    m_refreshAudioButton = new QPushButton("刷新设备", audioGroup);
    m_refreshAudioButton->setMaximumWidth(100);
    connect(m_refreshAudioButton, &QPushButton::clicked, this, &SettingsDialog::refreshAudioDevices);
    deviceLayout->addWidget(m_audioDeviceCombo, 1);
    deviceLayout->addWidget(m_refreshAudioButton);
    
    audioFormLayout->addRow("音频设备:", deviceLayout);
    audioGroupLayout->addLayout(audioFormLayout);
    
    // Add help text
    QLabel *helpLabel = new QLabel(
        "<b>提示：</b><br>"
        "• 如果要录制系统声音，需要在Windows声音设置中启用\"立体声混音\"<br>"
        "• 如果要录制麦克风，请选择麦克风设备<br>"
        "• 点击\"刷新设备\"按钮可重新检测音频设备",
        audioGroup);
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { color: #666; padding: 10px; background-color: #f0f0f0; border-radius: 5px; }");
    audioGroupLayout->addWidget(helpLabel);
    
    audioLayout->addWidget(audioGroup);
    audioLayout->addStretch();
    
    // Load audio devices
    loadAudioDevices();
    
    // General settings tab
    QWidget *generalTab = new QWidget();
    QVBoxLayout *generalLayout = new QVBoxLayout(generalTab);
    
    QGroupBox *generalGroup = new QGroupBox("常规设置", generalTab);
    QVBoxLayout *generalGroupLayout = new QVBoxLayout(generalGroup);
    
    m_minimizeOnRecordCheckBox = new QCheckBox("录制时最小化到托盘", generalGroup);
    m_minimizeOnRecordCheckBox->setChecked(true);
    generalGroupLayout->addWidget(m_minimizeOnRecordCheckBox);
    
    m_showNotificationsCheckBox = new QCheckBox("显示通知", generalGroup);
    m_showNotificationsCheckBox->setChecked(true);
    generalGroupLayout->addWidget(m_showNotificationsCheckBox);
    
    m_startWithSystemCheckBox = new QCheckBox("开机自动启动", generalGroup);
    m_startWithSystemCheckBox->setChecked(false);
    generalGroupLayout->addWidget(m_startWithSystemCheckBox);
    
    generalLayout->addWidget(generalGroup);
    generalLayout->addStretch();
    
    // Hotkey settings tab
    QWidget *hotkeyTab = new QWidget();
    QVBoxLayout *hotkeyLayout = new QVBoxLayout(hotkeyTab);
    
    QGroupBox *hotkeyGroup = new QGroupBox("快捷键设置", hotkeyTab);
    QFormLayout *hotkeyFormLayout = new QFormLayout(hotkeyGroup);
    
    m_startHotkeyEdit = new QLineEdit(hotkeyGroup);
    m_startHotkeyEdit->setPlaceholderText("点击设置快捷键");
    m_startHotkeyEdit->setReadOnly(true);
    m_startHotkeyEdit->setText("Ctrl+R");
    hotkeyFormLayout->addRow("开始录制:", m_startHotkeyEdit);
    
    m_stopHotkeyEdit = new QLineEdit(hotkeyGroup);
    m_stopHotkeyEdit->setPlaceholderText("点击设置快捷键");
    m_stopHotkeyEdit->setReadOnly(true);
    m_stopHotkeyEdit->setText("Ctrl+S");
    hotkeyFormLayout->addRow("停止录制:", m_stopHotkeyEdit);
    
    m_pauseHotkeyEdit = new QLineEdit(hotkeyGroup);
    m_pauseHotkeyEdit->setPlaceholderText("点击设置快捷键");
    m_pauseHotkeyEdit->setReadOnly(true);
    m_pauseHotkeyEdit->setText("Ctrl+P");
    hotkeyFormLayout->addRow("暂停/继续:", m_pauseHotkeyEdit);
    
    hotkeyLayout->addWidget(hotkeyGroup);
    hotkeyLayout->addStretch();
    
    // Add tabs
    tabWidget->addTab(outputTab, "输出");
    tabWidget->addTab(videoTab, "视频");
    tabWidget->addTab(audioTab, "音频");
    tabWidget->addTab(generalTab, "常规");
    tabWidget->addTab(hotkeyTab, "快捷键");
    
    mainLayout->addWidget(tabWidget);
    
    // Button box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults,
        this);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::onRejected);
    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
            this, &SettingsDialog::restoreDefaults);
    
    mainLayout->addWidget(buttonBox);
}

QString SettingsDialog::getOutputPath() const
{
    return m_outputPathEdit->text();
}

bool SettingsDialog::getRecordAudio() const
{
    return m_recordAudioCheckBox->isChecked();
}

bool SettingsDialog::getRecordMicrophone() const
{
    return m_recordMicrophoneCheckBox->isChecked();
}

bool SettingsDialog::minimizeOnRecord() const
{
    return m_minimizeOnRecordCheckBox->isChecked();
}

bool SettingsDialog::showNotifications() const
{
    return m_showNotificationsCheckBox->isChecked();
}

QString SettingsDialog::getVideoFormat() const
{
    return m_formatCombo->currentData().toString();
}

int SettingsDialog::getBitrate() const
{
    return m_bitrateSpinBox->value();
}

QString SettingsDialog::getAudioDevice() const
{
    return m_audioDeviceCombo->currentData().toString();
}

void SettingsDialog::setOutputPath(const QString &path)
{
    m_outputPathEdit->setText(path);
}

void SettingsDialog::setRecordAudio(bool record)
{
    m_recordAudioCheckBox->setChecked(record);
}

void SettingsDialog::setRecordMicrophone(bool record)
{
    m_recordMicrophoneCheckBox->setChecked(record);
}

void SettingsDialog::browseOutputPath()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        "选择输出文件夹",
        m_outputPathEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!dir.isEmpty()) {
        m_outputPathEdit->setText(dir);
    }
}

void SettingsDialog::onAccepted()
{
    saveSettings();
    accept();
}

void SettingsDialog::onRejected()
{
    reject();
}

void SettingsDialog::restoreDefaults()
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (defaultPath.isEmpty()) {
        defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    
    m_outputPathEdit->setText(defaultPath);
    m_formatCombo->setCurrentIndex(0);
    m_bitrateSpinBox->setValue(4000);
    m_hardwareAccelCheckBox->setChecked(true);
    m_recordAudioCheckBox->setChecked(true);
    m_recordMicrophoneCheckBox->setChecked(false);
    m_minimizeOnRecordCheckBox->setChecked(true);
    m_showNotificationsCheckBox->setChecked(true);
    m_startWithSystemCheckBox->setChecked(false);
}

void SettingsDialog::loadSettings()
{
    QSettings settings("FStarRTool", "ScreenRecorder");
    
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (defaultPath.isEmpty()) {
        defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    
    m_outputPathEdit->setText(settings.value("outputPath", defaultPath).toString());
    
    QString format = settings.value("videoFormat", "mp4").toString();
    int formatIndex = m_formatCombo->findData(format);
    if (formatIndex >= 0) {
        m_formatCombo->setCurrentIndex(formatIndex);
    }
    
    m_bitrateSpinBox->setValue(settings.value("bitrate", 4000).toInt());
    m_hardwareAccelCheckBox->setChecked(settings.value("hardwareAccel", true).toBool());
    m_recordAudioCheckBox->setChecked(settings.value("recordAudio", true).toBool());
    m_recordMicrophoneCheckBox->setChecked(settings.value("recordMicrophone", false).toBool());
    m_minimizeOnRecordCheckBox->setChecked(settings.value("minimizeOnRecord", true).toBool());
    m_showNotificationsCheckBox->setChecked(settings.value("showNotifications", true).toBool());
    m_startWithSystemCheckBox->setChecked(settings.value("startWithSystem", false).toBool());
}

void SettingsDialog::saveSettings()
{
    QSettings settings("FStarRTool", "ScreenRecorder");
    
    settings.setValue("outputPath", m_outputPathEdit->text());
    settings.setValue("videoFormat", m_formatCombo->currentData().toString());
    settings.setValue("bitrate", m_bitrateSpinBox->value());
    settings.setValue("hardwareAccel", m_hardwareAccelCheckBox->isChecked());
    settings.setValue("recordAudio", m_recordAudioCheckBox->isChecked());
    settings.setValue("recordMicrophone", m_recordMicrophoneCheckBox->isChecked());
    settings.setValue("audioDevice", m_audioDeviceCombo->currentData().toString());
    settings.setValue("minimizeOnRecord", m_minimizeOnRecordCheckBox->isChecked());
    settings.setValue("showNotifications", m_showNotificationsCheckBox->isChecked());
    settings.setValue("startWithSystem", m_startWithSystemCheckBox->isChecked());
}

void SettingsDialog::loadAudioDevices()
{
    m_audioDeviceCombo->clear();
    m_audioDeviceCombo->addItem("正在检测音频设备...", "");
    
    // Get FFmpeg path
    QString ffmpegPath;
    QString relativePath = QApplication::applicationDirPath() + 
        "/../../../ffmpeg-8.0.1-full_build-shared/bin/ffmpeg.exe";
    
    QFileInfo relativeInfo(relativePath);
    if (relativeInfo.exists()) {
        ffmpegPath = QDir::toNativeSeparators(relativeInfo.absoluteFilePath());
    } else {
        QString projectPath = "E:/2025-TTTT/53--Qt Project Practical Recommendations--/FStarRTool/ffmpeg-8.0.1-full_build-shared/bin/ffmpeg.exe";
        QFileInfo projectInfo(projectPath);
        if (projectInfo.exists()) {
            ffmpegPath = QDir::toNativeSeparators(projectInfo.absoluteFilePath());
        }
    }
    
    if (ffmpegPath.isEmpty()) {
        m_audioDeviceCombo->clear();
        m_audioDeviceCombo->addItem("未找到 FFmpeg", "");
        return;
    }
    
    // Run FFmpeg to list devices
    QProcess process;
    QStringList args;
    args << "-list_devices" << "true" << "-f" << "dshow" << "-i" << "dummy";
    
    process.start(ffmpegPath, args);
    process.waitForFinished(5000);
    
    QString output = process.readAllStandardError();
    
    // Parse audio devices from output
    QStringList devices;
    QStringList lines = output.split('\n');
    bool inAudioSection = false;
    
    for (const QString &line : lines) {
        if (line.contains("DirectShow audio devices")) {
            inAudioSection = true;
            continue;
        }
        if (inAudioSection && line.contains("DirectShow video devices")) {
            break;
        }
        if (inAudioSection && line.contains("\"")) {
            int start = line.indexOf('"');
            int end = line.indexOf('"', start + 1);
            if (start >= 0 && end > start) {
                QString device = line.mid(start + 1, end - start - 1);
                devices.append(device);
            }
        }
    }
    
    // Update combo box
    m_audioDeviceCombo->clear();
    
    if (devices.isEmpty()) {
        m_audioDeviceCombo->addItem("未检测到音频设备", "");
        m_audioDeviceCombo->addItem("(请启用\"立体声混音\"或连接麦克风)", "");
    } else {
        for (const QString &device : devices) {
            // Add icon or prefix based on device type
            QString displayName = device;
            if (device.contains("立体声混音", Qt::CaseInsensitive) || 
                device.contains("Stereo Mix", Qt::CaseInsensitive)) {
                displayName = "🔊 " + device + " (系统声音)";
            } else if (device.contains("麦克风", Qt::CaseInsensitive) || 
                       device.contains("Microphone", Qt::CaseInsensitive)) {
                displayName = "🎤 " + device;
            }
            m_audioDeviceCombo->addItem(displayName, device);
        }
    }
    
    // Restore saved device
    QSettings settings("FStarRTool", "ScreenRecorder");
    QString savedDevice = settings.value("audioDevice", "").toString();
    if (!savedDevice.isEmpty()) {
        int index = m_audioDeviceCombo->findData(savedDevice);
        if (index >= 0) {
            m_audioDeviceCombo->setCurrentIndex(index);
        }
    }
}

void SettingsDialog::refreshAudioDevices()
{
    loadAudioDevices();
    QMessageBox::information(this, "刷新完成", 
        QString("检测到 %1 个音频设备").arg(m_audioDeviceCombo->count()));
}
