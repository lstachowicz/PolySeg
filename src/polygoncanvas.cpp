#include "polygoncanvas.h"

#include <QFile>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QTextStream>

#include <iostream>
#include <limits>

inline int Distance(const QPoint& p1, const QPoint& p2)
{
  auto result = sqrt(pow(p1.x() - p2.x(), 2) + pow(p1.y() - p2.y(), 2));
  return result;
}

// Oblicza odległość punktu od segmentu linii
inline float DistanceFromPointToSegment(const QPoint& point, const QPoint& lineStart,
                                        const QPoint& lineEnd)
{
  // Wektor od lineStart do lineEnd
  float dx = lineEnd.x() - lineStart.x();
  float dy = lineEnd.y() - lineStart.y();

  // Jeśli segment ma zerową długość, zwróć odległość do punktu
  float segmentLengthSquared = dx * dx + dy * dy;
  if (segmentLengthSquared == 0.0f)
  {
    return Distance(point, lineStart);
  }

  // Parametr t określa gdzie projekcja punktu pada na linię (0 = start, 1 = end)
  float t =
      ((point.x() - lineStart.x()) * dx + (point.y() - lineStart.y()) * dy) / segmentLengthSquared;

  // Ogranicz t do zakresu [0, 1] - projekcja musi być na segmencie
  t = qBound(0.0f, t, 1.0f);

  // Znajdź najbliższy punkt na segmencie
  QPointF closestPoint(lineStart.x() + t * dx, lineStart.y() + t * dy);

  // Oblicz odległość od punktu do najbliższego punktu na segmencie
  float distX = point.x() - closestPoint.x();
  float distY = point.y() - closestPoint.y();

  return sqrt(distX * distX + distY * distY);
}

PolygonCanvas::PolygonCanvas(QWidget* parent) : QLabel(parent)
{
  // Initialize with default color for first polygon
  current_polygon_.class_id = 0;
  current_polygon_.color = Qt::red;

  // Enable keyboard focus to receive key events
  setFocusPolicy(Qt::StrongFocus);
}

void PolygonCanvas::Increase()
{
  scalar_ = scalar_ + 1.0;

  QSize size = pixmap().size();
  size.setWidth(static_cast<int>(size.width() * scalar_));
  size.setHeight(static_cast<int>(size.height() * scalar_));
  setFixedSize(size);
}

void PolygonCanvas::Decrease()
{
  auto new_scalar_ = scalar_ - 1.0;
  if (new_scalar_ > 0)
  {
    scalar_ = new_scalar_;
  }

  QSize size = pixmap().size();
  size.setWidth(static_cast<int>(size.width() * scalar_));
  size.setHeight(static_cast<int>(size.height() * scalar_));
  setFixedSize(size);
}

void PolygonCanvas::ResetZoom()
{
  scalar_ = 1.0;
  QSize size = pixmap().size();
  size.setWidth(static_cast<int>(size.width() * scalar_));
  size.setHeight(static_cast<int>(size.height() * scalar_));
  setFixedSize(size);
  std::cout << "Zoom reset to 100%" << std::endl;
}

void PolygonCanvas::StartNewPolygon(int class_id, QColor color)
{
  current_polygon_.class_id = class_id;
  current_polygon_.color = color;
  current_polygon_.points.clear();
  current_polygon_.is_selected = false;
  std::cout << "Started new polygon with class_id: " << class_id << std::endl;
}

void PolygonCanvas::FinishCurrentPolygon()
{
  if (current_polygon_.points.size() >= 3)
  {
    polygons_.push_back(current_polygon_);
    std::cout << "Finished polygon with " << current_polygon_.points.size() << " points"
              << std::endl;
    current_polygon_.points.clear();
    repaint();
  }
  else
  {
    std::cout << "Cannot finish polygon: need at least 3 points" << std::endl;
  }
}

void PolygonCanvas::ClearCurrentPolygon()
{
  current_polygon_.points.clear();
  repaint();
  std::cout << "Cleared current polygon" << std::endl;
}

void PolygonCanvas::mouseMoveEvent(QMouseEvent* ev)
{
  auto pos = ev->pos() / scalar_;
  active_point_pos_ = pos;
}

void PolygonCanvas::mousePressEvent(QMouseEvent* ev)
{
  QPoint pos = ev->pos() / scalar_;

  // Check if editing current polygon being drawn
  for (const auto& point : current_polygon_.points)
  {
    if (IsPointNearPosition(point, pos, POINT_SELECT_TOLERANCE))
    {
      active_point_ = point;
      active_point_pos_ = point;
      return;
    }
  }

  // Check if editing selected polygon
  if (selected_polygon_index_ >= 0 && selected_polygon_index_ < polygons_.size())
  {
    for (const auto& point : polygons_[selected_polygon_index_].points)
    {
      if (IsPointNearPosition(point, pos, POINT_SELECT_TOLERANCE))
      {
        active_point_ = point;
        active_point_pos_ = point;
        return;
      }
    }
  }
}

void PolygonCanvas::mouseReleaseEvent(QMouseEvent* ev)
{
  QPoint pos = ev->pos() / scalar_;

  if (!active_point_.isNull())
  {
    HandlePointDrag(pos);
  }
  else
  {
    // If currently drawing, add point to current polygon
    if (!current_polygon_.points.isEmpty())
    {
      HandlePointInsertion(pos);
    }
    else
    {
      // Not drawing - try to select a polygon
      SelectPolygon(pos);
    }
  }

  active_point_ = QPoint();
  active_point_pos_ = QPoint();
  repaint();
}

void PolygonCanvas::keyPressEvent(QKeyEvent* ev)
{
  if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter)
  {
    FinishCurrentPolygon();
  }
  else if (ev->key() == Qt::Key_Escape)
  {
    if (!current_polygon_.points.isEmpty())
    {
      ClearCurrentPolygon();
    }
    else
    {
      DeselectAll();
    }
  }
  else if (ev->key() == Qt::Key_Delete)
  {
    DeleteSelectedPolygon();
  }
  else
  {
    QLabel::keyPressEvent(ev);
  }
}

void PolygonCanvas::paintEvent(QPaintEvent*)
{
  QPainter painter(this);

  DrawImage(painter);

  // Draw all completed polygons
  for (const auto& polygon : polygons_)
  {
    if (polygon.points.size() < 2)
      continue;

    // Visual feedback for selection
    QColor drawColor = polygon.color;
    int lineWidth = LINE_WIDTH;

    if (polygon.is_selected)
    {
      lineWidth = 3;
      drawColor = drawColor.lighter(120);  // Brighter color
    }
    else
    {
      drawColor.setAlpha(180);  // Semi-transparent for unselected
    }

    QPen pen(drawColor, lineWidth);
    painter.setPen(pen);

    // Draw points
    for (const auto& point : polygon.points)
    {
      painter.drawEllipse(point * scalar_, POINT_DRAW_SIZE, POINT_DRAW_SIZE);
    }

    // Draw segments
    for (int i = 1; i < polygon.points.size(); ++i)
    {
      painter.drawLine(polygon.points[i - 1] * scalar_, polygon.points[i] * scalar_);
    }

    // Draw closing segment
    painter.setPen(QPen(drawColor.darker(120), lineWidth));
    painter.drawLine(polygon.points[0] * scalar_,
                     polygon.points[polygon.points.size() - 1] * scalar_);
  }

  // Draw current polygon being edited
  DrawPoints(painter);
  DrawSegments(painter);
  DrawClosingSegment(painter);
}

QSize PolygonCanvas::GetOriginalImageSize() const
{
  if (!pixmap().isNull())
  {
    return pixmap().size();
  }
  return QSize(0, 0);
}

void PolygonCanvas::ExportYolo(const QString& filename, int class_id)
{
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    std::cerr << "Cannot open file for writing: " << filename.toStdString() << std::endl;
    return;
  }

  QTextStream out(&file);

  QSize img_size = GetOriginalImageSize();
  if (img_size.width() == 0 || img_size.height() == 0)
  {
    std::cerr << "Invalid image size" << std::endl;
    return;
  }

  float img_width = static_cast<float>(img_size.width());
  float img_height = static_cast<float>(img_size.height());

  // Export all polygons
  for (const auto& polygon : polygons_)
  {
    out << polygon.class_id;

    for (const auto& point : polygon.points)
    {
      float normalized_x = qBound(0.0f, point.x() / img_width, 1.0f);
      float normalized_y = qBound(0.0f, point.y() / img_height, 1.0f);
      out << " " << normalized_x << " " << normalized_y;
    }

    out << "\n";
  }

  file.close();

  std::cout << "Annotations exported to: " << filename.toStdString() << std::endl;
  std::cout << "Polygons: " << polygons_.size() << std::endl;
}

void PolygonCanvas::LoadYoloAnnotations(const QString& filepath,
                                        const QVector<QColor>& class_colors)
{
  QFile file(filepath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    std::cerr << "Cannot open file for reading: " << filepath.toStdString() << std::endl;
    return;
  }

  QSize img_size = GetOriginalImageSize();
  if (img_size.width() == 0 || img_size.height() == 0)
  {
    std::cerr << "Invalid image size" << std::endl;
    return;
  }

  float img_width = static_cast<float>(img_size.width());
  float img_height = static_cast<float>(img_size.height());

  polygons_.clear();
  QTextStream in(&file);

  while (!in.atEnd())
  {
    QString line = in.readLine().trimmed();
    if (line.isEmpty())
    {
      continue;
    }

    QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 7)  // class_id + at least 3 points (6 coordinates)
    {
      std::cerr << "Invalid line: " << line.toStdString() << std::endl;
      continue;
    }

    bool ok;
    int class_id = parts[0].toInt(&ok);
    if (!ok)
    {
      std::cerr << "Invalid class_id: " << parts[0].toStdString() << std::endl;
      continue;
    }

    Polygon polygon;
    polygon.class_id = class_id;
    polygon.color = (class_id < class_colors.size()) ? class_colors[class_id] : Qt::red;
    polygon.is_selected = false;

    // Parse coordinate pairs
    for (int i = 1; i < parts.size() - 1; i += 2)
    {
      float x_norm = parts[i].toFloat(&ok);
      if (!ok)
        continue;

      float y_norm = parts[i + 1].toFloat(&ok);
      if (!ok)
        continue;

      // Denormalize coordinates
      int x_pixel = static_cast<int>(x_norm * img_width);
      int y_pixel = static_cast<int>(y_norm * img_height);

      polygon.points.append(QPoint(x_pixel, y_pixel));
    }

    if (polygon.points.size() >= 3)
    {
      polygons_.append(polygon);
    }
  }

  file.close();
  update();

  std::cout << "Loaded " << polygons_.size() << " polygons from: " << filepath.toStdString()
            << std::endl;
}

void PolygonCanvas::ClearAllPolygons()
{
  polygons_.clear();
  current_polygon_.points.clear();
  selected_polygon_index_ = -1;
  update();
}

void PolygonCanvas::AddPolygonFromPlugin(const QVector<QPoint>& points, int class_id,
                                         const QColor& color)
{
  if (points.size() < 3)
  {
    return;  // Invalid polygon
  }

  Polygon polygon;
  polygon.class_id = class_id;
  polygon.points = points;
  polygon.color = color;
  polygon.is_selected = false;

  polygons_.append(polygon);
  update();

  std::cout << "Added plugin polygon with " << points.size() << " points (class_id=" << class_id
            << ")" << std::endl;
}

void PolygonCanvas::SelectPolygon(const QPoint& pos)
{
  // Check from last to first (top to bottom in Z-order)
  for (int i = polygons_.size() - 1; i >= 0; --i)
  {
    const auto& polygon = polygons_[i];
    if (polygon.points.size() < 3)
      continue;

    // Point-in-polygon algorithm (ray casting)
    bool inside = false;
    int j = polygon.points.size() - 1;

    for (int k = 0; k < polygon.points.size(); ++k)
    {
      const QPoint& vi = polygon.points[k];
      const QPoint& vj = polygon.points[j];

      if (((vi.y() > pos.y()) != (vj.y() > pos.y())) &&
          (pos.x() < (vj.x() - vi.x()) * (pos.y() - vi.y()) / (vj.y() - vi.y()) + vi.x()))
      {
        inside = !inside;
      }
      j = k;
    }

    if (inside)
    {
      // Deselect all first
      for (auto& p : polygons_)
      {
        p.is_selected = false;
      }

      // Select this polygon
      polygons_[i].is_selected = true;
      selected_polygon_index_ = i;
      update();
      std::cout << "Selected polygon " << i << " (class_id=" << polygons_[i].class_id << ")"
                << std::endl;
      return;
    }
  }

  // No polygon selected - deselect all
  DeselectAll();
}

void PolygonCanvas::DeselectAll()
{
  for (auto& polygon : polygons_)
  {
    polygon.is_selected = false;
  }
  selected_polygon_index_ = -1;
  update();
  std::cout << "Deselected all polygons" << std::endl;
}

void PolygonCanvas::DeleteSelectedPolygon()
{
  if (selected_polygon_index_ >= 0 && selected_polygon_index_ < polygons_.size())
  {
    std::cout << "Deleting polygon " << selected_polygon_index_ << std::endl;
    polygons_.removeAt(selected_polygon_index_);
    selected_polygon_index_ = -1;
    update();
  }
}

// Private helper methods

bool PolygonCanvas::IsPointNearPosition(const QPoint& point, const QPoint& position,
                                        int tolerance) const
{
  return qAbs(point.x() - position.x()) <= tolerance && qAbs(point.y() - position.y()) <= tolerance;
}

int PolygonCanvas::FindNearestSegmentIndex(const QPoint& position) const
{
  float min_distance = std::numeric_limits<float>::max();
  int insert_index = -1;

  for (int i = 0; i < current_polygon_.points.size(); ++i)
  {
    int next_i = (i + 1) % current_polygon_.points.size();
    float distance = DistanceFromPointToSegment(position, current_polygon_.points[i],
                                                current_polygon_.points[next_i]);

    if (distance < min_distance)
    {
      min_distance = distance;
      insert_index = next_i;
    }
  }

  return insert_index;
}

void PolygonCanvas::HandlePointDrag(const QPoint& position)
{
  // Try editing current polygon first
  for (int i = 0; i < current_polygon_.points.size(); ++i)
  {
    if (current_polygon_.points[i] == active_point_)
    {
      if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
      {
        current_polygon_.points.removeAt(i);
      }
      else
      {
        current_polygon_.points[i] = position;
      }
      return;
    }
  }

  // Try editing selected polygon
  if (selected_polygon_index_ >= 0 && selected_polygon_index_ < polygons_.size())
  {
    auto& polygon = polygons_[selected_polygon_index_];
    for (int i = 0; i < polygon.points.size(); ++i)
    {
      if (polygon.points[i] == active_point_)
      {
        if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
        {
          polygon.points.removeAt(i);
          std::cout << "Removed point from selected polygon" << std::endl;
        }
        else
        {
          polygon.points[i] = position;
        }
        return;
      }
    }
  }
}

void PolygonCanvas::HandlePointInsertion(const QPoint& position)
{
  bool ctrl_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier);

  // If Ctrl is pressed and we have a selected polygon, try to insert point
  if (ctrl_pressed && selected_polygon_index_ >= 0 && selected_polygon_index_ < polygons_.size())
  {
    auto& polygon = polygons_[selected_polygon_index_];
    if (polygon.points.size() > 1)
    {
      // Find nearest segment in selected polygon
      float min_distance = std::numeric_limits<float>::max();
      int insert_index = -1;

      for (int i = 0; i < polygon.points.size(); ++i)
      {
        int next_i = (i + 1) % polygon.points.size();
        float distance =
            DistanceFromPointToSegment(position, polygon.points[i], polygon.points[next_i]);

        if (distance < min_distance)
        {
          min_distance = distance;
          insert_index = next_i;
        }
      }

      if (insert_index != -1 && min_distance < 10.0f)
      {
        polygon.points.insert(polygon.points.begin() + insert_index, position);
        std::cout << "Inserted point at index: " << insert_index << " in selected polygon"
                  << std::endl;
        return;
      }
    }
  }

  // Otherwise add to current polygon
  if (ctrl_pressed && current_polygon_.points.size() > 1)
  {
    int insert_index = FindNearestSegmentIndex(position);
    if (insert_index != -1)
    {
      current_polygon_.points.insert(current_polygon_.points.begin() + insert_index, position);
      std::cout << "Inserted at index: " << insert_index << std::endl;
    }
  }
  else
  {
    current_polygon_.points.push_back(position);
  }
}

void PolygonCanvas::DrawImage(QPainter& painter)
{
  QPixmap pix = pixmap();
  if (!pix.isNull())
  {
    pix = pix.scaled(pix.width() * scalar_, pix.height() * scalar_);
    painter.drawPixmap(0, 0, pix);
  }
}

void PolygonCanvas::DrawPoints(QPainter& painter)
{
  QPen pen(current_polygon_.color, POINT_DRAW_SIZE);
  painter.setPen(pen);

  for (const auto& point : current_polygon_.points)
  {
    QPoint draw_pos = (!active_point_.isNull() && point == active_point_)
                          ? active_point_pos_ * scalar_
                          : point * scalar_;
    painter.drawPoint(draw_pos);
  }
}

void PolygonCanvas::DrawSegments(QPainter& painter)
{
  if (current_polygon_.points.size() < 2)
    return;

  QPen pen(current_polygon_.color, LINE_WIDTH);
  painter.setPen(pen);

  for (int i = 1; i < current_polygon_.points.size(); ++i)
  {
    QPoint prev = current_polygon_.points[i - 1];
    QPoint curr = current_polygon_.points[i];

    if (curr == active_point_)
      curr = active_point_pos_;
    if (prev == active_point_)
      prev = active_point_pos_;

    painter.drawLine(prev * scalar_, curr * scalar_);
  }
}

void PolygonCanvas::DrawClosingSegment(QPainter& painter)
{
  if (current_polygon_.points.size() < 2)
    return;

  QPen pen(current_polygon_.color.darker(), LINE_WIDTH);
  painter.setPen(pen);

  QPoint first = current_polygon_.points[0];
  QPoint last = current_polygon_.points[current_polygon_.points.size() - 1];

  if (first == active_point_)
    first = active_point_pos_;
  if (last == active_point_)
    last = active_point_pos_;

  painter.drawLine(first * scalar_, last * scalar_);
}
