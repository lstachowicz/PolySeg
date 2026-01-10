# PolySeg

**Interactive Polygon Annotation Tool for AI Training**

A lightweight Qt6-based desktop application for creating precise polygon annotations on images. Designed for machine learning workflows, PolySeg enables fast, manual segmentation with direct YOLO format export for object detection and instance segmentation tasks.

## Features

- 🖱️ **Interactive polygon drawing** - Point-and-click interface with drag-and-drop editing
- 🎨 **Multi-polygon support** - Draw multiple polygons per image with different classes
- 🔍 **Image zoom controls** - Zoom in/out for detailed annotation work
- 💾 **YOLO format export** - One-click export to `.txt` files with normalized coordinates
- 🎯 **Precision editing** - Proximity-based point and segment selection
- ⌨️ **Keyboard shortcuts** - `Enter` to finish polygon, `Esc` to cancel
- 🖼️ **Image format support** - PNG, JPG, BMP

## Tech Stack

- **Language:** C++17
- **Framework:** Qt 6.8.0
- **Graphics:** QPainter for custom rendering

## Building from Source

### Prerequisites

- Qt 6.8.0 or later
- C++17 compatible compiler (GCC, Clang, MSVC)
- qmake

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/lstachowicz/PolySeg.git
cd PolySeg

# Build
qmake PolySeg.pro
make

# Run
./PolySeg
```

## Usage

1. **Load Image** - Click "Load" button and select an image
2. **Draw Polygon** - Click to add points around your object
3. **Edit Points** - Drag points to adjust position
   - `Ctrl+Click` on point to delete it
   - `Ctrl+Click` on segment to insert new point
4. **Finish Polygon** - Press `Enter` or double-click
5. **Draw More** - Start a new polygon by clicking
6. **Save** - Export annotations to YOLO format

## YOLO Format

Exported `.txt` files contain one line per polygon:
```
class_id x1 y1 x2 y2 x3 y3 ...
```

All coordinates are normalized to [0, 1] range.

## Roadmap

See [IMPLEMENTATION_PLAN.md](.github/IMPLEMENTATION_PLAN.md) for detailed development roadmap including:

- ✅ Phase 1: Multi-polygon system (COMPLETED)
- 🚧 Phase 2: Project management with folders
- 🚧 Phase 3: Image navigation (previous/next)
- 🚧 Phase 4: YOLO import/export
- 🚧 Phase 5: Polygon selection and editing
- 🚧 Phase 6: Class management UI
- 🚧 Phase 7: AI auto-detection with YOLO11

## License

MIT License - see [LICENSE](LICENSE) for details

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Support

If this tool saves you time, consider supporting the project:
- ⭐ Star the repository
- ☕ [Buy me a coffee](https://ko-fi.com/lstachowicz) (optional)
- 🐛 Report bugs or request features via [Issues](https://github.com/lstachowicz/PolySeg/issues)

## Author

**Lukasz Stachowicz**

Perfect for creating training datasets for computer vision models requiring polygon-based ground truth annotations.
