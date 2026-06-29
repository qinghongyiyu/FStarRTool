#ifndef SCREENSELECTOR_H
#define SCREENSELECTOR_H

#include <QWidget>
#include <QRect>
#include <QPoint>
#include <QPixmap>
#include <QRubberBand>

class ScreenSelector : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenSelector(QWidget *parent = nullptr);
    ~ScreenSelector();

    QRect selectRegion();
    QRect selectWindow();
    QRect selectFullScreen();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void captureScreen();
    QRect getWindowGeometry(const QPoint &pos);
    void drawCrosshair(QPainter &painter, const QPoint &pos);
    void drawDimensions(QPainter &painter, const QRect &rect);
    
    enum SelectionMode {
        None,
        Region,
        Window
    };
    
    SelectionMode m_mode;
    QPixmap m_screenPixmap;
    QPoint m_startPoint;
    QPoint m_currentPoint;
    QRect m_selectedRect;
    bool m_selecting;
    bool m_finished;
    
    QRubberBand *m_rubberBand;
};

#endif // SCREENSELECTOR_H
