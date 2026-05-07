#include "polygoncanvas.h"

#include <QFile>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#include <algorithm>

#include "edgedetector.h"
#include "logger.h"
#include "qtadapter.h"
#include <segcore/geometry.h>
#include <segcore/normalized_format_serializer.h>

namespace {

QPoint ClampToImageBounds(const QPoint& point, const QSize& image_size)
{
  int x = qBound(0, point.x(), image_size.width() - 1);
  int y = qBound(0, point.y(), image_size.height() - 1);
  return QPoint(x, y);
}

}  // namespace

PolygonCanvas::PolygonCanvas(QWidget* parent) : QLabel(parent)
{
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
}

void PolygonCanvas::setPixmap(const QPixmap& pm)
{
  QLabel::setPixmap(pm);
  edges_ = EdgeDetector::Result{};
  spdlog::debug("[EdgeSnap] setPixmap called, size={}x{}, snap={}", pm.width(), pm.height(),
               snap_to_edges_);
  if (snap_to_edges_) ComputeEdges();
}

void PolygonCanvas::SetAnnotationSet(segcore::IAnnotationSet* set)
{
  annotation_set_ = set;
}

void PolygonCanvas::LoadArtifact(const segcore::Artifact& artifact)
{
  display_frame_ = segcore::NormaliseForDisplay(artifact);
  QLabel::setPixmap(
      QPixmap::fromImage(qt_adapter::FrameToQImage(display_frame_)));
  edges_ = EdgeDetector::Result{};
  if (snap_to_edges_) ComputeEdges();
}

void PolygonCanvas::SetClassColors(const QVector<QColor>& colors)
{
  class_colors_ = colors;
}

void PolygonCanvas::SetDrawingMode(int class_id)
{
  drawing_class_id_ = class_id;
}

void PolygonCanvas::SetSnapToEdges(bool enabled)
{
  snap_to_edges_ = enabled;
  spdlog::debug("[EdgeSnap] SetSnapToEdges({}), pixmap null={}, edges valid={}", enabled,
               pixmap().isNull(), edges_.isValid());
  if (enabled && !edges_.isValid()) ComputeEdges();
  repaint();
}

void PolygonCanvas::SetEdgeMapOnly(bool enabled)
{
  edge_map_only_ = enabled;
  if (enabled && !edges_.isValid()) ComputeEdges();
  repaint();
}

void PolygonCanvas::ComputeEdges()
{
  if (pixmap().isNull())
  {
    spdlog::debug("[EdgeSnap] ComputeEdges: pixmap is null, skipping");
    return;
  }
  spdlog::debug("[EdgeSnap] ComputeEdges: detecting...");
  edges_ = EdgeDetector::Detect(pixmap().toImage());
  const long edge_count =
      std::count(edges_.edge_map.begin(), edges_.edge_map.end(), uint8_t{1});
  spdlog::debug("[EdgeSnap] ComputeEdges: found {} edge pixels ({}x{})", edge_count, edges_.width,
               edges_.height);
  repaint();
}

QPoint PolygonCanvas::ApplySnap(const QPoint& pos) const
{
  if (!snap_to_edges_) return pos;
  const QPoint snapped = EdgeDetector::SnapToEdge(pos, edges_);
  return snapped;
}

void PolygonCanvas::Increase()
{
  scalar_ = scalar_ + 1.0f;
  QSize size = pixmap().size();
  setFixedSize(static_cast<int>(size.width() * scalar_),
               static_cast<int>(size.height() * scalar_));
}

void PolygonCanvas::Decrease()
{
  float new_scalar = scalar_ - 1.0f;
  if (new_scalar > 0.0f) scalar_ = new_scalar;
  QSize size = pixmap().size();
  setFixedSize(static_cast<int>(size.width() * scalar_),
               static_cast<int>(size.height() * scalar_));
}

void PolygonCanvas::ResetZoom()
{
  scalar_ = 1.0f;
  QSize size = pixmap().size();
  setFixedSize(static_cast<int>(size.width() * scalar_),
               static_cast<int>(size.height() * scalar_));
  spdlog::info("Zoom reset to 100%");
}

void PolygonCanvas::StartNewPolygon(int class_id, QColor color)
{
  if (class_id >= class_colors_.size())
  {
    class_colors_.resize(class_id + 1, Qt::red);
  }
  class_colors_[class_id] = color;
  SetDrawingMode(class_id);
  spdlog::info("Started new polygon with class_id: {}", class_id);
}

void PolygonCanvas::FinishCurrentPolygon()
{
  if (annotation_set_ == nullptr) return;
  const segcore::Segment* ip = annotation_set_->GetInProgress();
  if (ip != nullptr && static_cast<int>(ip->points.size()) >= 3)
  {
    segcore::SegmentId committed_id = ip->id;
    annotation_set_->CommitSegment(committed_id);
    annotation_set_->SelectSegment(committed_id);
    drawing_class_id_ = -1;
    emit CurrentClassChanged(-1);
    emit PolygonsChanged();
    repaint();
    spdlog::info("Polygon finished and saved.");
  }
  else
  {
    spdlog::info("Cannot finish polygon: need at least 3 points");
  }
}

void PolygonCanvas::ClearCurrentPolygon()
{
  if (annotation_set_ == nullptr) return;
  annotation_set_->CancelInProgress();
  drawing_class_id_ = -1;
  emit CurrentClassChanged(-1);
  repaint();
  spdlog::info("Drawing cancelled");
}

void PolygonCanvas::mouseMoveEvent(QMouseEvent* ev)
{
  cursor_pos_ = ev->pos() / scalar_;
  QPixmap pix = pixmap();
  if (!pix.isNull()) cursor_pos_ = ClampToImageBounds(cursor_pos_, pix.size());
  repaint();
}

void PolygonCanvas::mousePressEvent(QMouseEvent* ev)
{
  QPoint pos = ev->pos() / scalar_;
  QPixmap pix = pixmap();
  if (!pix.isNull()) pos = ClampToImageBounds(pos, pix.size());

  if (annotation_set_ == nullptr) return;

  float tolerance = static_cast<float>(POINT_SELECT_TOLERANCE) / scalar_;

  // Check in-progress segment for drag
  const auto* ip = annotation_set_->GetInProgress();
  if (ip != nullptr)
  {
    for (int i = 0; i < static_cast<int>(ip->points.size()); ++i)
    {
      if (IsPointNearPosition(qt_adapter::ToQPoint(ip->points[i]), pos, tolerance))
      {
        drag_segment_id_ = ip->id;
        drag_point_index_ = i;
        return;
      }
    }
  }

  // Check selected segment for drag
  auto sel_id = annotation_set_->GetSelectedSegmentId();
  if (sel_id != segcore::kInvalidSegmentId)
  {
    const auto* seg = annotation_set_->GetSegment(sel_id);
    if (seg != nullptr)
    {
      for (int i = 0; i < static_cast<int>(seg->points.size()); ++i)
      {
        if (IsPointNearPosition(qt_adapter::ToQPoint(seg->points[i]), pos, tolerance))
        {
          drag_segment_id_ = sel_id;
          drag_point_index_ = i;
          return;
        }
      }
    }
  }
}

void PolygonCanvas::mouseReleaseEvent(QMouseEvent* ev)
{
  QPoint pos = ev->pos() / scalar_;
  QPixmap pix = pixmap();
  if (!pix.isNull()) pos = ClampToImageBounds(pos, pix.size());
  pos = ApplySnap(pos);

  if (annotation_set_ == nullptr) return;

  // Handle drag end
  if (drag_segment_id_ != segcore::kInvalidSegmentId)
  {
    bool ctrl = QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier);
    if (ctrl)
    {
      annotation_set_->RemovePoint(drag_segment_id_, drag_point_index_);
    }
    else
    {
      annotation_set_->MovePoint(drag_segment_id_, drag_point_index_,
                                 qt_adapter::FromQPoint(pos));
    }
    drag_segment_id_ = segcore::kInvalidSegmentId;
    drag_point_index_ = -1;
    emit PolygonsChanged();
    repaint();
    return;
  }

  // Right click: commit in-progress if >= 3 points
  if (ev->button() == Qt::RightButton)
  {
    const segcore::Segment* ip = annotation_set_->GetInProgress();
    if (ip != nullptr && static_cast<int>(ip->points.size()) >= 3)
    {
      segcore::SegmentId committed_id = ip->id;
      annotation_set_->CommitSegment(committed_id);
      annotation_set_->SelectSegment(committed_id);
      drawing_class_id_ = -1;
      emit CurrentClassChanged(-1);
      emit PolygonsChanged();
      repaint();
    }
    return;
  }

  bool ctrl = QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier);

  // Ctrl+Click on selected segment: insert point on nearest edge
  if (ctrl)
  {
    auto sel_id = annotation_set_->GetSelectedSegmentId();
    if (sel_id != segcore::kInvalidSegmentId)
    {
      const segcore::Segment* seg = annotation_set_->GetSegment(sel_id);
      if (seg != nullptr && seg->points.size() >= 2)
      {
        int idx = segcore::InsertionIndex(*seg, qt_adapter::FromQPoint(pos));
        annotation_set_->InsertPoint(sel_id, idx, qt_adapter::FromQPoint(pos));
        emit PolygonsChanged();
        repaint();
      }
    }
    return;
  }

  // Drawing mode: add point to in-progress
  if (drawing_class_id_ >= 0)
  {
    const segcore::Segment* ip = annotation_set_->GetInProgress();
    segcore::SegmentId id;
    if (ip == nullptr)
    {
      id = annotation_set_->BeginSegment(drawing_class_id_);
    }
    else
    {
      id = ip->id;
    }
    annotation_set_->AddPoint(id, qt_adapter::FromQPoint(pos));
    repaint();
    return;
  }

  // Not drawing: try to select a polygon
  SelectPolygon(pos);
}

void PolygonCanvas::keyPressEvent(QKeyEvent* ev)
{
  if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter)
  {
    FinishCurrentPolygon();
  }
  else if (ev->key() == Qt::Key_Escape)
  {
    if (annotation_set_ != nullptr && annotation_set_->GetInProgress() != nullptr)
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
  else if (ev->matches(QKeySequence::Undo))
  {
    Undo();
  }
  else if (ev->matches(QKeySequence::Redo))
  {
    Redo();
  }
  else if (ev->matches(QKeySequence::Copy))
  {
    CopySelectedPolygon();
  }
  else if (ev->matches(QKeySequence::Paste))
  {
    PastePolygon();
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
  DrawEdgeOverlay(painter);
  DrawCompletedSegments(painter);
  DrawInProgressSegment(painter);
}

QSize PolygonCanvas::GetOriginalImageSize() const
{
  if (!pixmap().isNull()) return pixmap().size();
  return QSize(0, 0);
}

void PolygonCanvas::ExportAnnotations(const QString& filename, int)
{
  if (annotation_set_ == nullptr) return;

  QSize img_size = GetOriginalImageSize();
  if (img_size.width() == 0 || img_size.height() == 0) return;

  std::string text = segcore::SegmentsToNormalizedFormat(
      annotation_set_->GetSegments(), img_size.width(), img_size.height());

  QFile file(filename);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    file.write(QByteArray::fromStdString(text));
  }
  else
  {
    spdlog::error("Cannot open file for writing: {}", filename.toStdString());
  }

  spdlog::info("Annotations exported to: {}", filename.toStdString());
}

void PolygonCanvas::LoadAnnotations(const QString& filepath, const QVector<QColor>& class_colors)
{
  if (annotation_set_ == nullptr) return;

  QSize img_size = GetOriginalImageSize();
  if (img_size.width() == 0 || img_size.height() == 0) return;

  QFile file(filepath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    spdlog::error("Cannot open file for reading: {}", filepath.toStdString());
    return;
  }

  class_colors_ = class_colors;
  annotation_set_->ClearAll();

  std::string text = file.readAll().toStdString();
  auto segments = segcore::NormalizedFormatToSegments(text, img_size.width(), img_size.height());

  for (const auto& seg : segments)
  {
    auto id = annotation_set_->BeginSegment(seg.class_id);
    for (const auto& p : seg.points)
    {
      annotation_set_->AddPoint(id, p);
    }
    annotation_set_->CommitSegment(id);
  }

  update();
  spdlog::info("Loaded {} polygons from: {}", segments.size(), filepath.toStdString());
}

void PolygonCanvas::ClearAllPolygons()
{
  if (annotation_set_ != nullptr) annotation_set_->ClearAll();
  drawing_class_id_ = -1;
  emit PolygonsChanged();
  update();
}

void PolygonCanvas::AddPolygonFromPlugin(const QVector<QPoint>& points, int class_id,
                                         const QColor&)
{
  if (points.size() < 3 || annotation_set_ == nullptr) return;
  auto id = annotation_set_->BeginSegment(class_id);
  for (const auto& p : points)
  {
    annotation_set_->AddPoint(id, qt_adapter::FromQPoint(p));
  }
  annotation_set_->CommitSegment(id);
  emit PolygonsChanged();
  repaint();
  spdlog::info("Added plugin polygon with {} points (class_id={})", points.size(), class_id);
}

void PolygonCanvas::SelectPolygon(const QPoint& pos)
{
  if (annotation_set_ == nullptr) return;
  auto seg_id =
      segcore::HitTestSegment(annotation_set_->GetSegments(), qt_adapter::FromQPoint(pos));
  annotation_set_->DeselectAll();
  if (seg_id != segcore::kInvalidSegmentId)
  {
    annotation_set_->SelectSegment(seg_id);
    spdlog::info("Selected segment {}", seg_id);
  }
  else
  {
    spdlog::info("Deselected all");
  }
  repaint();
}

void PolygonCanvas::DeselectAll()
{
  if (annotation_set_ != nullptr) annotation_set_->DeselectAll();
  repaint();
}

void PolygonCanvas::DeleteSelectedPolygon()
{
  if (annotation_set_ == nullptr) return;
  auto sel_id = annotation_set_->GetSelectedSegmentId();
  if (sel_id != segcore::kInvalidSegmentId)
  {
    annotation_set_->DeleteSegment(sel_id);
    emit PolygonsChanged();
    update();
  }
}

int PolygonCanvas::GetSelectedPolygonIndex() const
{
  if (annotation_set_ == nullptr) return -1;
  return (annotation_set_->GetSelectedSegmentId() != segcore::kInvalidSegmentId) ? 0 : -1;
}

bool PolygonCanvas::HasAnnotations() const
{
  return annotation_set_ != nullptr && !annotation_set_->GetSegments().empty();
}

int PolygonCanvas::GetAnnotationCount() const
{
  if (annotation_set_ == nullptr) return 0;
  return static_cast<int>(annotation_set_->GetSegments().size());
}

void PolygonCanvas::Undo()
{
  if (annotation_set_ == nullptr || !annotation_set_->CanUndo()) return;
  annotation_set_->Undo();
  emit PolygonsChanged();
  repaint();
}

void PolygonCanvas::Redo()
{
  if (annotation_set_ == nullptr || !annotation_set_->CanRedo()) return;
  annotation_set_->Redo();
  emit PolygonsChanged();
  repaint();
}

bool PolygonCanvas::CanUndo() const
{
  return annotation_set_ != nullptr && annotation_set_->CanUndo();
}

bool PolygonCanvas::CanRedo() const
{
  return annotation_set_ != nullptr && annotation_set_->CanRedo();
}

void PolygonCanvas::CopySelectedPolygon()
{
  if (annotation_set_ == nullptr) return;
  auto sel_id = annotation_set_->GetSelectedSegmentId();
  if (sel_id != segcore::kInvalidSegmentId)
  {
    annotation_set_->CopySegment(sel_id);
  }
}

void PolygonCanvas::PastePolygon()
{
  if (annotation_set_ == nullptr) return;
  segcore::SegmentId pasted_id = annotation_set_->PasteSegment();
  if (pasted_id == segcore::kInvalidSegmentId) return;
  annotation_set_->SelectSegment(pasted_id);
  emit PolygonsChanged();
  repaint();
}

bool PolygonCanvas::HasClipboard() const
{
  return annotation_set_ != nullptr && annotation_set_->HasClipboard();
}

bool PolygonCanvas::IsPointNearPosition(const QPoint& point, const QPoint& position,
                                        float tolerance) const
{
  float dx = static_cast<float>(point.x() - position.x());
  float dy = static_cast<float>(point.y() - position.y());
  return dx * dx + dy * dy <= tolerance * tolerance;
}

void PolygonCanvas::DrawImage(QPainter& painter)
{
  QPixmap pix = pixmap();
  if (pix.isNull()) return;
  if (edge_map_only_)
  {
    painter.fillRect(0, 0, static_cast<int>(pix.width() * scalar_),
                     static_cast<int>(pix.height() * scalar_), Qt::black);
  }
  else
  {
    pix = pix.scaled(static_cast<int>(pix.width() * scalar_),
                     static_cast<int>(pix.height() * scalar_));
    painter.drawPixmap(0, 0, pix);
  }
}

void PolygonCanvas::DrawEdgeOverlay(QPainter& painter)
{
  if (edges_.overlay.isNull()) return;

  if (edge_map_only_)
  {
    const int w = static_cast<int>(edges_.width * scalar_);
    const int h = static_cast<int>(edges_.height * scalar_);
    QImage bw(edges_.width, edges_.height, QImage::Format_RGB32);
    bw.fill(Qt::black);
    for (int y = 0; y < edges_.height; ++y)
    {
      QRgb* line = reinterpret_cast<QRgb*>(bw.scanLine(y));
      for (int x = 0; x < edges_.width; ++x)
      {
        if (edges_.edge_map[static_cast<size_t>(y * edges_.width + x)])
          line[x] = qRgb(255, 255, 255);
      }
    }
    painter.drawImage(0, 0, bw.scaled(w, h, Qt::IgnoreAspectRatio, Qt::FastTransformation));
    return;
  }

  if (!snap_to_edges_) return;
  const QImage scaled = edges_.overlay.scaled(
      static_cast<int>(edges_.overlay.width() * scalar_),
      static_cast<int>(edges_.overlay.height() * scalar_),
      Qt::IgnoreAspectRatio, Qt::FastTransformation);
  painter.drawImage(0, 0, scaled);
}

void PolygonCanvas::DrawCompletedSegments(QPainter& painter)
{
  if (annotation_set_ == nullptr) return;
  for (const auto& seg : annotation_set_->GetSegments())
  {
    if (seg.points.size() < 2) continue;
    QColor color = class_colors_.value(seg.class_id, Qt::red);
    int line_width = LINE_WIDTH;
    if (seg.selected)
    {
      line_width = 2;
      color = color.lighter(120);
    }
    else
    {
      color.setAlpha(180);
    }
    QPen pen(color, line_width);
    painter.setPen(pen);
    for (const auto& p : seg.points)
    {
      QPoint sp = qt_adapter::ToQPoint(p) * scalar_;
      painter.fillRect(sp.x() - POINT_DRAW_SIZE / 2, sp.y() - POINT_DRAW_SIZE / 2,
                       POINT_DRAW_SIZE, POINT_DRAW_SIZE, color);
    }
    for (int i = 1; i < static_cast<int>(seg.points.size()); ++i)
    {
      painter.drawLine(qt_adapter::ToQPoint(seg.points[static_cast<size_t>(i - 1)]) * scalar_,
                       qt_adapter::ToQPoint(seg.points[static_cast<size_t>(i)]) * scalar_);
    }
    painter.setPen(QPen(color.darker(120), line_width));
    painter.drawLine(qt_adapter::ToQPoint(seg.points.front()) * scalar_,
                     qt_adapter::ToQPoint(seg.points.back()) * scalar_);
  }
}

void PolygonCanvas::DrawInProgressSegment(QPainter& painter)
{
  if (annotation_set_ == nullptr) return;
  const segcore::Segment* ip = annotation_set_->GetInProgress();
  if (ip == nullptr) return;

  QColor color = class_colors_.value(ip->class_id, Qt::red);
  QPen pen(color, POINT_DRAW_SIZE);
  painter.setPen(pen);

  for (int i = 0; i < static_cast<int>(ip->points.size()); ++i)
  {
    QPoint draw_pos =
        (drag_segment_id_ == ip->id && drag_point_index_ == i)
            ? cursor_pos_ * static_cast<int>(scalar_)
            : qt_adapter::ToQPoint(ip->points[static_cast<size_t>(i)]) * scalar_;
    painter.drawPoint(draw_pos);
  }

  if (ip->points.size() < 2) return;

  painter.setPen(QPen(color, LINE_WIDTH));
  for (int i = 1; i < static_cast<int>(ip->points.size()); ++i)
  {
    QPoint prev =
        (drag_segment_id_ == ip->id && drag_point_index_ == i - 1)
            ? cursor_pos_ * static_cast<int>(scalar_)
            : qt_adapter::ToQPoint(ip->points[static_cast<size_t>(i - 1)]) * scalar_;
    QPoint curr =
        (drag_segment_id_ == ip->id && drag_point_index_ == i)
            ? cursor_pos_ * static_cast<int>(scalar_)
            : qt_adapter::ToQPoint(ip->points[static_cast<size_t>(i)]) * scalar_;
    painter.drawLine(prev, curr);
  }

  painter.setPen(QPen(color.darker(), LINE_WIDTH));
  QPoint first =
      (drag_segment_id_ == ip->id && drag_point_index_ == 0)
          ? cursor_pos_ * static_cast<int>(scalar_)
          : qt_adapter::ToQPoint(ip->points.front()) * scalar_;
  QPoint last_pt =
      (drag_segment_id_ == ip->id &&
       drag_point_index_ == static_cast<int>(ip->points.size()) - 1)
          ? cursor_pos_ * static_cast<int>(scalar_)
          : qt_adapter::ToQPoint(ip->points.back()) * scalar_;
  painter.drawLine(first, last_pt);
}
