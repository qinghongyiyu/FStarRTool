#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QTimer>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>

class ScreenRecorder;
class RecordingControlWidget;
class ScreenSelector;
class SettingsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void startRecording();
    void stopRecording();
    void pauseRecording();
    void resumeRecording();
    void selectFullScreen();
    void selectWindow();
    void selectRegion();
    void openSettings();
    void showAbout();
    void onRecordingStarted();
    void onRecordingStopped();
    void onRecordingPaused();
    void onRecordingResumed();
    void onRecordingError(const QString &error);
    void updateRecordingTime();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onVideoCodecChanged(int index);
    void onAudioCodecChanged(int index);
    void onQualityChanged(int value);
    void onFrameRateChanged(int value);

private:
    void createActions();
    void createMenus();
    // void createToolBar();  // 工具栏已删除
    void createStatusBar();
    void createTrayIcon();
    void setupUI();
    void updateUIState(bool recording);
    void saveSettings();
    void loadSettings();

    // UI Components
    // QToolBar *mainToolBar;  // 工具栏已删除
    QLabel *statusLabel;
    QLabel *timeLabel;
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    
    // Control widgets
    QComboBox *videoCodecCombo;
    QComboBox *audioCodecCombo;
    QSpinBox *qualitySpinBox;
    QSpinBox *frameRateSpinBox;
    QPushButton *recordButton;
    QPushButton *pauseButton;
    QPushButton *stopButton;
    QPushButton *regionButton;
    QPushButton *fullScreenButton;
    QPushButton *windowButton;
    
    // Actions
    QAction *startAction;
    QAction *stopAction;
    QAction *pauseAction;
    QAction *fullScreenAction;
    QAction *windowAction;
    QAction *regionAction;
    QAction *settingsAction;
    QAction *exitAction;
    QAction *aboutAction;
    QAction *showHideAction;
    
    // Menus
    QMenu *fileMenu;
    QMenu *recordMenu;
    QMenu *helpMenu;
    
    // Core components
    ScreenRecorder *recorder;
    ScreenSelector *screenSelector;
    SettingsDialog *settingsDialog;
    
    // Recording state
    QTimer *recordingTimer;
    int recordingSeconds;
    bool isPaused;
    
    // Recording settings
    QString outputPath;
    QString videoCodec;
    QString audioCodec;
    QString audioDevice;
    int quality;
    int frameRate;
    bool recordAudio;
    bool recordMicrophone;
    QRect recordingRegion;
};

#endif // MAINWINDOW_H
