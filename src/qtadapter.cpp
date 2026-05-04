#include "qtadapter.h"

namespace qt_adapter {

polyseg::Frame<segcore::Rgb24> QImageToFrame(const QImage& img)
{
  QImage rgb = img.convertToFormat(QImage::Format_RGB888);
  int w = rgb.width();
  int h = rgb.height();
  polyseg::Frame<segcore::Rgb24> frame(static_cast<uint16_t>(w), static_cast<uint16_t>(h));
  segcore::Rgb24* dst = frame.data();
  for (int y = 0; y < h; ++y)
  {
    const uchar* row = rgb.scanLine(y);
    for (int x = 0; x < w; ++x)
    {
      dst[y * w + x] = {row[x * 3], row[x * 3 + 1], row[x * 3 + 2]};
    }
  }
  return frame;
}

}  // namespace qt_adapter
