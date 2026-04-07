#ifndef MASKEDMENU_H
#define MASKEDMENU_H

#include <QMenu>
#include <QPainterPath>

/**
 * @brief Класс MaskedMenu
 *
 * Наследник QMenu с поддержкой скруглённых углов.
 * Скругление достигается за счёт применения маски (QRegion),
 * сформированной из QPainterPath.
 */
class MaskedMenu : public QMenu
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор класса MaskedMenu
     * @param parent Родительский виджет
     */
    explicit MaskedMenu(QWidget *parent = nullptr);

    /**
     * @brief Функция установки радиуса скругления углов меню
     * @param radius Радиус скругления в пикселях
     */
    void setCornerRadius(int radius);

protected:
    /**
     * @brief Обработчик события показа меню
     * @param event Событие показа виджета
     */
    void showEvent(QShowEvent *event) override;

private:
    int m_radius = 20; // Радиус скругления углов меню
};

#endif // MASKEDMENU_H
