#ifndef QGSVERTEXEDITOR_H
#define QGSVERTEXEDITOR_H

// Библиотеки Qt
#include <QAbstractTableModel>
#include <QItemSelection>
#include <QStyledItemDelegate>
#include <QClipboard>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QStyledItemDelegate>
#include <QKeyEvent>
#include <QLineEdit>
#include <QVector2D>

// Библиотеки QGIS API
#include "qgsapplication.h"
#include "qgscoordinateutils.h"
#include "qgsmapcanvas.h"
#include "qgsmessagelog.h"
#include "qgslockedfeature.h"
#include "qgsvectorlayer.h"
#include "qgsgeometryutils.h"
#include "qgsproject.h"
#include "qgscoordinatetransform.h"
#include "qgsdoublevalidator.h"
#include "qgsdockwidget.h"
#include "qgspoint.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsvertexid.h"

class QLabel;
class QTableView;

class QgsMapCanvas;
class QgsLockedFeature;
class QgsVectorLayer;

/**
 * @class QgsVertexEntry
 * @brief Класс для хранения информации о вершине объекта
 *
 * Данный класс представляет информацию о вершине: координаты точки, идентификатор вершины и состояние выбора.
 */
class QgsVertexEntry
{
public:
    /**
     * @brief Конструктор
     *
     * Создает новую запись о вершине с заданными координатами и идентификатором.
     *
     * @param p координаты вершины
     * @param vertexId идентификатор вершины
     */
    QgsVertexEntry(const QgsPoint &p, QgsVertexId vertexId)
        : mSelected(false)
        , mPoint(p)
        , mVertexId(vertexId)
    {
    }

    QgsVertexEntry(const QgsVertexEntry &rh) = delete;

    QgsVertexEntry &operator=(const QgsVertexEntry &rh) = delete;

    /**
     * @brief Возвращает координаты вершины
     *
     * @return ссылка на точку с координатами вершины
     */
    const QgsPoint &point() const
    {
        return mPoint;
    }

    /**
     * @brief Возвращает идентификатор вершины
     *
     * @return идентификатор вершины
     */
    QgsVertexId vertexId() const
    {
        return mVertexId;
    }

    /**
     * @brief Проверяет, выбрана ли вершина
     *
     * @return true если вершина выбрана, false в противном случае
     */
    bool isSelected() const
    {
        return mSelected;
    }

    /**
     * @brief Устанавливает состояние выбора вершины
     *
     * @param selected новое состояние выбора (true - выбрана, false - нет)
     */
    void setSelected(bool selected)
    {
        mSelected = selected;
    }

private:
    bool mSelected;         // Флаг выбора вершины
    QgsPoint mPoint;        // Координаты точки вершины
    QgsVertexId mVertexId;  // Идентификатор вершины
};

/**
 * @class QgsVertexEditorModel
 * @brief Модель данных для редактора вершин
 *
 * Данный класс реализует таблицу для отображения и редактирования
 * координат вершин выбранного объекта. Поддерживает отображение координат
 * X, Y, Z, M и радиуса кривизны.
 */
class QgsVertexEditorModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    /**
     * @brief Конструктор
     *
     * Создает новую модель редактора вершин.
     *
     * @param canvas указатель на карту
     * @param parent указатель на родительский объект
     */
    QgsVertexEditorModel(QgsMapCanvas *canvas, QObject *parent = nullptr);

    /**
     * @brief Устанавливает редактируемый объект
     *
     * Привязывает модель к выбранному объекту и обновляет отображение.
     *
     * @param lockedFeature указатель на выбранный объект
     */
    void setFeature(QgsLockedFeature *lockedFeature);

    /**
     * @brief Возвращает количество строк в модели
     *
     * @param parent родительский индекс
     * @return количество строк (количество вершин объекта)
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief Возвращает количество столбцов в модели
     *
     * @param parent родительский индекс
     * @return количество столбцов (зависит от наличия Z, M, R)
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief Возвращает данные для указанной ячейки
     *
     * @param index индекс ячейки
     * @param role роль данных
     * @return данные ячейки в зависимости от роли
     */
    QVariant data(const QModelIndex &index, int role) const override;

    /**
     * @brief Возвращает данные заголовка столбца
     *
     * @param section номер столбца
     * @param orientation ориентация заголовка
     * @param role роль данных
     * @return данные заголовка
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @brief Устанавливает данные в ячейку
     *
     * Обновляет координаты вершины при редактировании.
     *
     * @param index индекс ячейки
     * @param value новое значение
     * @param role роль данных
     * @return true если данные успешно установлены
     */
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    /**
     * @brief Возвращает флаги элемента
     *
     * @param index индекс элемента
     * @return флаги элемента (доступность редактирования и т.д.)
     */
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    QgsLockedFeature *mLockedFeature = nullptr; // Указатель на редактируемый объект
    QgsMapCanvas *mCanvas = nullptr; // Указатель на карту

    bool mHasZ = false; // Флаг наличия Z-координаты
    bool mHasM = false; // Флаг наличия M-значения
    bool mHasR = true;  // Флаг наличия радиуса кривизны (всегда true для кривых)

    int mZCol = -1;     // Номер столбца с Z-координатой
    int mMCol = -1;     // Номер столбца с M-значением
    int mRCol = -1;     // Номер столбца с радиусом кривизны

    QFont mWidgetFont;  // Шрифт виджета

    /**
     * @brief Функция вычисления радиуса кривизны
     *
     * Рассчитывает радиус кривизны для указанной вершины.
     *
     * @param row номер строки (вершины)
     * @param r выходной параметр для радиуса
     * @param minRadius выходной параметр для минимального радиуса
     * @return true если радиус успешно вычислен
     */
    bool calcR(int row, double &r, double &minRadius) const;

};

/**
 * @class QgsVertexEditor
 * @brief Виджет редактора вершин
 *
 * Данный класс реализует плавающую панель для редактирования координат
 * вершин выбранного объекта в виде таблицы.
 */
class QgsVertexEditor : public QgsDockWidget
{
    Q_OBJECT
public:
    /**
     * @brief Конструктор
     *
     * Создает новый редактор вершин.
     *
     * @param canvas указатель на карту
     */
    QgsVertexEditor(QgsMapCanvas *canvas);

    /**
     * @brief Обновляет редактор для нового объекта
     *
     * Привязывает редактор к выбранному объекту.
     *
     * @param lockedFeature указатель на выбранный объект
     */
    void updateEditor(QgsLockedFeature *lockedFeature);

    QgsLockedFeature *mLockedFeature = nullptr;     // Указатель на редактируемый объект
    QgsMapCanvas *mCanvas = nullptr;                // Указатель на карту
    QTableView *mTableView = nullptr;               // Таблица для отображения вершин
    QgsVertexEditorModel *mVertexModel = nullptr;   // Модель данных таблицы

signals:
    /**
     * @brief Сигнал запроса удаления выбранных вершин
     *
     * Вызывается при необходимости удаления выбранных вершин.
     */
    void deleteSelectedRequested();

    /**
     * @brief Сигнал закрытия редактора
     *
     * Вызывается при закрытии редактора.
     */
    void editorClosed();

protected:
    /**
     * @brief Обработчик нажатия клавиш
     *
     * Обрабатывает клавиатурные сокращения редактора.
     *
     * @param event событие нажатия клавиши
     */
    void keyPressEvent(QKeyEvent *event) override;

    /**
     * @brief Обработчик закрытия виджета
     *
     * Вызывается при закрытии редактора.
     *
     * @param event событие закрытия
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    /**
     * @brief Обновляет выбор в таблице
     *
     * Синхронизирует выбор в таблице с выбором вершин на карте.
     */
    void updateTableSelection();

    /**
     * @brief Обновляет выбор вершин
     *
     * Синхронизирует выбор вершин на карте с выбором в таблице.
     *
     * @param selected новый выбор
     * @param deselected снятый выбор
     */
    void updateVertexSelection( const QItemSelection &, const QItemSelection &deselected );

private:
    QLabel *mHintLabel = nullptr;           // Метка с подсказкой
    bool mUpdatingTableSelection = false;   // Флаг обновления выбора в таблице
    bool mUpdatingVertexSelection = false;  // Флаг обновления выбора вершин
};

/**
 * @class CoordinateItemDelegate
 * @brief Делегат для редактирования координат
 *
 * Данный класс реализует делегат для отображения и редактирования
 * координат в таблице редактора вершин с учетом системы координат.
 */
class CoordinateItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:

    /**
     * @brief Конструктор
     *
     * Создает новый делегат для редактирования координат.
     *
     * @param crs система координат для форматирования
     * @param parent указатель на родительский объект
     */
    explicit CoordinateItemDelegate(const QgsCoordinateReferenceSystem &crs, QObject *parent = nullptr);

    /**
     * @brief Возвращает текст для отображения
     *
     * Форматирует значение координаты для отображения с учетом текущей локализацией.
     *
     * @param value значение координаты
     * @param locale локализация
     * @return отформатированная строка
     */
    QString displayText(const QVariant &value, const QLocale &locale) const override;

protected:
    /**
     * @brief Создает редактор ячейки
     *
     * @param parent родительский виджет
     * @param option параметры отображения
     * @param index индекс ячейки
     * @return указатель на созданный редактор
     */
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex &index) const override;

    /**
     * @brief Устанавливает данные из редактора в модель
     *
     * @param editor редактор
     * @param model модель данных
     * @param index индекс ячейки
     */
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    /**
     * @brief Устанавливает данные в редактор
     *
     * @param editor редактор
     * @param index индекс ячейки
     */
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

private:
    /**
     * @brief Возвращает количество знаков после запятой
     *
     * Определяет точность отображения координат.
     *
     * @return количество знаков после запятой
     */
    int displayDecimalPlaces() const;

    QgsCoordinateReferenceSystem mCrs; // Система координат
};

#endif // QGSVERTEXEDITOR_H
