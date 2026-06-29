#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>

class ScreenRecorder;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    QString getOutputPath() const;
    bool getRecordAudio() const;
    bool getRecordMicrophone() const;
    bool minimizeOnRecord() const;
    bool showNotifications() const;
    QString getVideoFormat() const;
    int getBitrate() const;
    QString getAudioDevice() const;
    
    void setOutputPath(const QString &path);
    void setRecordAudio(bool record);
    void setRecordMicrophone(bool record);

private slots:
    void browseOutputPath();
    void onAccepted();
    void onRejected();
    void restoreDefaults();
    void refreshAudioDevices();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void loadAudioDevices();
    
    // Output settings
    QLineEdit *m_outputPathEdit;
    QPushButton *m_browseButton;
    QComboBox *m_formatCombo;
    
    // Video settings
    QSpinBox *m_bitrateSpinBox;
    QCheckBox *m_hardwareAccelCheckBox;
    
    // Audio settings
    QCheckBox *m_recordAudioCheckBox;
    QCheckBox *m_recordMicrophoneCheckBox;
    QComboBox *m_audioDeviceCombo;
    QPushButton *m_refreshAudioButton;
    
    // General settings
    QCheckBox *m_minimizeOnRecordCheckBox;
    QCheckBox *m_showNotificationsCheckBox;
    QCheckBox *m_startWithSystemCheckBox;
    
    // Hotkey settings
    QLineEdit *m_startHotkeyEdit;
    QLineEdit *m_stopHotkeyEdit;
    QLineEdit *m_pauseHotkeyEdit;
};

#endif // SETTINGSDIALOG_H
