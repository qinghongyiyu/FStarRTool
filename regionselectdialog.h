#ifndef REGIONSELECTDIALOG_H
#define REGIONSELECTDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QRect>

class RegionSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegionSelectDialog(QWidget *parent = nullptr);
    ~RegionSelectDialog();
    
    QRect getSelectedRegion() const { return selectedRegion; }
    
    enum SelectionMode {
        WindowMode,
        RectangleMode,
        FixedSizeMode,
        ScreenNoTaskbarMode,
        FullScreenMode,
        LastRegionMode
    };

private slots:
    void selectWindow();
    void selectRectangle();
    void selectFixedSize();
    void selectScreenNoTaskbar();
    void selectFullScreen();
    void selectLastRegion();

private:
    void setupUI();
    void saveLastRegion();
    void loadLastRegion();
    
    QPushButton *windowButton;
    QPushButton *rectangleButton;
    QPushButton *fixedSizeButton;
    QPushButton *screenNoTaskbarButton;
    QPushButton *fullScreenButton;
    QPushButton *lastRegionButton;
    
    QSpinBox *widthSpinBox;
    QSpinBox *heightSpinBox;
    
    QLabel *infoLabel;
    
    QRect selectedRegion;
    QRect lastRegion;
};

#endif // REGIONSELECTDIALOG_H
