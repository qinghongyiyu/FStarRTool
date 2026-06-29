#include "recordingcontrolwidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QStyle>

RecordingControlWidget::RecordingControlWidget(QWidget *parent)
    : QWidget(parent)
    , m_recordingSeconds(0)
    , m_isRecording(false)
    , m_isPaused(false)
    , m_dragging(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(300, 80);
    
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &RecordingControlWidget::updateTime);
    
    setupUI();
}

RecordingControlWidget::~RecordingControlWidget()
{
}

void RecordingControlWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(5);
    
    // Status and time display
    QHBoxLayout *displayLayout = new QHBoxLayout();
    
    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setStyleSheet("QLabel { color: white; font-weight: bold; }");
    
    m_timeLabel = new QLabel("00:00:00", this);
    m_timeLabel->setStyleSheet("QLabel { color: #4CAF50; font-weight: bold; font-size: 14px; }");
    m_timeLabel->setAlignment(Qt::AlignRight);
    
    displayLayout->addWidget(m_statusLabel);
    displayLayout->addStretch();
    displayLayout->addWidget(m_timeLabel);
    
    // Control buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(5);
    
    m_recordButton = new QPushButton("●", this);
    m_recordButton->setFixedSize(40, 40);
    m_recordButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; border-radius: 20px; font-size: 20px; font-weight: bold; }"
        "QPushButton:hover { background-color: #da190b; }"
        "QPushButton:pressed { background-color: #c41408; }");
    m_recordButton->setToolTip("开始录制");
    connect(m_recordButton, &QPushButton::clicked, this, &RecordingControlWidget::recordClicked);
    
    m_pauseButton = new QPushButton("||", this);
    m_pauseButton->setFixedSize(40, 40);
    m_pauseButton->setStyleSheet(
        "QPushButton { background-color: #FF9800; color: white; border-radius: 20px; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #e68900; }"
        "QPushButton:pressed { background-color: #cc7a00; }");
    m_pauseButton->setToolTip("暂停");
    m_pauseButton->setEnabled(false);
    connect(m_pauseButton, &QPushButton::clicked, this, &RecordingControlWidget::pauseClicked);
    
    m_stopButton = new QPushButton("■", this);
    m_stopButton->setFixedSize(40, 40);
    m_stopButton->setStyleSheet(
        "QPushButton { background-color: #607D8B; color: white; border-radius: 20px; font-size: 20px; font-weight: bold; }"
        "QPushButton:hover { background-color: #546E7A; }"
        "QPushButton:pressed { background-color: #455A64; }");
    m_stopButton->setToolTip("停止录制");
    m_stopButton->setEnabled(false);
    connect(m_stopButton, &QPushButton::clicked, this, &RecordingControlWidget::stopClicked);
    
    m_minimizeButton = new QPushButton("−", this);
    m_minimizeButton->setFixedSize(30, 30);
    m_minimizeButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; border-radius: 15px; font-size: 18px; font-weight: bold; }"
        "QPushButton:hover { background-color: #1976D2; }");
    m_minimizeButton->setToolTip("最小化");
    connect(m_minimizeButton, &QPushButton::clicked, this, &RecordingControlWidget::minimizeClicked);
    
    m_closeButton = new QPushButton("×", this);
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; border-radius: 15px; font-size: 18px; font-weight: bold; }"
        "QPushButton:hover { background-color: #da190b; }");
    m_closeButton->setToolTip("关闭");
    connect(m_closeButton, &QPushButton::clicked, this, &RecordingControlWidget::closeClicked);
    
    buttonLayout->addWidget(m_recordButton);
    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_minimizeButton);
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(displayLayout);
    mainLayout->addLayout(buttonLayout);
}

void RecordingControlWidget::startRecording()
{
    m_isRecording = true;
    m_isPaused = false;
    m_recordingSeconds = 0;
    
    m_recordButton->setEnabled(false);
    m_pauseButton->setEnabled(true);
    m_stopButton->setEnabled(true);
    
    m_statusLabel->setText("录制中");
    m_statusLabel->setStyleSheet("QLabel { color: #f44336; font-weight: bold; }");
    
    m_timer->start(1000);
}

void RecordingControlWidget::stopRecording()
{
    m_isRecording = false;
    m_isPaused = false;
    
    m_recordButton->setEnabled(true);
    m_pauseButton->setEnabled(false);
    m_stopButton->setEnabled(false);
    
    m_statusLabel->setText("就绪");
    m_statusLabel->setStyleSheet("QLabel { color: white; font-weight: bold; }");
    
    m_timer->stop();
    m_recordingSeconds = 0;
    m_timeLabel->setText("00:00:00");
}

void RecordingControlWidget::pauseRecording()
{
    if (m_isPaused) {
        resumeRecording();
    } else {
        m_isPaused = true;
        m_timer->stop();
        m_pauseButton->setText("▶");
        m_statusLabel->setText("已暂停");
        m_statusLabel->setStyleSheet("QLabel { color: #FF9800; font-weight: bold; }");
    }
}

void RecordingControlWidget::resumeRecording()
{
    m_isPaused = false;
    m_timer->start(1000);
    m_pauseButton->setText("||");
    m_statusLabel->setText("录制中");
    m_statusLabel->setStyleSheet("QLabel { color: #f44336; font-weight: bold; }");
}

void RecordingControlWidget::updateTime()
{
    m_recordingSeconds++;
    
    int hours = m_recordingSeconds / 3600;
    int minutes = (m_recordingSeconds % 3600) / 60;
    int seconds = m_recordingSeconds % 60;
    
    m_timeLabel->setText(QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0')));
}

void RecordingControlWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw rounded rectangle background
    painter.setBrush(QColor(30, 30, 30, 230));
    painter.setPen(QPen(QColor(100, 100, 100), 2));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10, 10);
}

void RecordingControlWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void RecordingControlWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}
