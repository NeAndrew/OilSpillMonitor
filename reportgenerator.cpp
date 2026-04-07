#include "reportgenerator.h"

ReportGenerator::ReportGenerator(const ReportData& data, QObject *parent)
    : QObject(parent)
    , m_data(data)
{
}

double ReportGenerator::mmToPoints(double mm)
{
    return mm * 2.83465;
}

// Функция проверяет, достаточно ли места на странице, и создаёт новую при необходимости
static void ensureSpace(QPdfWriter* writer, double& yOffset, double requiredH, double pageH, double bottomMargin, double margin)
{
    if (writer && yOffset + requiredH > pageH - bottomMargin)
    {
        writer->newPage();
        yOffset = margin;
    }
}

bool ReportGenerator::generate()
{
    QPdfWriter pdfWriter(m_data.outputFilePath);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setResolution(300);
    pdfWriter.setTitle("Отчет " + m_data.appName);

    QPainter painter(&pdfWriter);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Вычисляем размеры страницы и отступы в пикселях
    QRect viewport = painter.viewport();
    double pageW = viewport.width();
    double pageH = viewport.height();
    double margin = 0.05 * qMin(pageW, pageH); // 5% от минимального размера
    double contentW = pageW - 2 * margin; // Ширина содержимого
    double bottomMargin = margin + 0.03 * pageH; // Дополнительный нижний отступ
    double yOffset = margin;

    // Последовательная отрисовка секций отчёта с проверкой флагов
    drawHeader(painter, yOffset, margin, contentW, pageH, bottomMargin, &pdfWriter);

    if (m_data.includeFlags.includePhotos)
    {
        drawTwoImagesRow(painter, yOffset, margin, contentW, pageH, bottomMargin, &pdfWriter);
    }

    if (m_data.includeFlags.includeDetectionOverlay)
    {
        drawDetectionOverlay(painter, yOffset, margin, contentW, pageH, bottomMargin, &pdfWriter);
    }

    if (m_data.includeFlags.includeSpillScreenshots)
    {
        drawSpillScreenshots(painter, yOffset, margin, contentW, pageH, bottomMargin, &pdfWriter);
    }

    drawDetailsTable(painter, yOffset, margin, contentW, pageH, bottomMargin, &pdfWriter);

    painter.end();
    return QFile::exists(m_data.outputFilePath);
}

void ReportGenerator::drawHeader(QPainter& painter, double& yOffset, double margin, double contentW, double pageH, double bottomMargin, QPdfWriter* writer)
{
    const double logoH = 0.04 * pageH;
    const double spacing = 0.01 * pageH;

    ensureSpace(writer, yOffset, logoH + spacing, pageH, bottomMargin, margin);

    if (!m_data.logoImagePath.isEmpty())
    {
        QImage logo(m_data.logoImagePath);

        if (!logo.isNull())
        {
            // Масштабируем логотип с сохранением пропорций
            double logoW = logoH * static_cast<double>(logo.width()) / logo.height();
            QImage scaled = logo.scaled(
                        static_cast<int>(logoW),
                        static_cast<int>(logoH),
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation
                        );

            QRectF logoRect(margin, yOffset, scaled.width(), scaled.height());
            painter.drawImage(logoRect, scaled);

            // Рисуем название приложения справа от логотипа
            painter.setFont(QFont("Arial", 12, QFont::Bold));
            QRectF titleRect(
                        logoRect.right() + spacing,
                        yOffset,
                        contentW - logoRect.width() - spacing,
                        logoH
                        );
            painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_data.appName);
            yOffset += logoRect.height() + spacing;
        }
        else
        {
            // Если логотип не загрузился, то рисуем только текст
            painter.setFont(QFont("Arial", 14, QFont::Bold));
            painter.drawText(
                        QRectF(margin, yOffset, contentW, logoH),
                        Qt::AlignLeft | Qt::AlignVCenter,
                        m_data.appName
                        );
            yOffset += logoH + spacing;
        }
    }
    else
    {
        // Если логотип не указан, рисуем только текст
        painter.setFont(QFont("Arial", 14, QFont::Bold));
        painter.drawText(
                    QRectF(margin, yOffset, contentW, logoH),
                    Qt::AlignLeft | Qt::AlignVCenter,
                    m_data.appName
                    );
        yOffset += logoH + spacing;
    }

    // Разделительная линия под шапкой
    painter.setPen(QPen(QColor(100, 100, 100), 0.5));
    painter.drawLine(QLineF(margin, yOffset, margin + contentW, yOffset));
    yOffset += spacing * 1.5;
}

// Отрисовка основных изображений (vv и rgb) в один ряд
void ReportGenerator::drawTwoImagesRow(QPainter& painter, double& yOffset,  double margin, double contentW, double pageH, double bottomMargin, QPdfWriter* writer)
{
    if (m_data.sentinelImageOriginalPath.isEmpty() && m_data.sentinelImagePolyPath.isEmpty())
    {
        return;
    }

    // Заголовок секции
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    double titleH = 0.02 * pageH;
    ensureSpace(writer, yOffset, titleH, pageH, bottomMargin, margin);
    painter.drawText(
                QRectF(margin, yOffset, contentW, titleH),
                Qt::AlignLeft | Qt::AlignVCenter,
                "Спутниковый снимок Sentinel-1"
                );
    yOffset += titleH * 1.1;

    // Расчёт слотов для двух изображений с учётом промежутков
    const int columns = 2;
    const double gap = 0.015 * contentW; // 1.5% от ширины содержимого
    double slotWidth = (contentW - gap * (columns - 1)) / columns;
    double rowH = 0;

    // Вычисляем высоту строки на основе обоих изображений для сохранения пропорций
    QStringList paths = { m_data.sentinelImageOriginalPath, m_data.sentinelImagePolyPath };
    for (const QString& path : paths) {
        if (!path.isEmpty() && QFile::exists(path))
        {
            QImage img(path);
            if (!img.isNull())
            {
                rowH = qMax(rowH, slotWidth * img.height() / static_cast<double>(img.width()));
            }
        }
    }

    if (rowH == 0)
    {
        return;
    }

    ensureSpace(writer, yOffset, rowH, pageH, bottomMargin, margin);

    // Отрисовка изображений
    double x = margin;
    for (const QString& path : paths)
    {
        if (path.isEmpty() || !QFile::exists(path))
        {
            x += slotWidth + gap;
            continue;
        }

        QImage img(path);
        if (img.isNull())
        {
            x += slotWidth + gap;
            continue;
        }

        QImage scaled = img.scaled(slotWidth, rowH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawImage(QRectF(x, yOffset, scaled.width(), scaled.height()), scaled);
        x += slotWidth + gap;
    }

    yOffset += rowH + 0.015 * pageH;
}

// Функция отрисовка изображений прогноза разлива (spill_0/24/48/72)
void ReportGenerator::drawSpillScreenshots(QPainter& painter, double& yOffset, double& margin, double& contentW, double& pageH, double& bottomMargin, QPdfWriter* writer)
{
    // Используем пути к изображениям из ReportData
    QStringList imagePaths = m_data.spillForecastImages;

    // Фильтруем пустые пути
    QStringList validPaths;
    for (const QString& path : imagePaths)
    {
        if (!path.isEmpty() && QFile::exists(path))
        {
            validPaths << path;
        }
    }

    if (validPaths.isEmpty())
    {
        return;
    }

    // Заголовок секции
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    double titleH = 0.02 * pageH;
    ensureSpace(writer, yOffset, titleH, pageH, bottomMargin, margin);

    painter.drawText(QRectF(margin, yOffset, contentW, titleH), Qt::AlignLeft | Qt::AlignVCenter, "Прогноз распространения через: 0 - 24 - 48 - 72 ч");
    yOffset += titleH * 1.1;

    // Расчёт сетки изображений (2 колонки, 2 строки)
    const int columns = 2;
    const int rows = 2;
    const double gap = 0.015 * contentW;
    double slotWidth = (contentW - gap * (columns - 1)) / columns;
    double rowH = 0;

    // Вычисляем высоту строки на основе всех изображений
    for (const QString& path : std::as_const(validPaths))
    {
        QImage img(path);
        if (!img.isNull())
        {
            rowH = qMax(rowH, slotWidth * img.height() / static_cast<double>(img.width()));
        }
    }

    ensureSpace(writer, yOffset, rowH * rows + gap * (rows - 1), pageH, bottomMargin, margin);

    // Отрисовка изображений прогноза в сетке 2x2
    int imageIndex = 0;
    for (int row = 0; row < rows && imageIndex < validPaths.size(); ++row)
    {
        double x = margin;
        for (int col = 0; col < columns && imageIndex < validPaths.size(); ++col)
        {
            const QString& path = validPaths[imageIndex];
            QImage img(path);
            if (!img.isNull())
            {
                QImage scaled = img.scaled(
                            slotWidth,
                            rowH,
                            Qt::KeepAspectRatio,
                            Qt::SmoothTransformation
                            );
                painter.drawImage(
                            QRectF(x, yOffset + row * (rowH + gap), scaled.width(), scaled.height()),
                            scaled
                            );
            }
            x += slotWidth + gap;
            imageIndex++;
        }
    }

    yOffset += rowH * rows + gap * (rows - 1) + 0.015 * pageH;
}

void ReportGenerator::drawDetailsTable(QPainter& painter, double& yOffset, double margin, double contentW, double pageH, double bottomMargin, QPdfWriter* writer)
{
    QStringList rows;
    const auto& d = m_data.details;
    const auto& f = m_data.includeFlags;

    // Формируем строки таблицы на основе флагов
    if (f.includeTimestamp && d.timestamp.isValid())
    {
        rows << QString("Время: %1").arg(d.timestamp.toString("dd.MM.yyyy HH:mm:ss"));
    }
    if (f.includePerimeter)
    {
        rows << QString("Периметр: %1 м").arg(d.perimeter, 0, 'f', 2);
    }
    if (f.includeArea)
    {
        rows << QString("Площадь: %1 м²").arg(d.area, 0, 'f', 2);
    }
    if (f.includeImageId && !d.imageId.isEmpty())
    {
        rows << QString("ID снимка: %1").arg(d.imageId);
    }
    if (f.includeCoordinates && !d.coordinates.isEmpty())
    {
        rows << QString("Координаты: %1").arg(d.coordinates);
    }

    if (rows.isEmpty())
    {
        return;
    }

    // Заголовок секции
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    double titleH = 0.025 * pageH;
    ensureSpace(writer, yOffset, titleH, pageH, bottomMargin, margin);

    painter.drawText(QRectF(margin, yOffset, contentW, titleH), Qt::AlignLeft | Qt::AlignVCenter, "Детали пятна");
    yOffset += titleH * 1.1;

    // Отрисовка строк с деталями
    painter.setFont(QFont("Arial", 9));
    for (const QString& row : std::as_const(rows))
    {
        ensureSpace(writer, yOffset, 0.025 * pageH, pageH, bottomMargin, margin);

        painter.drawText(QRectF(margin, yOffset, contentW, 0.05 * pageH), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, row);
        yOffset += 0.02 * pageH;
    }
}

void ReportGenerator::drawDetectionOverlay(QPainter& painter, double& yOffset, double& margin, double& contentW, double& pageH, double& bottomMargin, QPdfWriter* writer)
{
    if (m_data.detectionOverlayImagePath.isEmpty() || !QFile::exists(m_data.detectionOverlayImagePath))
    {
        return;
    }

    // Заголовок секции
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    double titleH = 0.02 * pageH;
    ensureSpace(writer, yOffset, titleH, pageH, bottomMargin, margin);
    painter.drawText(QRectF(margin, yOffset, contentW, titleH), Qt::AlignLeft | Qt::AlignVCenter, "Обнаруженное пятно");
    yOffset += titleH * 1.1;

    // Расчет размеров изображения
    const double maxWidth = contentW;
    const double maxHeight = 0.5 * pageH; // Увеличиваем высоту для лучшей видимости

    QImage img(m_data.detectionOverlayImagePath);
    if (img.isNull())
    {
        return;
    }

    // Масштабируем изображение с сохранением пропорций
    QImage scaled = img.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    ensureSpace(writer, yOffset, scaled.height(), pageH, bottomMargin, margin);

    // Центрируем изображение
    double x = margin + (contentW - scaled.width()) / 2.0;

    painter.drawImage(QRectF(x, yOffset, scaled.width(), scaled.height()), scaled);

    yOffset += scaled.height() + 0.01 * pageH;
}
