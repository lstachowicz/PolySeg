#include "superlabel.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QStyle>
#include <QFile>
#include <QTextStream>

#include <iostream>
#include <limits>

inline int Distance(const QPoint& p1, const QPoint& p2)
{
    auto result = sqrt(pow(p1.x() - p2.x(), 2) + pow(p1.y() - p2.y(), 2));
    return result;
}

// Oblicza odległość punktu od segmentu linii
inline float DistanceFromPointToSegment(const QPoint& point, const QPoint& lineStart, const QPoint& lineEnd)
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
    float t = ((point.x() - lineStart.x()) * dx + (point.y() - lineStart.y()) * dy) / segmentLengthSquared;
    
    // Ogranicz t do zakresu [0, 1] - projekcja musi być na segmencie
    t = qBound(0.0f, t, 1.0f);
    
    // Znajdź najbliższy punkt na segmencie
    QPointF closestPoint(lineStart.x() + t * dx, lineStart.y() + t * dy);
    
    // Oblicz odległość od punktu do najbliższego punktu na segmencie
    float distX = point.x() - closestPoint.x();
    float distY = point.y() - closestPoint.y();
    
    return sqrt(distX * distX + distY * distY);
}


SuperLabel::SuperLabel(QWidget *parent) : QLabel(parent)
{
    // Initialize with default color for first polygon
    current_polygon_.class_id = 0;
    current_polygon_.color = Qt::red;
    
    // Enable keyboard focus to receive key events
    setFocusPolicy(Qt::StrongFocus);
}

void SuperLabel::Increase()
{
    scalar_ = scalar_ + 1.0;

    auto size = pixmap().size() * scalar_;
    setFixedSize(size);
}

void SuperLabel::Decrease()
{
    auto new_scalar_ = scalar_ - 1.0;
    if (new_scalar_ > 0)
    {
        scalar_ = new_scalar_;
    }

    auto size = pixmap().size() * scalar_;
    setFixedSize(size);
}

void SuperLabel::StartNewPolygon(int class_id, QColor color)
{
    current_polygon_.class_id = class_id;
    current_polygon_.color = color;
    current_polygon_.points.clear();
    current_polygon_.is_selected = false;
    std::cout << "Started new polygon with class_id: " << class_id << std::endl;
}

void SuperLabel::FinishCurrentPolygon()
{
    if (current_polygon_.points.size() >= 3)
    {
        polygons_.push_back(current_polygon_);
        std::cout << "Finished polygon with " << current_polygon_.points.size() << " points" << std::endl;
        current_polygon_.points.clear();
        repaint();
    }
    else
    {
        std::cout << "Cannot finish polygon: need at least 3 points" << std::endl;
    }
}

void SuperLabel::ClearCurrentPolygon()
{
    current_polygon_.points.clear();
    repaint();
    std::cout << "Cleared current polygon" << std::endl;
}

void SuperLabel::mouseMoveEvent(QMouseEvent* ev)
{
    auto pos = ev->pos() / scalar_;
    active_point_pos_ = pos;
}

void SuperLabel::mousePressEvent(QMouseEvent *ev)
{
    QPoint pos = ev->pos() / scalar_;
    
    for (const auto& point : current_polygon_.points)
    {
        if (IsPointNearPosition(point, pos, POINT_SELECT_TOLERANCE))
        {
            active_point_ = point;
            active_point_pos_ = point;
            break;
        }
    }
}

void SuperLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    QPoint pos = ev->pos() / scalar_;

    if (!active_point_.isNull())
    {
        HandlePointDrag(pos);
    }
    else
    {
        HandlePointInsertion(pos);
    }

    active_point_ = QPoint();
    active_point_pos_ = QPoint();
    repaint();
}

void SuperLabel::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter)
    {
        FinishCurrentPolygon();
    }
    else if (ev->key() == Qt::Key_Escape)
    {
        ClearCurrentPolygon();
    }
    else
    {
        QLabel::keyPressEvent(ev);
    }
}

void SuperLabel::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    
    DrawImage(painter);
    
    // Draw all completed polygons
    for (const auto& polygon : polygons_)
    {
        if (polygon.points.size() < 2) continue;
        
        QPen pen(polygon.color, polygon.is_selected ? 3 : LINE_WIDTH);
        painter.setPen(pen);
        
        // Draw points
        for (const auto& point : polygon.points)
        {
            painter.drawPoint(point * scalar_);
        }
        
        // Draw segments
        for (int i = 1; i < polygon.points.size(); ++i)
        {
            painter.drawLine(polygon.points[i - 1] * scalar_, polygon.points[i] * scalar_);
        }
        
        // Draw closing segment
        painter.setPen(QPen(polygon.color.darker(), polygon.is_selected ? 3 : LINE_WIDTH));
        painter.drawLine(polygon.points[0] * scalar_, 
                        polygon.points[polygon.points.size() - 1] * scalar_);
    }
    
    // Draw current polygon being edited
    DrawPoints(painter);
    DrawSegments(painter);
    DrawClosingSegment(painter);
}

QSize SuperLabel::GetOriginalImageSize() const
{
    if (!pixmap().isNull())
    {
        return pixmap().size();
    }
    return QSize(0, 0);
}

void SuperLabel::ExportYolo(const QString& filename, int class_id)
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
    
    std::cout << "YOLO label exported to: " << filename.toStdString() << std::endl;
    std::cout << "Polygons: " << polygons_.size() << std::endl;
}

// Private helper methods

bool SuperLabel::IsPointNearPosition(const QPoint& point, const QPoint& position, int tolerance) const
{
    return qAbs(point.x() - position.x()) <= tolerance && 
           qAbs(point.y() - position.y()) <= tolerance;
}

int SuperLabel::FindNearestSegmentIndex(const QPoint& position) const
{
    float min_distance = std::numeric_limits<float>::max();
    int insert_index = -1;
    
    for (int i = 0; i < current_polygon_.points.size(); ++i)
    {
        int next_i = (i + 1) % current_polygon_.points.size();
        float distance = DistanceFromPointToSegment(position, current_polygon_.points[i], current_polygon_.points[next_i]);
        
        if (distance < min_distance)
        {
            min_distance = distance;
            insert_index = next_i;
        }
    }
    
    return insert_index;
}

void SuperLabel::HandlePointDrag(const QPoint& position)
{
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
            break;
        }
    }
}

void SuperLabel::HandlePointInsertion(const QPoint& position)
{
    bool ctrl_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier);
    
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

void SuperLabel::DrawImage(QPainter& painter)
{
    QPixmap pix = pixmap();
    if (!pix.isNull())
    {
        pix = pix.scaled(pix.width() * scalar_, pix.height() * scalar_);
        painter.drawPixmap(0, 0, pix);
    }
}

void SuperLabel::DrawPoints(QPainter& painter)
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

void SuperLabel::DrawSegments(QPainter& painter)
{
    if (current_polygon_.points.size() < 2) return;
    
    QPen pen(current_polygon_.color, LINE_WIDTH);
    painter.setPen(pen);

    for (int i = 1; i < current_polygon_.points.size(); ++i)
    {
        QPoint prev = current_polygon_.points[i - 1];
        QPoint curr = current_polygon_.points[i];

        if (curr == active_point_) curr = active_point_pos_;
        if (prev == active_point_) prev = active_point_pos_;

        painter.drawLine(prev * scalar_, curr * scalar_);
    }
}

void SuperLabel::DrawClosingSegment(QPainter& painter)
{
    if (current_polygon_.points.size() < 2) return;
    
    QPen pen(current_polygon_.color.darker(), LINE_WIDTH);
    painter.setPen(pen);

    QPoint first = current_polygon_.points[0];
    QPoint last = current_polygon_.points[current_polygon_.points.size() - 1];

    if (first == active_point_) first = active_point_pos_;
    if (last == active_point_) last = active_point_pos_;

    painter.drawLine(first * scalar_, last * scalar_);
}
