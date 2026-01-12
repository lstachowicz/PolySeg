#ifndef POLYGONCANVAS_H
#define POLYGONCANVAS_H

#include <QColor>
#include <QLabel>
#include <QPoint>
#include <QVector>

struct Polygon
{
  int class_id = 0;
  QVector<QPoint> points;
  QColor color = Qt::red;
  bool is_selected = false;
};

class PolygonCanvas : public QLabel
{
  Q_OBJECT

 public:
  explicit PolygonCanvas(QWidget* parent = nullptr);

  void Increase();
  void Decrease();
  void ResetZoom();

  QVector<Polygon> GetPolygons() const { return polygons_; }
  QSize GetOriginalImageSize() const;
  void ExportYolo(const QString& filename, int class_id = 0);
  void LoadYoloAnnotations(const QString& filepath, const QVector<QColor>& class_colors);
  void ClearAllPolygons();

  void StartNewPolygon(int class_id = 0, QColor color = Qt::red);
  void FinishCurrentPolygon();
  void ClearCurrentPolygon();

  // Plugin integration
  void AddPolygonFromPlugin(const QVector<QPoint>& points, int class_id, const QColor& color);
  QPixmap GetPixmap() const { return pixmap(); }

  // Selection & Editing
  void SelectPolygon(const QPoint& pos);
  void DeselectAll();
  void DeleteSelectedPolygon();
  int GetSelectedPolygonIndex() const { return selected_polygon_index_; }

 protected:
  void mouseMoveEvent(QMouseEvent* ev) override;
  void mousePressEvent(QMouseEvent* ev) override;
  void mouseReleaseEvent(QMouseEvent* ev) override;
  void paintEvent(QPaintEvent* paint_event) override;
  void keyPressEvent(QKeyEvent* ev) override;

 private:
  // Helper methods
  bool IsPointNearPosition(const QPoint& point, const QPoint& position, int tolerance) const;
  int FindNearestSegmentIndex(const QPoint& position) const;
  void HandlePointDrag(const QPoint& position);
  void HandlePointInsertion(const QPoint& position);
  void DrawImage(QPainter& painter);
  void DrawPoints(QPainter& painter);
  void DrawSegments(QPainter& painter);
  void DrawClosingSegment(QPainter& painter);

  // Constants
  static constexpr int POINT_SELECT_TOLERANCE = 5;
  static constexpr int POINT_DRAW_SIZE = 5;
  static constexpr int LINE_WIDTH = 1;

  // Member variables
  QVector<Polygon> polygons_;
  Polygon current_polygon_;
  int selected_polygon_index_ = -1;
  QPoint active_point_;
  QPoint active_point_pos_;
  float scalar_ = 1.0;
};

#endif  // POLYGONCANVAS_H
