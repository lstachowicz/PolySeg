#ifndef POLYGONCANVAS_H
#define POLYGONCANVAS_H

#include <QColor>
#include <QImage>
#include <QLabel>
#include <QPoint>
#include <QVector>

#include "edgedetector.h"
#include <imgproc/frame.h>
#include <segcore/artifact.h>
#include <segcore/iannotation_set.h>
#include <segcore/types.h>

class PolygonCanvas : public QLabel
{
  Q_OBJECT

 public:
  explicit PolygonCanvas(QWidget* parent = nullptr);

  void Increase();
  void Decrease();
  void ResetZoom();

  QSize GetOriginalImageSize() const;
  void ExportAnnotations(const QString& filename, int class_id = 0);
  void LoadAnnotations(const QString& filepath, const QVector<QColor>& class_colors);
  void ClearAllPolygons();

  void StartNewPolygon(int class_id = 0, QColor color = Qt::red);
  void FinishCurrentPolygon();
  void ClearCurrentPolygon();
  void SetDrawingMode(int class_id);

  // Plugin integration
  void AddPolygonFromPlugin(const QVector<QPoint>& points, int class_id, const QColor& color);
  QPixmap GetPixmap() const { return pixmap(); }

  // Selection & Editing
  void SelectPolygon(const QPoint& pos);
  void DeselectAll();
  void DeleteSelectedPolygon();
  int GetSelectedPolygonIndex() const;

  // Annotation set injection
  void SetAnnotationSet(segcore::IAnnotationSet* set);
  segcore::IAnnotationSet* GetAnnotationSet() const { return annotation_set_; }
  void LoadArtifact(const segcore::Artifact& artifact);
  void SetClassColors(const QVector<QColor>& colors);

  // Annotation queries
  bool HasAnnotations() const;
  int GetAnnotationCount() const;

  // Undo/Redo
  void Undo();
  void Redo();
  bool CanUndo() const;
  bool CanRedo() const;

  // Copy/Paste
  void CopySelectedPolygon();
  void PastePolygon();
  bool HasClipboard() const;

  // Edge snap
  void SetSnapToEdges(bool enabled);
  void SetEdgeMapOnly(bool enabled);
  void ComputeEdges();
  void setPixmap(const QPixmap& pixmap);

 signals:
  void PolygonsChanged();
  void CurrentClassChanged(int class_id);

 protected:
  void mouseMoveEvent(QMouseEvent* ev) override;
  void mousePressEvent(QMouseEvent* ev) override;
  void mouseReleaseEvent(QMouseEvent* ev) override;
  void paintEvent(QPaintEvent* paint_event) override;
  void keyPressEvent(QKeyEvent* ev) override;

 private:
  bool IsPointNearPosition(const QPoint& point, const QPoint& position, float tolerance) const;
  void DrawImage(QPainter& painter);
  void DrawEdgeOverlay(QPainter& painter);
  void DrawCompletedSegments(QPainter& painter);
  void DrawInProgressSegment(QPainter& painter);
  QPoint ApplySnap(const QPoint& pos) const;

  static constexpr int POINT_SELECT_TOLERANCE = 5;
  static constexpr int POINT_DRAW_SIZE = 5;
  static constexpr int LINE_WIDTH = 1;

  segcore::IAnnotationSet* annotation_set_ = nullptr;
  polyseg::Frame<segcore::Rgb24> display_frame_;
  segcore::SegmentId drag_segment_id_ = segcore::kInvalidSegmentId;
  int drag_point_index_ = -1;
  QVector<QColor> class_colors_;
  int drawing_class_id_ = -1;

  QPoint cursor_pos_;
  float scalar_ = 1.0f;

  EdgeDetector::Result edges_;
  bool snap_to_edges_ = false;
  bool edge_map_only_ = false;
};

#endif  // POLYGONCANVAS_H
