#ifndef RECORDINGCONTROLWIDGET_H
#define RECORDINGCONTROLWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

class RecordingControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RecordingControlWidget(QWidget *parent = nullptr);
    ~RecordingControlWidget();

    void startRecording();
    void stopRecording();
    void pauseRecording();
    void resumeRecording();
    
    bool isRecording() const { return m_isRecording; }
    bool isPaused() const { return m_isPaused; }
    int getRecordingTime() const { return m_recordingSeconds; }

signals:
    void recordClicked();
    void stopClicked();
    void pauseClicked();
    void minimizeClicked();
    void closeClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void updateTime();

private:
    void setupUI();
    
    QPushButton *m_recordButton;
    QPushButton *m_pauseButton;
    QPushButton *m_stopButton;
    QPushButton *m_minimizeButton;
    QPushButton *m_closeButton;
    QLabel *m_timeLabel;
    QLabel *m_statusLabel;
    
    QTimer *m_timer;
    int m_recordingSeconds;
    bool m_isRecording;
    bool m_isPaused;
    
    QPoint m_dragPosition;
    bool m_dragging;
};

#endif // RECORDINGCONTROLWIDGET_H
