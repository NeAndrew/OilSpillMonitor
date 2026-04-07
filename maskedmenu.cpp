#include "maskedmenu.h"

MaskedMenu::MaskedMenu(QWidget *parent)
    : QMenu(parent)
{
    setStyleSheet(R"(
        QMenu {
            background-color: white;
            border: 4px solid #9ACEEB;
            border-radius: 19px;
            padding: 4px;
        }

        QMenu::item {
            padding: 6px 20px;
            border-radius: 19px;
            color: #2b2b2b;
        }

        QMenu::item:selected {
            background-color: #9ACEEB;
            color: white;
        }

        QMenu::item:pressed {
            background-color: #d0d0d0;
            color: #2b2b2b;
        }
    )");
}

void MaskedMenu::setCornerRadius(int radius)
{
    m_radius = radius;
}

void MaskedMenu::showEvent(QShowEvent *event)
{
    // Вызываем базовую реализацию QMenu
    QMenu::showEvent(event);

    // Формируем путь со скруглёнными углами
    QPainterPath path;
    path.addRoundedRect(rect(), m_radius, m_radius);

    // Преобразуем путь в регион и применяем как маску
    QRegion mask(path.toFillPolygon().toPolygon());
    setMask(mask);
}
