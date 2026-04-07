#include "qgsvertexeditor.h"

static const int MIN_RADIUS_ROLE = Qt::UserRole + 1;

QgsVertexEditorModel::QgsVertexEditorModel(QgsMapCanvas *canvas, QObject *parent)
    : QAbstractTableModel(parent)
    , mCanvas(canvas)
{
    QWidget *parentWidget = qobject_cast<QWidget *>(parent);
    if (parentWidget)
    {
        mWidgetFont = parentWidget->font();
    }
}

void QgsVertexEditorModel::setFeature(QgsLockedFeature *lockedFeature)
{
    beginResetModel(); // Начинаем полный сброс модели

    mLockedFeature = lockedFeature;
    if (mLockedFeature && mLockedFeature->layer())
    {
        const Qgis::WkbType layerWKBType = mLockedFeature->layer()->wkbType();

        mHasZ = QgsWkbTypes::hasZ(layerWKBType); // Проверяем наличие Z-координаты в геометрии
        mHasM = QgsWkbTypes::hasM(layerWKBType); // Проверяем наличие M-значения (измерения)

        if (mHasZ)
        {
            mZCol = 2;
        }
        if (mHasM)
        {
            mMCol = 2 + (mHasZ ? 1 : 0); // M-колонка идёт после Z (если Z есть)
        }
        if (mHasR)
        {
            mRCol = 2 + ( mHasZ ? 1 : 0 ) + ( mHasM ? 1 : 0 ); // Колонка радиуса всегда последняя
        }
    }

    endResetModel(); // Завершаем сброс, представление обновляется
}

int QgsVertexEditorModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !mLockedFeature)
    {
        return 0;
    }

    return mLockedFeature->vertexMap().count();
}

int QgsVertexEditorModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (!mLockedFeature)
    {
        return 0;
    }
    else
    {
        return 2 + ( mHasZ ? 1 : 0 ) + ( mHasM ? 1 : 0 ) + ( mHasR ? 1 : 0 ); // X, Y + опционально Z, M, R
    }
}

QVariant QgsVertexEditorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !mLockedFeature || (role != Qt::DisplayRole && role != Qt::EditRole && role != MIN_RADIUS_ROLE && role != Qt::FontRole))
    {
        return QVariant();
    }

    if (index.row() >= mLockedFeature->vertexMap().count())
    {
        return QVariant();
    }

    if (index.column() >= columnCount())
    {
        return QVariant();
    }

    const QgsVertexEntry *vertex = mLockedFeature->vertexMap().at(index.row());
    if (!vertex)
    {
        return QVariant();
    }

    if (role == Qt::FontRole)
    {
        // Формируем шрифт. Жирный для выбранных, курсив для кривых
        double r = 0;
        double minRadius = 0;
        QFont font = mWidgetFont;
        bool fontChanged = false;
        if (vertex->isSelected())
        {
            font.setBold(true);
            fontChanged = true;
        }
        if (calcR(index.row(), r, minRadius))
        {
            font.setItalic(true);
            fontChanged = true;
        }
        if (fontChanged)
        {
            return font;
        }
        else
        {
            return QVariant();
        }
    }

    if (role == MIN_RADIUS_ROLE)
    {
        if (index.column() == mRCol)
        {
            double r = 0;
            double minRadius = 0;
            if (calcR(index.row(), r, minRadius))
            {
                return minRadius;
            }
        }
        return QVariant();
    }

    if (index.column() == 0)
    {
        return vertex->point().x();
    }
    else if (index.column() == 1)
    {
        return vertex->point().y();
    }
    else if (index.column() == mZCol)
    {
        return vertex->point().z();
    }
    else if (index.column() == mMCol)
    {
        return vertex->point().m();
    }
    else if (index.column() == mRCol)
    {
        double r = 0;
        double minRadius = 0;
        if (calcR(index.row(), r, minRadius))
        {
            return r;
        }
        return QVariant();
    }
    else
    {
        return QVariant();
    }

}

QVariant QgsVertexEditorModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Vertical)
        {
            return QVariant(section);
        }
        else
        {
            if (section == 0)
            {
                return QVariant(tr("x"));
            }
            else if (section == 1)
            {
                return QVariant(tr("y"));
            }
            else if (section == mZCol)
            {
                return QVariant(tr("z"));
            }
            else if (section == mMCol)
            {
                return QVariant(tr("m"));
            }
            else if (section == mRCol)
            {
                return QVariant(tr("r"));
            }
            else
            {
                return QVariant();
            }
        }
    }
    else if (role == Qt::ToolTipRole)
    {
        if (orientation == Qt::Vertical)
        {
            return QVariant(tr("Vertex %1").arg(section));
        }
        else
        {
            if (section == 0)
            {
                return QVariant(tr("X Coordinate"));
            }
            else if (section == 1)
            {
                return QVariant(tr("Y Coordinate"));
            }
            else if (section == mZCol)
            {
                return QVariant(tr("Z Coordinate"));
            }
            else if (section == mMCol)
            {
                return QVariant(tr("M Value"));
            }
            else if (section == mRCol)
            {
                return QVariant(tr("Radius Value"));
            }
            else
            {
                return QVariant();
            }
        }
    }
    else
    {
        return QVariant();
    }
}

bool QgsVertexEditorModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
    {
        return false;
    }
    if (!mLockedFeature || !mLockedFeature->layer() || index.row() >= mLockedFeature->vertexMap().count())
    {
        return false;
    }

    const double doubleValue { QgsDoubleValidator::toDouble( value.toString() ) };

    double x = (index.column() == 0 ? doubleValue : mLockedFeature->vertexMap().at(index.row())->point().x());
    double y = (index.column() == 1 ? doubleValue : mLockedFeature->vertexMap().at(index.row())->point().y());

    if ( index.column() == mRCol )
    {
        if ( index.row() == 0 || index.row() >= mLockedFeature->vertexMap().count() - 1 )
        {
            return false; // Первая и последняя вершины не могут быть кривыми
        }

        const double x1 = mLockedFeature->vertexMap().at(index.row() - 1)->point().x();
        const double y1 = mLockedFeature->vertexMap().at(index.row() - 1)->point().y();
        const double x2 = x;
        const double y2 = y;
        const double x3 = mLockedFeature->vertexMap().at(index.row() + 1)->point().x();
        const double y3 = mLockedFeature->vertexMap().at(index.row() + 1)->point().y();

        QgsPoint result;
        if (QgsGeometryUtils::segmentMidPoint(QgsPoint(x1, y1), QgsPoint(x3, y3), result, doubleValue, QgsPoint(x2, y2)))
        {
            // Перемещаем вершину на середину дуги окружности с заданным радиусом
            x = result.x();
            y = result.y();
        }
    }
    const double z = (index.column() == mZCol ? doubleValue : mLockedFeature->vertexMap().at(index.row())->point().z());
    const double m = (index.column() == mMCol ? doubleValue : mLockedFeature->vertexMap().at(index.row())->point().m());
    const QgsPoint p(Qgis::WkbType::PointZM, x, y, z, m); // Создаём точку со всеми координатами

    mLockedFeature->layer()->beginEditCommand(QObject::tr("Moved vertices")); // Начинаем команду отмены
    mLockedFeature->layer()->moveVertex(p, mLockedFeature->featureId(), index.row()); // Перемещаем вершину
    mLockedFeature->layer()->endEditCommand(); // Завершаем команду отмены
    mLockedFeature->layer()->triggerRepaint(); // Запрашиваем перерисовку карты

    return false;
}

Qt::ItemFlags QgsVertexEditorModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);

    if (index.isValid())
    {
        return flags | Qt::ItemIsEditable;
    }
    else
    {
        return flags;
    }
}

bool QgsVertexEditorModel::calcR(int row, double &r, double &minRadius) const
{
    if (row <= 0 || !mLockedFeature || row >= mLockedFeature->vertexMap().count() - 1)
    {
        return false;
    }

    const QgsVertexEntry *entry = mLockedFeature->vertexMap().at(row);

    const bool curvePoint = (entry->vertexId().type == Qgis::VertexType::Curve); // Проверяем тип вершины (кривая/обычная)
    if (!curvePoint)
    {
        return false;
    }

    const QgsPoint &p1 = mLockedFeature->vertexMap().at(row - 1)->point();
    const QgsPoint &p2 = mLockedFeature->vertexMap().at(row)->point();
    const QgsPoint &p3 = mLockedFeature->vertexMap().at(row + 1)->point();

    double cx, cy;
    QgsGeometryUtils::circleCenterRadius(p1, p2, p3, r, cx, cy); // Вычисляем центр и радиус окружности по 3 точкам

    double x13 = p3.x() - p1.x(), y13 = p3.y() - p1.y();
    minRadius = 0.5 * std::sqrt(x13 * x13 + y13 * y13); // Минимальный возможный радиус = половине длины хорды

    return true;
}

QgsVertexEditor::QgsVertexEditor(QgsMapCanvas *canvas)
    : mCanvas(canvas)
    , mVertexModel(new QgsVertexEditorModel(mCanvas, this))
{
    setWindowTitle(tr("Vertex Editor"));
    setObjectName(QStringLiteral("VertexEditor"));

    QWidget *content = new QWidget(this);
    content->setMinimumHeight(160);
    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);

    mHintLabel = new QLabel(this);
    mHintLabel->setText( QStringLiteral("%1\n\n%2").arg(
                             tr( "Щелкните правой кнопкой мыши по редактируемому объекту, чтобы отобразить таблицу его вершин." ),
                             tr( "Выделение вершин на холсте выберет вершины этого объекта в таблице." ) ) );
    mHintLabel->setWordWrap(true);
    mHintLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    mTableView = new QTableView(this);
    mTableView->setSelectionMode(QTableWidget::ExtendedSelection);
    mTableView->setSelectionBehavior(QTableWidget::SelectRows);
    mTableView->setVisible(false);
    mTableView->setModel(mVertexModel);

    // Синхронизация выбора в таблице с картой
    connect(mTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QgsVertexEditor::updateVertexSelection);

    layout->addWidget(mTableView);
    layout->addWidget(mHintLabel);

    setWidget(content);
}

void QgsVertexEditor::updateEditor(QgsLockedFeature *lockedFeature)
{
    mLockedFeature = lockedFeature;

    mVertexModel->setFeature(mLockedFeature);

    updateTableSelection();

    if (mLockedFeature)
    {
        mHintLabel->setVisible(false);
        mTableView->setVisible(true);

        connect(mLockedFeature, &QgsLockedFeature::selectionChanged, this, &QgsVertexEditor::updateTableSelection); // Синхронизация выбора на карте с таблицей

        if (mLockedFeature->layer())
        {
            const QgsCoordinateReferenceSystem crs = mLockedFeature->layer()->crs();
            // Устанавливаем делегаты для форматирования координат по системе координат слоя
            mTableView->setItemDelegateForColumn(0, new CoordinateItemDelegate(crs, this));
            mTableView->setItemDelegateForColumn(1, new CoordinateItemDelegate(crs, this));
            mTableView->setItemDelegateForColumn(2, new CoordinateItemDelegate(crs, this));
            mTableView->setItemDelegateForColumn(3, new CoordinateItemDelegate(crs, this));
            mTableView->setItemDelegateForColumn(4, new CoordinateItemDelegate(crs, this));
        }
    }
    else
    {
        mHintLabel->setVisible(true);
        mTableView->setVisible(false);
    }
}

void QgsVertexEditor::updateTableSelection()
{
    if (!mLockedFeature || mUpdatingVertexSelection || mUpdatingTableSelection) // Защита от рекурсии при обновлении выбора
    {
        return;
    }

    mUpdatingTableSelection = true;
    const QList<QgsVertexEntry *> &vertexMap = mLockedFeature->vertexMap();
    int firstSelectedRow = -1;
    QItemSelection selection;
    for (int i = 0, n = vertexMap.size(); i < n; ++i)
    {
        if (vertexMap[i]->isSelected())
        {
            if (firstSelectedRow < 0)
            {
                firstSelectedRow = i; // Запоминаем первую выбранную строку для прокрутки
            }
            selection.select(mVertexModel->index(i, 0), mVertexModel->index(i, mVertexModel->columnCount() - 1)); // Выбираем всю строку
        }
    }
    mTableView->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);

    if (firstSelectedRow >= 0)
    {
        mTableView->scrollTo(mVertexModel->index(firstSelectedRow, 0), QAbstractItemView::PositionAtTop); // Прокручиваем к первой выбранной строке
    }

    mUpdatingTableSelection = false;
}

void QgsVertexEditor::updateVertexSelection(const QItemSelection &, const QItemSelection &)
{
    if (!mLockedFeature || mUpdatingVertexSelection || mUpdatingTableSelection)
    {
        return;
    }

    mUpdatingVertexSelection = true;

    mLockedFeature->deselectAllVertices();

    const QgsCoordinateTransform t(mLockedFeature->layer()->crs(), mCanvas->mapSettings().destinationCrs(), QgsProject::instance()); // Трансформация координат слоя
    std::unique_ptr<QgsRectangle> bbox; // Охват выделенных вершин
    const QModelIndexList indexList = mTableView->selectionModel()->selectedRows();
    for (const QModelIndex &index : indexList)
    {
        const int vertexIdx = index.row();
        mLockedFeature->selectVertex(vertexIdx);

        const QgsPointXY point(mLockedFeature->vertexMap().at(vertexIdx)->point());
        if (!bbox)
        {
            bbox.reset(new QgsRectangle(point, point)); // Инициализируем охват первой точкой
        }
        else
        {
            bbox->combineExtentWith(point); // Расширяем охват на точку
        }
    }

    if (bbox)
    {
        try
        {
            QgsRectangle transformedBbox = t.transform(*bbox); // Переводим охват в CRS карты
            const QgsRectangle canvasExtent = mCanvas->mapSettings().visibleExtent();
            transformedBbox.combineExtentWith(canvasExtent); // Объединяем с текущим охватом (не уменьшаем)
            mCanvas->setExtent(transformedBbox, true); // Устанавливаем охват карты
            mCanvas->refresh(); // Перерисовываем
        }
        catch (QgsCsException &cse)
        {
            // Ошибка трансформации
        }
    }

    mUpdatingVertexSelection = false;
}

void QgsVertexEditor::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete)
    {
        emit deleteSelectedRequested();

        e->ignore();
    }
    else if (e->matches(QKeySequence::Copy))
    {
        if (!mTableView->selectionModel()->hasSelection())
        {
            return;
        }
        QString text;
        QItemSelectionRange range = mTableView->selectionModel()->selection().first(); // Первая выделенная область
        for (int i = range.top(); i <= range.bottom(); ++i)
        {
            QStringList rowContents;
            for (int j = range.left(); j <= range.right(); ++j)
            {
                rowContents << mVertexModel->index(i, j).data().toString(); // Собираем значения ячеек в строку
            }
            text += rowContents.join('\t'); // Табуляция между колонками
            text += '\n'; // Перенос строк между рядами
        }
        QApplication::clipboard()->setText(text); // Копируем в буфер обмена
    }
}

void QgsVertexEditor::closeEvent(QCloseEvent *event)
{
    QgsDockWidget::closeEvent(event); // Вызываем родительский обработчик

    emit editorClosed(); // Сигнализируем о закрытии редактора
}

CoordinateItemDelegate::CoordinateItemDelegate( const QgsCoordinateReferenceSystem &crs, QObject *parent )
    : QStyledItemDelegate(parent), mCrs(crs) {}

QString CoordinateItemDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    return locale.toString(value.toDouble(), 'f', displayDecimalPlaces());
}

QWidget *CoordinateItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const
{
    QLineEdit *lineEdit = new QLineEdit(parent);
    QgsDoubleValidator *validator = new QgsDoubleValidator(lineEdit);
    if (!index.data(MIN_RADIUS_ROLE).isNull())
    {
        validator->setBottom(index.data(MIN_RADIUS_ROLE).toDouble()); // Устанавливаем минимальное значение для радиуса (половина длины хорды)
    }
    lineEdit->setValidator(validator);
    return lineEdit;
}

void CoordinateItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const 
{
    QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
    if (lineEdit->hasAcceptableInput())
    {
        QStyledItemDelegate::setModelData(editor, model, index); // Применяем только если ввод валиден (не меньше minRadius)
    }
}

void CoordinateItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
    if (lineEdit && index.isValid())
    {
        lineEdit->setText(displayText(index.data().toDouble(), QLocale())); // Форматируем значение с нужной точностью при открытии редактора
    }
}

int CoordinateItemDelegate::displayDecimalPlaces() const
{
    return QgsCoordinateUtils::calculateCoordinatePrecisionForCrs( mCrs, QgsProject::instance() ); // Точность из CRS проекта
}
