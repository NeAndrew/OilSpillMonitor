#include "detectionrenderer.h"

DetectionRenderer::DetectionRenderer()
    : m_drawMode(DrawMode::Labels)
    , m_confidenceThreshold(0.5)
{
    m_labelFont.setPointSize(30);
    m_labelFont.setBold(true);

    m_classStyles["Oil-Spill"] =
    {
    {QColor(255, 100, 100, 200), QColor(200, 50, 50, 255)},
            "Oil-Spill"
};
    
    m_classStyles["Land"] =
    {
    {QColor(120, 200, 120, 200), QColor(60, 150, 60, 255)},
            "Land"
};
    
    // Для других классов
    m_classStyles["default"] =
    {
    {QColor(100, 180, 255, 180), QColor(50, 120, 200, 255)},
            "Unknown"
};
}

void DetectionRenderer::setDrawMode(DrawMode mode)
{
    m_drawMode = mode;
}

void DetectionRenderer::setConfidenceThreshold(double threshold)
{
    m_confidenceThreshold = threshold;
}

void DetectionRenderer::setLabelFont(const QFont &font)
{
    m_labelFont = font;
}

void DetectionRenderer::setColorScheme(const QString &className, const ColorScheme &scheme)
{
    m_classStyles[className] = {scheme, className};
}

QPixmap DetectionRenderer::renderImage(const QPixmap &original, const QJsonArray &predictions) const
{
    if (original.isNull())
    {
        return QPixmap();
    }

    QPixmap result = original.copy();
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setFont(m_labelFont);

    for (const QJsonValue &val : predictions)
    {
        QJsonObject pred = val.toObject();

        double conf = pred["confidence"].toDouble();
        if (conf < m_confidenceThreshold)
        {
            continue;
        }

        QString className = pred["class"].toString();
        Style style = getClassStyle(className);

        // Отрисовка полигона
        if (pred.contains("points") && pred["points"].isArray())
        {
            QPolygonF polygon;
            QJsonArray pts = pred["points"].toArray();

            for (const QJsonValue &p : std::as_const(pts))
            {
                QJsonObject pt = p.toObject();
                polygon << QPointF(pt["x"].toDouble(), pt["y"].toDouble());
            }

            if (polygon.isEmpty())
            {
                continue;
            }

            drawPolygon(painter, polygon, style);

            // Отрисовка метки если нужно
            if (m_drawMode == DrawMode::Labels)
            {
                QPointF center(0, 0);
                for (const QPointF &p : std::as_const(polygon))
                {
                    center += p;
                }
                center /= polygon.size();

                QString label = QString("%1 %2%").arg(style.displayName).arg(int(conf * 100));
                drawLabel(painter, center, label, m_labelFont);
            }
        }
    }

    painter.end();
    return result;
}

DetectionRenderer::Style DetectionRenderer::getClassStyle(const QString &className) const
{
    if (m_classStyles.contains(className))
    {
        return m_classStyles[className];
    }
    return m_classStyles["default"];
}

void DetectionRenderer::drawPolygon(QPainter &painter, const QPolygonF &polygon, const Style &style) const
{
    // Создание градиента
    QLinearGradient grad(polygon.boundingRect().topLeft(), polygon.boundingRect().bottomRight());
    grad.setColorAt(0, style.scheme.fillColor);
    QColor endColor = style.scheme.fillColor;
    endColor.setAlpha(style.scheme.fillColor.alpha() / 2);
    grad.setColorAt(1, endColor);

    painter.setBrush(QBrush(grad));
    painter.setPen(QPen(style.scheme.borderColor, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPolygon(polygon);
}

void DetectionRenderer::drawLabel(QPainter &painter, const QPointF &center, const QString &text, const QFont &font) const
{
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(text);
    textRect.adjust(-8, -4, 8, 4);
    textRect.moveCenter(center.toPoint());

    // Фон метки
    QColor labelBg(135, 206, 235, 220);
    QColor labelText(30, 50, 70, 255);

    painter.setPen(Qt::NoPen);
    painter.setBrush(labelBg);
    painter.drawRoundedRect(textRect, 5, 5);

    // Текст метки
    painter.setPen(labelText);
    painter.drawText(textRect, Qt::AlignCenter, text);
}
