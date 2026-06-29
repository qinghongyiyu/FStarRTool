#include "screenselector.h"
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QWindow>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

ScreenSelector::ScreenSelector(QWidget *parent)
    : QWidget(parent)
    , m_mode(None)
    , m_selecting(false)
    , m_finished(false)
    , m_rubberBand(nullptr)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

ScreenSelector::~ScreenSelector()
{
    if (m_rubberBand) {
        delete m_rubberBand;
    }
}

QRect ScreenSelector::selectRegion()
{
    m_mode = Region;
    m_selecting = false;
    m_finished = false;
    m_selectedRect = QRect();
    
    captureScreen();
    
    // Show fullscreen on all screens
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        setGeometry(screen->geometry());
    }
    
    showFullScreen();
    
    // Create rubber band
    if (!m_rubberBand) {
        m_rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    }
    
    // Wait for selection
    QEventLoop loop;
    connect(this, &QWidget::destroyed, &loop, &QEventLoop::quit);
    
    while (!m_finished && isVisible()) {
        QApplication::processEvents();
    }
    
    hide();
    
    return m_selectedRect;
}

QRect ScreenSelector::selectWindow()
{
    m_mode = Window;
    m_selecting = false;
    m_finished = false;
    m_selectedRect = QRect();
    
    captureScreen();
    
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        setGeometry(screen->geometry());
    }
    
    showFullScreen();
    
    // Wait for selection
    while (!m_finished && isVisible()) {
        QApplication::processEvents();
    }
    
    hide();
    
    return m_selectedRect;
}

QRect ScreenSelector::selectFullScreen()
{
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        return screen->geometry();
    }
    return QRect();
}

void ScreenSelector::captureScreen()
{
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        m_screenPixmap = screen->grabWindow(0);
    }
}

void ScreenSelector::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    
    // Draw screenshot
    if (!m_screenPixmap.isNull()) {
        painter.drawPixmap(0, 0, m_screenPixmap);
    }
    
    // Draw semi-transparent overlay
    painter.fillRect(rect(), QColor(0, 0, 0, 100));
    
    if (m_mode == Region && m_selecting) {
        // Clear selected area
        QRect selectedArea = QRect(m_startPoint, m_currentPoint).normalized();
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(selectedArea, Qt::transparent);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        
        // Draw original screenshot in selected area
        painter.drawPixmap(selectedArea, m_screenPixmap, selectedArea);
        
        // Draw selection border
        painter.setPen(QPen(QColor(0, 120, 215), 2));
        painter.drawRect(selectedArea);
        
        // Draw dimensions
        drawDimensions(painter, selectedArea);
    } else if (m_mode == Window) {
        // Highlight window under cursor
        QRect windowRect = getWindowGeometry(m_currentPoint);
        if (!windowRect.isNull()) {
            painter.setCompositionMode(QPainter::CompositionMode_Clear);
            painter.fillRect(windowRect, Qt::transparent);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            
            painter.drawPixmap(windowRect, m_screenPixmap, windowRect);
            
            painter.setPen(QPen(QColor(255, 0, 0), 3));
            painter.drawRect(windowRect);
            
            drawDimensions(painter, windowRect);
        }
    }
    
    // Draw crosshair
    drawCrosshair(painter, m_currentPoint);
}

void ScreenSelector::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_mode == Region) {
            m_startPoint = event->pos();
            m_currentPoint = event->pos();
            m_selecting = true;
            
            if (m_rubberBand) {
                m_rubberBand->setGeometry(QRect(m_startPoint, QSize()));
                m_rubberBand->show();
            }
        } else if (m_mode == Window) {
            m_selectedRect = getWindowGeometry(event->pos());
            m_finished = true;
            close();
        }
    } else if (event->button() == Qt::RightButton) {
        m_finished = true;
        m_selectedRect = QRect();
        close();
    }
    
    update();
}

void ScreenSelector::mouseMoveEvent(QMouseEvent *event)
{
    m_currentPoint = event->pos();
    
    if (m_mode == Region && m_selecting) {
        QRect rect = QRect(m_startPoint, m_currentPoint).normalized();
        if (m_rubberBand) {
            m_rubberBand->setGeometry(rect);
        }
    }
    
    update();
}

void ScreenSelector::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_mode == Region && m_selecting) {
        m_selecting = false;
        m_selectedRect = QRect(m_startPoint, m_currentPoint).normalized();
        
        if (m_rubberBand) {
            m_rubberBand->hide();
        }
        
        // Ensure minimum size
        if (m_selectedRect.width() < 10 || m_selectedRect.height() < 10) {
            m_selectedRect = QRect();
        }
        
        m_finished = true;
        close();
    }
}

void ScreenSelector::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        m_finished = true;
        m_selectedRect = QRect();
        close();
    }
}

QRect ScreenSelector::getWindowGeometry(const QPoint &pos)
{
#ifdef Q_OS_WIN
    POINT pt;
    pt.x = pos.x();
    pt.y = pos.y();
    
    HWND hwnd = WindowFromPoint(pt);
    if (hwnd) {
        // Get the top-level window
        while (GetParent(hwnd) != NULL) {
            hwnd = GetParent(hwnd);
        }
        
        RECT rect;
        if (GetWindowRect(hwnd, &rect)) {
            return QRect(rect.left, rect.top, 
                        rect.right - rect.left, 
                        rect.bottom - rect.top);
        }
    }
#else
    Q_UNUSED(pos);
#endif
    
    return QRect();
}

void ScreenSelector::drawCrosshair(QPainter &painter, const QPoint &pos)
{
    painter.setPen(QPen(QColor(255, 255, 255), 1));
    
    // Horizontal line
    painter.drawLine(0, pos.y(), width(), pos.y());
    
    // Vertical line
    painter.drawLine(pos.x(), 0, pos.x(), height());
    
    // Draw position text
    QString posText = QString("(%1, %2)").arg(pos.x()).arg(pos.y());
    QRect textRect = painter.fontMetrics().boundingRect(posText);
    textRect.moveTopLeft(pos + QPoint(10, 10));
    
    painter.fillRect(textRect.adjusted(-2, -2, 2, 2), QColor(0, 0, 0, 180));
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, posText);
}

void ScreenSelector::drawDimensions(QPainter &painter, const QRect &rect)
{
    QString dimText = QString("%1 x %2").arg(rect.width()).arg(rect.height());
    QRect textRect = painter.fontMetrics().boundingRect(dimText);
    
    // Position text at top-left of selection
    QPoint textPos = rect.topLeft() + QPoint(5, -5);
    if (textPos.y() < textRect.height()) {
        textPos.setY(rect.top() + textRect.height() + 5);
    }
    
    textRect.moveTopLeft(textPos);
    
    painter.fillRect(textRect.adjusted(-3, -3, 3, 3), QColor(0, 120, 215, 200));
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, dimText);
}
