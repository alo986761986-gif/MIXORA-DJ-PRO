#pragma once
#include "Wav.h"
#include <QString>
#include <QWidget>
namespace mixora {
// Uses Qt multimedia backend. Supported codecs depend on installed Windows/Qt backend.
WavFile decodeQtFile(const QString& filename);
}
