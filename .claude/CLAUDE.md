# GitHub Copilot Instructions for PolySeg

## Project Overview
PolySeg is a secure offline polygon annotation tool for creating instance segmentation datasets. The application enables interactive drawing, editing, and management of polygon annotations on images, exporting to normalized coordinate format compatible with various deep learning frameworks through plugin architecture.

## Code Standards and Language Requirements

### Language
- All code, comments, variables, and function names must be in English
- No mixed Polish-English terminology
- Documentation and commit messages in English only

### Professional Standards
- No emoji or decorative symbols in code or documentation
- Clean, readable code following industry best practices
- Consistent naming conventions throughout the project
- Professional commit messages and documentation

### Code Style
- Follow Google C++ Style Guide
- Use clang-format for automatic formatting (configuration in .clang-format)
- Use clang-tidy for static analysis (configuration in .clang-tidy)
- Run formatting before commits: `clang-format -i *.cpp *.h`
- Run linting: `clang-tidy *.cpp -- -I/path/to/qt/include`

### Commit Message Format
Use conventional commit format with the following structure:

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, no logic change)
- `refactor`: Code refactoring (no feature or fix)
- `test`: Adding or updating tests
- `chore`: Build, config, or tooling changes

**Rules:**
- Subject line: max 50 characters, imperative mood, no period
- Body: wrap at 72 characters, explain what and why (not how)
- Scope: optional, e.g., `canvas`, `mainwindow`, `plugins`, `ui`

**Examples:**
```
feat(canvas): add multi-polygon selection support

fix(plugins): resolve memory leak in AI model loading

docs: update build instructions

refactor(mainwindow): extract file dialog logic to separate method
```

## Architecture

### Core Components
- **PolygonCanvas** (`polygoncanvas.h/cpp`) - Custom QLabel widget handling polygon drawing, editing, and rendering
- **MainWindow** (`mainwindow.h/cpp`) - Main application window managing UI and file operations
- **Polygon struct** - Data structure containing class_id, points vector, color, and selection state

### Key Data Structures
```cpp
struct Polygon {
    int class_id;
    QVector<QPoint> points;
    QColor color;
    bool is_selected;
};
```

### Current Implementation Status
Phase 1.1 Complete: Core multi-polygon system
- Multiple polygons per image
- Individual polygon colors
- Current polygon being drawn vs completed polygons
- Keyboard shortcuts (Enter to finish, Esc to cancel)

## Code Style Guidelines

### Qt Conventions
- Use Qt types: `QString`, `QVector`, `QPoint`, etc.
- Member variables end with underscore: `polygons_`, `scalar_`
- Use Qt signals/slots for event handling
- Prefer Qt containers over STL

### Naming
- Classes: `PascalCase` (e.g., `PolygonCanvas`)
- Functions: `PascalCase` (e.g., `FinishCurrentPolygon`)
- Variables: `snake_case` (e.g., `current_polygon_`)
- Member variables: `snake_case` with trailing underscore (e.g., `polygons_`, `scalar_`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `POINT_SELECT_TOLERANCE`)

### Formatting
- Opening braces on new line (Allman style)
- 2-space indentation
- 100 character line limit
- Pointer alignment left (`int* ptr`)

### File Organization
- Headers: class declaration, inline methods
- Implementation: method definitions, helper functions
- Keep related functionality together

## Implementation Patterns

### Polygon Drawing Flow
1. User clicks → point added to `current_polygon_.points`
2. `Ctrl+Click` → insert/delete point based on proximity
3. `Enter` key → call `FinishCurrentPolygon()`
4. Polygon moved to `polygons_` vector and new `current_polygon_` started

### Rendering Strategy
```cpp
void paintEvent() {
    // 1. Draw image
    // 2. Draw all completed polygons (from polygons_)
    // 3. Draw current polygon being edited (current_polygon_)
}
```

### Normalized Export Format
- One line per polygon: `class_id x1 y1 x2 y2 ...`
- Coordinates normalized to [0, 1]: `x_norm = x_pixel / image_width`
- All polygons exported to single .txt file
- Compatible with multiple frameworks via plugin system

## Future Development

### Immediate Next Steps (Phase 1.2)
- Add class management UI (QListWidget)
- Default classes: person, car, dog
- Class selection to set current_polygon_.color and class_id

### Upcoming Features
- Project system (.polyseg folders)
- Image navigation (previous/next)
- Annotation import
- Polygon selection and editing
- AI auto-detection (plugin-based integration)

## Common Tasks

### Adding New Methods to PolygonCanvas
1. Declare in `polygoncanvas.h` (public/protected/private)
2. Implement in `polygoncanvas.cpp`
3. Update `paintEvent()` if visual changes needed
4. Add keyboard shortcuts in `keyPressEvent()` if applicable

### Working with Polygons
- **Current polygon:** `current_polygon_` (being drawn)
- **All completed:** `polygons_` vector
- **Selection:** Use `selected_polygon_index_` (-1 = none)

### Event Handling
- Mouse: `mousePressEvent`, `mouseMoveEvent`, `mouseReleaseEvent`
- Keyboard: `keyPressEvent` (requires `setFocusPolicy(Qt::StrongFocus)`)
- Always call `repaint()` after state changes

## Qt-Specific Tips

### Coordinate Scaling
- Images can be zoomed: use `scalar_` multiplier
- Click position: `ev->pos() / scalar_` for actual coordinates
- Drawing: `point * scalar_` for scaled display

### QPainter Usage
```cpp
QPainter painter(this);
QPen pen(color, width);
painter.setPen(pen);
painter.drawPoint(point);
painter.drawLine(p1, p2);
```

### File I/O
- Use `QFile`, `QTextStream` for text files
- Use `QFileDialog` for user file selection
- Use `QFileInfo` for path manipulation

## Testing Guidelines
- Build: `qmake6 PolySeg.pro && make -j4`
- Run: `./PolySeg`
- Test multi-polygon: Draw → Enter → Draw → Enter → Save
- Verify annotation format: Check .txt file has multiple lines

## Dependencies
- C++17 compiler
- qmake build system
- Future: OpenCV (for AI features in Phase 7)

## Notes for AI Assistance
- Follow existing code patterns in polygoncanvas.cpp/mainwindow.cpp
- Maintain compatibility with normalized annotation format
- Keep UI responsive (avoid blocking operations)
- Prioritize user workflow efficiency
- Document complex algorithms (e.g., point-in-polygon)
- Preserve framework-agnostic plugin architecture

2. **Drawing points** (red, thickness 5):
   - For each point in `points_`
   - Takes into account active point position during dragging

3. **Drawing lines between points** (red, thickness 1):
   - Connects consecutive points with lines
   - Takes into account active point position

4. **Drawing closing line** (dark red, thickness 1):
   - Connects first and last point, creating closed polygon

## Helper Functions:

**Distance(const QPoint& p1, const QPoint& p2)**
- Calculates Euclidean distance between two points
- Used to find nearest point

## How to Use the Application:

1. **Loading image**:
   - Click "Load" button
   - Select image file (PNG, JPG, BMP)

2. **Adding points**:
   - Click left mouse button where you want to add a point
   - Points are added sequentially and connected with lines

4. **Inserting point between existing ones**:
   - Hold Ctrl and left-click on line segment
   - Point will be inserted at appropriate position in sequence

5. **Moving point**:
   - Click on existing point (within ±5 pixels radius)
   - While holding mouse button, drag point to new location
   - Release mouse button

6. **Deleting point**:
   - Click on point while holding Ctrl
   - Point will be removed from polygon
   - Note: Distance from point is checked first (±5px), if closer than 5px removes point, otherwise inserts new point between segments

7. **Zoom**:
   - Click "+" to zoom in
   - Click "-" to zoom out

7. **Saving annotations**:
   - After marking area with polygon, click "Save Annotations"
   - Select location and .txt filename
   - Save format: `class_id x1 y1 x2 y2 x3 y3 ...` where all coordinates are normalized (0.0-1.0)

## Annotation Format:

Application exports annotations in normalized format for instance segmentation:
- Each line represents one object
- Format: `<class_id> <x1> <y1> <x2> <y2> ... <xn> <yn>`
- All coordinates are normalized relative to image size (0.0 - 1.0)
- `class_id` - Object class ID (currently fixed: 0)
- `x, y` - Normalized coordinates of consecutive polygon points
- Format compatible with multiple frameworks through plugin system

Example:
```
0 0.123 0.456 0.234 0.567 0.345 0.678 0.456 0.789
```

## Known Limitations:

- Code for inserting points between existing ones is marked as "buggy" in comments
- Distance from nearest point is calculated, not from line between points
- No undo operation available
- No ability to change class_id from interface (currently fixed: 0)
- Application saves only one object per image (one polygon)

## Technologies:

- C++17
- QPainter for 2D graphics

## Build:

Build from the `build` directory:
```bash
cd build
qmake ../PolySeg.pro
make -j$(nproc)
```

For clean build:
```bash
cd build
make clean
qmake ../PolySeg.pro
make -j$(nproc)
```
