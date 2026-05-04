#pragma once

#include <QImage>
#include <QPoint>
#include <imgproc/frame.h>
#include <segcore/display.h>
#include <segcore/types.h>

namespace qt_adapter {

inline QPoint ToQPoint(segcore::Point2D p)
{
  return {p.x, p.y};
}

inline segcore::Point2D FromQPoint(QPoint p)
{
  return {p.x(), p.y()};
}

// Zero-copy: QImage borrows Frame's data pointer — Frame must outlive QImage
inline QImage FrameToQImage(const polyseg::Frame<segcore::Rgb24>& f)
{
  return QImage(reinterpret_cast<const uchar*>(f.data()), f.width(), f.height(), f.width() * 3,
                QImage::Format_RGB888);
}

// Copies pixels from QImage into a new Frame<Rgb24>
polyseg::Frame<segcore::Rgb24> QImageToFrame(const QImage& img);

}  // namespace qt_adapter
