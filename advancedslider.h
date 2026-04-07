#ifndef ADVANCEDSLIDER_H
#define ADVANCEDSLIDER_H

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QBoxLayout>
#include <QStyleOptionSlider>
#include <QPainter>

/**
 * @brief Класс advancedSlider представляет собой слайдер с кастомным стилем и меткой-лейблом.
 * Метка-лейбл показывает текущее значение слайдера и автоматически позиционируется над ползунком.
 */
class advancedSlider : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief Конструктор
     * @param orientation Ориентация слайдера
     * @param parent Родительский виджет
     */
    explicit advancedSlider(Qt::Orientation orientation, QWidget *parent = nullptr);

    /**
     * @brief Функция получения текущего значения слайдера
     * @return Значение слайдера
     */
    int value() const;

    /**
     * @brief Функция установки значения слайдера
     * @param value Новое значение слайдера
     */
    void setValue(int value);

    /**
     * @brief Функция установки диапазона значений слайдера
     * @param min Минимальное значение
     * @param max Максимальное значение
     */
    void setRange(int min, int max);

    /**
     * @brief Функция получения указателя на слайдер
     * @return Указатель на QSlider
     */
    QSlider* slider() const { return m_slider; }

signals:
    /**
     * @brief Сигнал изменения значения слайдера
     * @param value Новое значение
     */
    void valueChanged(int value);

protected:
    // Изменение размеров слайдера
    void resizeEvent(QResizeEvent *event) override;

    // Отрисовка маркеров (точек) под слайдером
    void paintEvent(QPaintEvent *event) override;

private:
    /**
     * @brief Функция перерисовки лейбла
     */
    void updateLabel();
    
    /**
     * @brief Настройка параметров слайдера
     */
    void setupSlider();
    
    /**
     * @brief Настройка стиля слайдера
     */
    void setupSliderStyle();
    
    /**
     * @brief Настройка лейбла
     */
    void setupLabel();
    
    /**
     * @brief Настройка компоновки
     */
    void setupLayout();
    
    /**
     * @brief Настройка соединений
     */
    void setupConnections();
    
    /**
     * @brief Создание QStyleOptionSlider для расчетов
     * @param value Значение для отображения на слайдере
     * @return Настроенный QStyleOptionSlider
     */
    QStyleOptionSlider createStyleOption(int value) const;

    Qt::Orientation m_orientation; // Ориентация слайдера
    QSlider *m_slider;             // Слайдер
    QLabel  *m_label;              // Метка-лейбл с текущим значением
};

#endif // ADVANCEDSLIDER_H
