#include "regionselectdialog.h"
#include "screenselector.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QApplication>
#include <QScreen>
#include <QSettings>
#include <QStyle>
#include <QTimer>

RegionSelectDialog::RegionSelectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("选择录制区域");
    setMinimumWidth(500);
    
    loadLastRegion();
    setupUI();
}

RegionSelectDialog::~RegionSelectDialog()
{
}

void RegionSelectDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 信息标签
    infoLabel = new QLabel("请选择录制区域类型：", this);
    infoLabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; padding: 10px; }");
    mainLayout->addWidget(infoLabel);
    
    // 选择方式分组
    QGroupBox *selectionGroup = new QGroupBox("区域选择方式", this);
    QGridLayout *gridLayout = new QGridLayout(selectionGroup);
    gridLayout->setSpacing(10);
    
    // 窗口或对象
    windowButton = new QPushButton("🪟 窗口或对象", this);
    windowButton->setMinimumHeight(50);
    windowButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #2196F3; "
        "  color: white; "
        "  font-size: 11pt; "
        "  font-weight: bold; "
        "  border-radius: 5px; "
        "  padding: 10px; "
        "}"
        "QPushButton:hover { background-color: #1976D2; }"
        "QPushButton:pressed { background-color: #0D47A1; }"
    );
    windowButton->setToolTip("点击后选择要录制的窗口");
    connect(windowButton, &QPushButton::clicked, this, &RegionSelectDialog::selectWindow);
    gridLayout->addWidget(windowButton, 0, 0);
    
    // 矩形区域
    rectangleButton = new QPushButton("📐 矩形区域", this);
    rectangleButton->setMinimumHeight(50);
    rectangleButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #4CAF50; "
        "  color: white; "
        "  font-size: 11pt; "
        "  font-weight: bold; "
        "  border-radius: 5px; "
        "  padding: 10px; "
        "}"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #3d8b40; }"
    );
    rectangleButton->setToolTip("拖动鼠标选择矩形区域");
    connect(rectangleButton, &QPushButton::clicked, this, &RegionSelectDialog::selectRectangle);
    gridLayout->addWidget(rectangleButton, 0, 1);
    
    // 固定大小区域
    QWidget *fixedSizeWidget = new QWidget(this);
    QVBoxLayout *fixedLayout = new QVBoxLayout(fixedSizeWidget);
    fixedLayout->setContentsMargins(0, 0, 0, 0);
    
    fixedSizeButton = new QPushButton("📏 固定大小区域", this);
    fixedSizeButton->setMinimumHeight(40);
    fixedSizeButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #FF9800; "
        "  color: white; "
        "  font-size: 11pt; "
        "  font-weight: bold; "
        "  border-radius: 5px; "
        "  padding: 8px; "
        "}"
        "QPushButton:hover { background-color: #e68900; }"
        "QPushButton:pressed { background-color: #cc7a00; }"
    );
    fixedSizeButton->setToolTip("指定固定宽度和高度");
    connect(fixedSizeButton, &QPushButton::clicked, this, &RegionSelectDialog::selectFixedSize);
    
    QHBoxLayout *sizeLayout = new QHBoxLayout();
    QLabel *widthLabel = new QLabel("宽度:", this);
    widthSpinBox = new QSpinBox(this);
    widthSpinBox->setRange(100, 7680);
    widthSpinBox->setValue(1280);
    widthSpinBox->setSuffix(" px");
    
    QLabel *heightLabel = new QLabel("高度:", this);
    heightSpinBox = new QSpinBox(this);
    heightSpinBox->setRange(100, 4320);
    heightSpinBox->setValue(720);
    heightSpinBox->setSuffix(" px");
    
    sizeLayout->addWidget(widthLabel);
    sizeLayout->addWidget(widthSpinBox);
    sizeLayout->addWidget(heightLabel);
    sizeLayout->addWidget(heightSpinBox);
    
    fixedLayout->addWidget(fixedSizeButton);
    fixedLayout->addLayout(sizeLayout);
    
    gridLayout->addWidget(fixedSizeWidget, 1, 0);
    
    // 整个屏幕（不含任务栏）
    screenNoTaskbarButton = new QPushButton("🖥️ 整个屏幕 (不含任务栏)", this);
    screenNoTaskbarButton->setMinimumHeight(50);
    screenNoTaskbarButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #9C27B0; "
        "  color: white; "
        "  font-size: 11pt; "
        "  font-weight: bold; "
        "  border-radius: 5px; "
        "  padding: 10px; "
        "}"
        "QPushButton:hover { background-color: #7B1FA2; }"
        "QPushButton:pressed { background-color: #6A1B9A; }"
    );
    screenNoTaskbarButton->setToolTip("录制整个屏幕，但不包含任务栏");
    connect(screenNoTaskbarButton, &QPushButton::clicked, this, &RegionSelectDialog::selectScreenNoTaskbar);
    gridLayout->addWidget(screenNoTaskbarButton, 1, 1);
    
    // 整个屏幕
    fullScreenButton = new QPushButton("🌐 整个屏幕", this);
    fullScreenButton->setMinimumHeight(50);
    fullScreenButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #00BCD4; "
        "  color: white; "
        "  font-size: 11pt; "
        "  font-weight: bold; "
        "  border-radius: 5px; "
        "  padding: 10px; "
        "}"
        "QPushButton:hover { background-color: #0097A7; }"
        "QPushButton:pressed { background-color: #00838F; }"
    );
    fullScreenButton->setToolTip("录制整个屏幕，包含任务栏");
    connect(fullScreenButton, &QPushButton::clicked, this, &RegionSelectDialog::selectFullScreen);
    gridLayout->addWidget(fullScreenButton, 2, 0);
    
    // 重复上次区域
    lastRegionButton = new QPushButton("🔄 重复上次区域", this);
    lastRegionButton->setMinimumHeight(50);
    lastRegionButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #607D8B; "
        "  color: white; "
        "  font-size: 11pt; "
        "  font-weight: bold; "
        "  border-radius: 5px; "
        "  padding: 10px; "
        "}"
        "QPushButton:hover { background-color: #546E7A; }"
        "QPushButton:pressed { background-color: #455A64; }"
    );
    
    if (lastRegion.isNull() || lastRegion.isEmpty()) {
        lastRegionButton->setEnabled(false);
        lastRegionButton->setToolTip("没有上次的录制区域");
    } else {
        lastRegionButton->setToolTip(QString("使用上次的区域: %1x%2 at (%3, %4)")
            .arg(lastRegion.width())
            .arg(lastRegion.height())
            .arg(lastRegion.x())
            .arg(lastRegion.y()));
    }
    connect(lastRegionButton, &QPushButton::clicked, this, &RegionSelectDialog::selectLastRegion);
    gridLayout->addWidget(lastRegionButton, 2, 1);
    
    mainLayout->addWidget(selectionGroup);
    
    // 取消按钮
    QPushButton *cancelButton = new QPushButton("取消", this);
    cancelButton->setMinimumHeight(35);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    mainLayout->addWidget(cancelButton);
}

void RegionSelectDialog::selectWindow()
{
    hide();
    QTimer::singleShot(500, this, [this]() {
        ScreenSelector selector;
        selectedRegion = selector.selectWindow();
        if (!selectedRegion.isNull() && !selectedRegion.isEmpty()) {
            saveLastRegion();
            accept();
        } else {
            show();
        }
    });
}

void RegionSelectDialog::selectRectangle()
{
    hide();
    QTimer::singleShot(500, this, [this]() {
        ScreenSelector selector;
        selectedRegion = selector.selectRegion();
        if (!selectedRegion.isNull() && !selectedRegion.isEmpty()) {
            saveLastRegion();
            accept();
        } else {
            show();
        }
    });
}

void RegionSelectDialog::selectFixedSize()
{
    int width = widthSpinBox->value();
    int height = heightSpinBox->value();
    
    // 居中显示固定大小区域
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width) / 2;
        int y = (screenGeometry.height() - height) / 2;
        selectedRegion = QRect(x, y, width, height);
        saveLastRegion();
        accept();
    }
}

void RegionSelectDialog::selectScreenNoTaskbar()
{
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        // 获取可用几何区域（不包含任务栏）
        selectedRegion = screen->availableGeometry();
        saveLastRegion();
        accept();
    }
}

void RegionSelectDialog::selectFullScreen()
{
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        // 获取完整屏幕几何区域（包含任务栏）
        selectedRegion = screen->geometry();
        saveLastRegion();
        accept();
    }
}

void RegionSelectDialog::selectLastRegion()
{
    if (!lastRegion.isNull() && !lastRegion.isEmpty()) {
        selectedRegion = lastRegion;
        accept();
    }
}

void RegionSelectDialog::saveLastRegion()
{
    if (!selectedRegion.isNull() && !selectedRegion.isEmpty()) {
        QSettings settings("FStarRTool", "ScreenRecorder");
        settings.setValue("lastRegion/x", selectedRegion.x());
        settings.setValue("lastRegion/y", selectedRegion.y());
        settings.setValue("lastRegion/width", selectedRegion.width());
        settings.setValue("lastRegion/height", selectedRegion.height());
    }
}

void RegionSelectDialog::loadLastRegion()
{
    QSettings settings("FStarRTool", "ScreenRecorder");
    int x = settings.value("lastRegion/x", -1).toInt();
    int y = settings.value("lastRegion/y", -1).toInt();
    int width = settings.value("lastRegion/width", 0).toInt();
    int height = settings.value("lastRegion/height", 0).toInt();
    
    if (x >= 0 && y >= 0 && width > 0 && height > 0) {
        lastRegion = QRect(x, y, width, height);
    }
}
