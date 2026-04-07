#include "advancedslider.h"

// Константы стилей
namespace SliderConstants
{
// от 0 до 72 часов
constexpr int DEFAULT_MIN = 0;
constexpr int DEFAULT_MAX = 72;
constexpr int DEFAULT_STEP = 1;
constexpr int DEFAULT_TICK_INTERVAL = 6;
constexpr int MARKER_STEP = 6;

// Цвета
const QColor SLIDER_HANDLE_COLOR = QColor(154, 206, 235); // #9ACEEB
const QColor SLIDER_HANDLE_HOVER_COLOR = QColor(191, 231, 255); // #bfe7ff
const QColor SLIDER_GROOVE_COLOR = QColor(171, 171, 171); // #ababab
const QColor MARKER_COLOR = QColor(102, 102, 102); // #666666

// Размеры
constexpr int HANDLE_SIZE = 16;
constexpr int HANDLE_BORDER = 2;
constexpr int GROOVE_HEIGHT = 3;
constexpr int MARKER_SIZE = 6;
constexpr int LABEL_OFFSET = 6;
constexpr int MARKER_VERTICAL_OFFSET = 20;

// Размеры лейбла
constexpr int LABEL_MIN_WIDTH = 28;
constexpr int LABEL_MAX_WIDTH = 40;
}

// Конструктор
advancedSlider::advancedSlider(Qt::Orientation orientation, QWidget *parent)
    : QWidget(parent)
    , m_orientation(orientation)
{
    // Создаем слайдер
    m_slider = new QSlider(orientation, this);
    setupSlider();
    setupSliderStyle();
    
    // Создаем метку-лейбл
    m_label = new QLabel(this);
    setupLabel();
    setupLayout();
    
    m_label->setText(QString::number(m_slider->value()));
    updateLabel();
    setupConnections();
}

void advancedSlider::setupSlider()
{
    m_slider->setRange(SliderConstants::DEFAULT_MIN, SliderConstants::DEFAULT_MAX);
    m_slider->setSingleStep(SliderConstants::DEFAULT_STEP);
    m_slider->setTickInterval(SliderConstants::DEFAULT_TICK_INTERVAL);
    m_slider->setTickPosition(QSlider::TicksBelow);
}

void advancedSlider::setupSliderStyle()
{
    QString sliderStyle = QString(
                "QSlider::groove:horizontal {"
                "    height: %1px;"
                "    background: %2;"
                "    border-radius: %3px;"
                "}"
                "QSlider::handle:horizontal {"
                "    background: %4;"
                "    border: %5px solid white;"
                "    width: %6px;"
                "    height: %6px;"
                "    margin: -%7px 0;"
                "    border-radius: %8px;"
                "}"
                "QSlider::sub-page:horizontal { background: %4; }"
                "QSlider::handle:horizontal:hover {"
                "    background: %9;"
                "    border: %5px solid #ffffff;"
                "}")
            .arg(SliderConstants::GROOVE_HEIGHT)
            .arg(SliderConstants::SLIDER_GROOVE_COLOR.name())
            .arg(SliderConstants::GROOVE_HEIGHT / 2)
            .arg(SliderConstants::SLIDER_HANDLE_COLOR.name())
            .arg(SliderConstants::HANDLE_BORDER)
            .arg(SliderConstants::HANDLE_SIZE)
            .arg(SliderConstants::HANDLE_SIZE / 2)
            .arg(SliderConstants::HANDLE_SIZE / 2)
            .arg(SliderConstants::SLIDER_HANDLE_HOVER_COLOR.name());
    
    m_slider->setStyleSheet(sliderStyle);
}

void advancedSlider::setupLabel()
{
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setMinimumWidth(SliderConstants::LABEL_MIN_WIDTH);
    m_label->setMaximumWidth(SliderConstants::LABEL_MAX_WIDTH);
    
    QString labelStyle = QString(
                "color: %1;"
                "font-weight: bold;"
                "background: rgba(220,220,220,180);"
                "padding: 2px 4px;"
                "border-radius: 4px;")
            .arg(SliderConstants::SLIDER_HANDLE_COLOR.name());
    m_label->setStyleSheet(labelStyle);
    m_label->raise();
}

void advancedSlider::setupLayout()
{
    QBoxLayout *layout = static_cast<QBoxLayout*>(new QHBoxLayout(this));
    layout->addWidget(m_slider);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
}

void advancedSlider::setupConnections()
{
    connect(m_slider, &QSlider::valueChanged, this, [this](int v)
    {
        m_label->setText(QString::number(v) + " ч");
        updateLabel();
        emit valueChanged(v);
    });
    
    connect(m_slider, &QSlider::rangeChanged, this, [this]()
    {
        updateLabel();
    });
}

QStyleOptionSlider advancedSlider::createStyleOption(int value) const
{
    QStyleOptionSlider opt;
    opt.initFrom(m_slider);
    opt.orientation = m_orientation;
    opt.minimum = m_slider->minimum();
    opt.maximum = m_slider->maximum();
    opt.sliderPosition = value;
    opt.sliderValue = value;
    opt.tickPosition = m_slider->tickPosition();
    opt.upsideDown = false;
    return opt;
}

int advancedSlider::value() const
{
    if (!m_slider)
    {
        return 0;
    }

    return m_slider->value();
}

void advancedSlider::setValue(int value)
{
    if (m_slider && value != m_slider->value())
    {
        m_slider->setValue(value);
    }
}

void advancedSlider::setRange(int min, int max)
{
    if (m_slider && (min != m_slider->minimum() || max != m_slider->maximum()))
    {
        m_slider->setRange(min, max);
    }
}

void advancedSlider::updateLabel()
{
    if (!m_slider || !m_label)
    {
        return;
    }

    QStyleOptionSlider opt = createStyleOption(m_slider->value());
    QRect handleRect = m_slider->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, m_slider);
    QPoint center = m_slider->mapTo(this, handleRect.center());

    int x = center.x() - m_label->width() / 2;
    int y = center.y() - handleRect.height() / 2 - m_label->height() - SliderConstants::LABEL_OFFSET;

    x = qBound(0, x, width() - m_label->width());
    y = qBound(0, y, height() - m_label->height());

    m_label->move(x, y);
}

void advancedSlider::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateLabel();
}

void advancedSlider::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    if (!m_slider)
    {
        return;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(SliderConstants::MARKER_COLOR);

    int min = m_slider->minimum();
    int max = m_slider->maximum();

    for (int v = min; v <= max; v += SliderConstants::MARKER_STEP)
    {
        QStyleOptionSlider opt = createStyleOption(v);
        QRect r = m_slider->style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, m_slider);
        QPoint c = m_slider->mapTo(this, r.center());
        p.drawEllipse(c.x() - SliderConstants::MARKER_SIZE/2,
                      c.y() + SliderConstants::MARKER_VERTICAL_OFFSET,
                      SliderConstants::MARKER_SIZE,
                      SliderConstants::MARKER_SIZE);
    }
}
