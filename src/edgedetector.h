#ifndef EDGEDETECTOR_H
#define EDGEDETECTOR_H

#include <QImage>
#include <QPoint>
#include <cstdint>
#include <vector>

class EdgeDetector
{
 public:
  struct Result
  {
    QImage overlay;
    std::vector<uint8_t> edge_map;
    int width = 0;
    int height = 0;
    bool isValid() const { return !edge_map.empty(); }
  };

  static Result Detect(const QImage& image);
  static QPoint SnapToEdge(const QPoint& img_pos, const Result& edges, int radius = 15);
};

#endif  // EDGEDETECTOR_H
