#ifndef DETECTIONRENDERER_H
#define DETECTIONRENDERER_H

// Библиотеки Qt
#include <QJsonArray>
#include <QPen>
#include <QBrush>
#include <QPainter>
#include <QJsonObject>

/**
 * @brief Класс для отрисовки результатов детекции нефтяных пятен
 *
 * Класс реализует визуализацию детекций на изображении с настраиваемыми
 * режимами отображения, порогами уверенности и цветовыми схемами.
 */
class DetectionRenderer
{
public:
    //  Перечисление режимов отрисовки
    enum class DrawMode
    {
        Labels, // Отображение меток с классами и уверенностью
        Shapes  // Отображение только фигур без меток
    };

    // Структура цветовой заливки классов обнаружений
    struct ColorScheme
    {
        QColor fillColor;   // Цвет заливки
        QColor borderColor; // Цвет границы
    };

    // Конструктор рендерера
    DetectionRenderer();

    /**
     * @brief Установка режима отрисовки
     * @param mode Режим отрисовки
     */
    void setDrawMode(DrawMode mode);
    
    /**
     * @brief Установка порога уверенности
     * @param threshold Порог уверенности (0.0-1.0)
     */
    void setConfidenceThreshold(double threshold);
    
    /**
     * @brief Установка шрифта для меток
     * @param font Шрифт для отображения меток
     */
    void setLabelFont(const QFont &font);
    
    /**
     * @brief Установка цветовой схемы для класса
     * @param className Имя класса
     * @param scheme Цветовая схема
     */
    void setColorScheme(const QString &className, const ColorScheme &scheme);

    /**
     * @brief Отрисовка изображения с детекциями
     * @param original Исходное изображение
     * @param predictions Массив предсказаний
     * @return Изображение с наложенными детекциями
     */
    QPixmap renderImage(const QPixmap &original, const QJsonArray &predictions) const;

private:
     // Структура стиля для отрисовки класса
    struct Style
    {
        ColorScheme scheme;    // Цветовая схема
        QString displayName;   // Отображаемое имя класса
    };

    /**
     * @brief Получение стиля для класса
     * @param className Имя класса
     * @return Стиль для отрисовки
     */
    Style getClassStyle(const QString &className) const;
    
    /**
     * @brief Отрисовка полигона
     * @param painter Объект для рисования
     * @param polygon Полигон для отрисовки
     * @param style Стиль отрисовки
     */
    void drawPolygon(QPainter &painter, const QPolygonF &polygon, const Style &style) const;
    
    /**
     * @brief Отрисовка метки
     * @param painter Объект для рисования
     * @param center Центр метки
     * @param text Текст метки
     * @param font Шрифт метки
     */
    void drawLabel(QPainter &painter, const QPointF &center, const QString &text, const QFont &font) const;

    DrawMode m_drawMode;                 // Режим отображения
    double m_confidenceThreshold;        // Порог уверенности
    QFont m_labelFont;                   // Шрифт меток
    QHash<QString, Style> m_classStyles; // Стили классов
};

#endif // DETECTIONRENDERER_H
