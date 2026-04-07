#include "SarProcess.h"


SarProcess::SarProcess(QObject *parent) : QObject(parent)
{
    GDALAllRegister(); 
    qRegisterMetaType<SarProcess::Mode>("Mode");
    qRegisterMetaType<SARProcessingResult>("SARProcessingResult");
}

void SarProcess::process(const QString &inputPath, Mode mode)
{
    SARProcessingResult result;

    if (mode == ModePreprocessed) 
    {
        result = processPreprocessedTiff(inputPath);
    } 
    else 
    {
        result = processRawSAFE(inputPath);
    }

    // Отправляем результат в главный поток 
    emit finished(result);
}

SARProcessingResult SarProcess::processPreprocessedTiff(const QString &tiffPath)
{
    SARProcessingResult result;
    result.success = true;
    result.vvOut = tiffPath;

    GDALDatasetH hDataset = GDALOpen(tiffPath.toUtf8().constData(), GA_ReadOnly);
    if (!hDataset) 
    {
        result.error = tr("GDAL: не удалось открыть %1").arg(tiffPath);
        result.success = false;
        return result;
    }

    int width = GDALGetRasterXSize(hDataset);
    int height = GDALGetRasterYSize(hDataset);
    double geoTransform[6];

    if (GDALGetGeoTransform(hDataset, geoTransform) != CE_None) 
    {
        GDALClose(hDataset);
        result.error = tr("GDAL: не удалось прочитать геопривязку");
        result.success = false;
        return result;
    }

    result.geotransform = {geoTransform[0], geoTransform[1], geoTransform[2], geoTransform[3], geoTransform[4], geoTransform[5]};
    result.geotiffSize = QSize(width, height);
    result.imageDataTime = parseDateFromFilename(tiffPath);
    GDALClose(hDataset);

    QFileInfo fi(tiffPath);
    QString tempDir = m_sourceDir + "Data/temp/";
    QDir().mkpath(tempDir);

    result.outputPathVv = tempDir + fi.baseName() + "_vv.png";
    if (!convertGeoTiffToPNG(tiffPath, result.outputPathVv)) 
    {
        result.error = tr("Не удалось создать PNG-превью");
        result.success = false;
        return result;
    }

    return result;
}

SARProcessingResult SarProcess::processRawSAFE(const QString &safePath)
{
    SARProcessingResult result;

    if (m_snapPath.isEmpty()) 
    {
        result.error = tr("Путь к SNAP не задан");
        return result;
    }

    QString gptPath = m_snapPath + "/gpt";
    QFileInfo inputInfo(safePath);
    QString basename = inputInfo.baseName();
    QString tempDir = m_sourceDir + "Data/temp/";
    QDir().mkpath(tempDir);

    auto makePath = [&tempDir, &basename](const QString &suffix) 
    {
        return tempDir + basename + suffix;
    };

    // Промежуточные файлы
    QString mlOut = makePath("_ml.dim");
    QString speckleOut = makePath("_speckle.dim");
    QString calOut = makePath("_cal.dim");
    QString orbitOut = makePath("_orbit.dim");
    QString geoBigOut = makePath("_geo.tif");
    QString ratioOut = makePath("_ratio.tif");
    QString vvOut = makePath("_vv.tif");
    QString vhOut = makePath("_vh.tif");
    QString rgbTemp = makePath("_rgb_tmp.tif");
    QString finalOut = makePath("_rgb_cog.tif");

    // Вспомогательная лямбда для запуска команд
    auto run = [this](const QString &exe, const QStringList &args) -> bool 
    {
        QProcess proc;
        proc.start(exe, args);
        return proc.waitForFinished(-1) && proc.exitCode() == 0;
    };

    // Этапы обработки SNAP
    if (!run(gptPath, {"Multilook", "-PnRgLooks=10", "-PnAzLooks=10", safePath, "-t", mlOut})) 
    {
        result.error = "Multilook failed"; 
        return result; 
    }
    if (!run(gptPath, {"Speckle-Filter", "-Pfilter=Refined Lee", "-PfilterSizeX=3", "-PfilterSizeY=3",
                       "-PestimateENL=true", "-Penl=1.0", mlOut, "-t", speckleOut}))
    {
        result.error = "Speckle-Filter failed"; 
        return result; 
    }
    if (!run(gptPath, {"Calibration", "-PoutputSigmaBand=true", "-PoutputImageScaleInDb=false",
                       speckleOut, "-t", calOut})) 
    {
        result.error = "Calibration failed";
        return result; 
    }
    if (!run(gptPath, {"Apply-Orbit-File", calOut, "-t", orbitOut})) 
    {
        result.error = "Apply-Orbit-File failed"; 
        return result; 
    }
    if (!run(gptPath, {"Terrain-Correction", "-PdemName=GETASSE30", "-PpixelSpacingInMeter=100",
                       "-PnodataValueAtSea=false", "-PsourceBands=Sigma0_VV,Sigma0_VH",
                       "-PmapProjection=" + m_targetCrs, "-t", geoBigOut, orbitOut})) 
    {
        result.error = "Terrain-Correction failed"; 
        return result; 
    }

    // Этапы обработки GDAL
    if (!run("gdal_calc.py", {"-A", geoBigOut, "--A_band=1", "-B", geoBigOut, "--B_band=2",
                             "--calc=where(B!=0, A/B, 0)", "--outfile=" + ratioOut,
                             "--type=Float32", "--NoDataValue=0"})) 
    {
        result.error = "gdal_calc.py failed"; 
        return result; 
    }
    if (!run("gdal_translate", {"-b", "1", "-a_nodata", "0", geoBigOut, vvOut})) 
    {
        result.error = "gdal_translate VV failed"; 
        return result; 
    }
    if (!run("gdal_translate", {"-b", "2", "-a_nodata", "0", geoBigOut, vhOut})) 
    {
        result.error = "gdal_translate VH failed"; 
        return result; 
    }
    if (!run("gdal_merge.py", {"-separate", "-o", rgbTemp, "-co", "TILED=YES", "-co", "COMPRESS=DEFLATE",
                               vvOut, vhOut, ratioOut})) 
    {
        result.error = "gdal_merge.py failed"; 
        return result; 
    }
    if (!run("gdal_translate", {rgbTemp, finalOut, "-a_nodata", "0",
                               "-co", "TILED=YES", "-co", "COMPRESS=DEFLATE",
                               "-co", "BLOCKXSIZE=512", "-co", "BLOCKYSIZE=512"})) 
    {
        result.error = "gdal_translate COG failed"; 
        return result; 
    }
    if (!run("gdaladdo", {finalOut, "--config", "COMPRESS_OVERVIEW", "DEFLATE",
                          "-r", "average", "2", "4", "8", "16", "32"})) 
    {
        // Не критично, можно продолжить 
    }

    // Конвертация в PNG
    result.outputPathRgb = tempDir + basename + "_rgb.png";
    result.outputPathVv = tempDir + basename + "_vv.png";
    convertGeoTiffToPNG(finalOut, result.outputPathRgb);
    convertGeoTiffToPNG(vvOut, result.outputPathVv);

    // Чтение метаданных
    GDALDatasetH hDataset = GDALOpen(vvOut.toUtf8().constData(), GA_ReadOnly);
    if (hDataset) 
    {
        int width = GDALGetRasterXSize(hDataset);
        int height = GDALGetRasterYSize(hDataset);
        double geoTransform[6];
        if (GDALGetGeoTransform(hDataset, geoTransform) == CE_None) 
        {
            result.geotransform = {geoTransform[0], geoTransform[1], geoTransform[2], geoTransform[3], geoTransform[4], geoTransform[5]};
            result.geotiffSize = QSize(width, height);
        }
        GDALClose(hDataset);
    }

    // Заполнение результатов
    result.success = true;
    result.finalOut = finalOut;
    result.vvOut = vvOut;
    result.basename = basename;
    result.imageDataTime = parseDateFromFilename(safePath);

    // Очистка промежуточных файлов
    for (const QString &f : {mlOut, speckleOut, calOut, orbitOut, geoBigOut, ratioOut, rgbTemp, vhOut}) 
    {
        QFile::remove(f);
        QFile::remove(f + ".aux.xml");
    }
    QDir d(tempDir);
    for (const QString &folder : d.entryList({"*.data"}, QDir::Dirs | QDir::NoDotAndDotDot))
    {
        QDir(d.filePath(folder)).removeRecursively();
    }

    return result;
}

// Функция преобразования GeoTIFF в PNG
bool SarProcess::convertGeoTiffToPNG(const QString &inputPath, const QString &outputPath, float gamma)
{
    GDALAllRegister();
    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(inputPath.toUtf8().constData(), GA_ReadOnly));
    if (!ds) 
    {
        return false;
    }

    const int width = ds->GetRasterXSize();
    const int height = ds->GetRasterYSize();
    const int bandCount = ds->GetRasterCount();

    if (bandCount != 1 && bandCount != 3) 
    {
        GDALClose(ds);
        return false;
    }

    // Чтение каналов
    std::vector<std::vector<float>> bands(bandCount);
    for (int b = 0; b < bandCount; ++b) 
    {
        bands[b].resize(width * height);
        GDALRasterBand* band = ds->GetRasterBand(b + 1);
        band->RasterIO(GF_Read, 0, 0, width, height, bands[b].data(), width, height, GDT_Float32, 0, 0);
    }
    GDALClose(ds);

    // Растяжение 2–98%
    struct Stretch { float p2, p98; };
    std::vector<Stretch> stretches(bandCount);

    for (int b = 0; b < bandCount; ++b) 
    {
        std::vector<float> valid;
        valid.reserve(width * height);
        for (float v : bands[b])
        {
            if (v != 0.0f && std::isfinite(v)) valid.push_back(v);
        }

        if (valid.empty()) 
        {
            return false;
        }
        std::sort(valid.begin(), valid.end());

        auto percentile = [&](double p)
        {
            size_t idx = static_cast<size_t>(p * (valid.size() - 1));
            return valid[idx];
        };
        stretches[b].p2 = percentile(0.02);
        stretches[b].p98 = percentile(0.98);
    }

    // Создание QImage
    QImage img(width, height, bandCount == 1 ? QImage::Format_Grayscale8 : QImage::Format_RGB888);

    for (int y = 0; y < height; ++y) 
    {
        uchar* row = img.scanLine(y);
        for (int x = 0; x < width; ++x) 
        {
            int idx = y * width + x;
            if (bandCount == 1) 
            {
                float v = bands[0][idx];
                if (v == 0.0f || !std::isfinite(v)) 
                { 
                    row[x] = 0; 
                    continue; 
                }
                float range = stretches[0].p98 - stretches[0].p2;
                if (range < 1e-6f) 
                { 
                    row[x] = 128; 
                    continue; 
                } // защита от деления на 0
                float norm = std::clamp((v - stretches[0].p2) / range, 0.0f, 1.0f);
                row[x] = static_cast<uchar>(std::pow(norm, 1.0f / gamma) * 255.0f);
            } 
            else 
            {
                for (int c = 0; c < 3; ++c) 
                {
                    float v = bands[c][idx];
                    if (v == 0.0f || !std::isfinite(v)) 
                    {
                        row[x * 3 + c] = 0; 
                        continue; 
                    }
                    float range = stretches[c].p98 - stretches[c].p2;
                    if (range < 1e-6f) 
                    { 
                        row[x * 3 + c] = 128;
                        continue; 
                    }
                    float norm = std::clamp((v - stretches[c].p2) / range, 0.0f, 1.0f);
                    row[x * 3 + c] = static_cast<uchar>(std::pow(norm, 1.0f / gamma) * 255.0f);
                }
            }
        }
    }

    return img.save(outputPath);
}

QDateTime SarProcess::parseDateFromFilename(const QString &filename)
{
    QFileInfo fi(filename);
    for (const QString &part : fi.baseName().split('_')) 
    {
        if (part.length() >= 15 && part.at(8) == 'T') 
        {
            QDateTime dt = QDateTime::fromString(part.left(15), "yyyyMMddThhmmss");
            if (dt.isValid()) 
            {
                return dt;
            }
        }
    }
    return QDateTime::currentDateTime();
}

bool SarProcess::runGpt(const QString &gptPath, const QStringList &args)
{
    QProcess proc;
    proc.start(gptPath, args);
    return proc.waitForFinished(-1) && proc.exitCode() == 0;
}
