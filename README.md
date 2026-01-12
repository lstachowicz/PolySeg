# PolySeg - AI-Powered Polygon Segmentation Tool

**Smart Polygon Annotation with Universal AI Plugin Support**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Qt Version](https://img.shields.io/badge/Qt-6.8.0-green.svg)](https://www.qt.io/)

A professional Qt6-based desktop application for creating polygon annotations with **universal AI plugin support**. Integrate your own AI models for automatic detection!

##  Key Features

- 🖱️ **Interactive polygon drawing** - Point-and-click with drag-and-drop editing
-  **Universal AI Plugin System** - Integrate ANY AI detection framework or custom model
- 🎨 **Multi-polygon & classes** - Unlimited polygons per image with color-coded classes
- 📁 **Project management** - Organized `.polyseg` projects with auto-save
- 🔍 **Zoom & navigation** - Zoom controls + keyboard shortcuts for image navigation
- 💾 **Export formats** - Normalized segmentation format, bounding boxes, COCO JSON
- ✂️ **Selection & editing** - Click to select, drag points, delete polygons
- ⌨️ **Keyboard shortcuts** - Fast workflow (Ctrl+D for AI detection, Enter to finish polygon)
- 🖼️ **Image formats** - PNG, JPG, BMP support

##  AI Plugin System

PolySeg uses a **universal plugin architecture** that works with ANY AI tool or framework!

### Features

-  **Framework agnostic** - Works with any detection model (PyTorch, TensorFlow, ONNX, etc.)
-  **Flexible deployment** - Native Python, Docker, virtual environments
-  **Batch processing** - Process hundreds of images automatically
-  **Quality control** - Manual review system prevents bad training data
-  **Simple integration** - JSON-based subprocess communication

### Quick Start

**Example: AI Model in Docker**
```bash
# Configure in PolySeg (Tools → Configure Plugin)
Command: docker
Script: exec -i ai_container python /app/model_plugin.py
Detect Args: detect --image {image} --model {model} --conf {confidence}

Settings:
  - model: /app/models/model.pt
  - confidence: 0.25
```

**Example: Python with Virtual Environment**
```bash
# Configure in PolySeg
Command: bash
Script: -c "source venv/bin/activate && python ./plugins/model_plugin.py"
Detect Args: detect --image {image} --model {model}

Settings:
  - model: ./models/model.pt
  - confidence: 0.25
```

**Example: Conda Environment**
```bash
# Configure in PolySeg
Command: conda
Script: run -n myenv python ./plugins/custom_plugin.py
Detect Args: detect --image {image}
```

### Workflow

1. **Configure** - Tools → Configure Plugin (one-time setup)
2. **Detect** - Ctrl+D (single) or Ctrl+Shift+D (batch all images)
3. **Review** - Ctrl+U to navigate, edit AI results
4. **Approve** - Ctrl+Enter (or Reject with Ctrl+Backspace)
5. **Train** - Use only approved annotations for model training

**📖 Complete guide:** See [docs/PLUGIN_ARCHITECTURE.md](docs/PLUGIN_ARCHITECTURE.md) for detailed documentation, plugin development, and examples.

## Tech Stack

- **Language:** C++17
- **Framework:** Qt 6.8.0
- **Graphics:** QPainter for custom rendering

## Building from Source

### Prerequisites

- Qt 6.8.0 or later
- C++17 compatible compiler (GCC, Clang, MSVC)
- qmake
- clang-format and clang-tidy (optional, for development)

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

### Development Tools

Format code using clang-format:
```bash
clang-format -i *.cpp *.h
```

Run static analysis with clang-tidy:
```bash
clang-tidy *.cpp -- -I/path/to/qt/include
```

## Usage

### Manual Annotation

1. **Create Project** - File → New Project
2. **Add Images** - Copy images to `YourProject/images/`
3. **Select Class** - Choose from dropdown or create new (Ctrl+M)
4. **Draw Polygon** - Click to add points around object
5. **Edit Points** - Drag to adjust, Ctrl+Click to insert/delete
6. **Finish** - Press `Enter` or double-click
7. **Navigate** - Ctrl+Right/Left for next/previous image
8. **Save** - Auto-saves on navigation, or Ctrl+S

### AI-Assisted Annotation

**Single Image:**
1. **Configure Plugin** - Tools → Configure Plugin (one-time setup)
2. **Load Image** - Open any image in project
3. **Run Detection** - Ctrl+D or Tools → Auto Detect
4. **Review Results** - AI generates polygons automatically
5. **Edit as Needed** - Adjust, delete, or add more polygons
6. **Save** - Auto-saves when navigating to next image

**Batch Detection (Multiple Images):** 
1. **Open Project** - with multiple images
2. **Configure Plugin** - one-time setup
3. **Batch Detect** - Ctrl+Shift+D or Tools → Batch Detect All
   - AI processes all images automatically
   - Results saved to `.meta` files (pending review)
   - Skips already approved images
4. **Review Loop:**
   - Press **Ctrl+U** (Next Unreviewed)
   - Review AI detections, edit if needed
   - **Approve** (Ctrl+Enter) → saves to `.txt` (ready for training)
   - **Reject** (Ctrl+Backspace) → clears and deletes `.meta`
5. **Repeat** until all images reviewed
6. **Train Model** - only on approved `.txt` files

**Why .meta files?**
- Prevents unverified AI detections from affecting model training
- Manual review ensures data quality
- Can edit before approving
- Only approved annotations (`.txt`) used for training

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | New project |
| `Ctrl+S` | Save annotations |
| `Ctrl+D` | AI auto-detect (single image) |
| `Ctrl+Shift+D` | Batch detect all images |
| `Ctrl+U` | Next unreviewed image |
| `Ctrl+Enter` | Approve & save annotations |
| `Ctrl+Backspace` | Reject & clear AI detections |
| `Ctrl+Right` | Next image |
| `Ctrl+Left` | Previous image |
| `Home` / `End` | First / Last image |
| `Enter` | Finish polygon |
| `Esc` | Cancel / Deselect |
| `Del` | Delete selected |
| `Ctrl+M` | Manage classes |

## Annotation Format

Exported `.txt` files (in `labels/` folder):
```
class_id x1 y1 x2 y2 x3 y3 ...
```

All coordinates normalized to `[0.0, 1.0]` range.

## Roadmap

See [IMPLEMENTATION_PLAN.md](.github/IMPLEMENTATION_PLAN.md) for detailed roadmap.

**Completed:**
-  Phase 1: Multi-polygon system
-  Phase 2: Project management
-  Phase 3: Image navigation
-  Phase 4: Annotation import/export
-  Phase 5: Selection & editing
-  Phase 7: Universal AI plugin system

**In Progress:**
- 🚧 Phase 6: Class navigation shortcuts
- 🚧 Phase 8: Enhanced menu system
- 🚧 Phase 9: Project settings dialog

**Planned:**
- 📋 Phase 10: Statistics & validation
- 📋 Phase 11: Undo/Redo system
- 📋 Phase 12: Batch processing

## Documentation

- **[docs/PLUGIN_ARCHITECTURE.md](docs/PLUGIN_ARCHITECTURE.md)** - Complete AI plugin development guide
- **[docs/DEPLOYMENT_EXAMPLES.md](docs/DEPLOYMENT_EXAMPLES.md)** - Plugin deployment for Docker, venv, conda, etc.
- **[docs/BATCH_DETECTION_GUIDE.md](docs/BATCH_DETECTION_GUIDE.md)** - Batch processing workflow & best practices
- **[examples/plugins/](examples/plugins/)** - Example plugin implementations
- **[IMPLEMENTATION_PLAN.md](.github/IMPLEMENTATION_PLAN.md)** - Development roadmap

## Dependencies

This application is built with:

- **Qt Framework 6.8.0** - LGPL v3 License ([https://www.qt.io/](https://www.qt.io/))
  - Qt is dynamically linked, allowing users to replace libraries under LGPL v3 terms
  - See [THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md) for complete information

## License

**PolySeg Application Code:** MIT License - see [LICENSE](LICENSE) for details

**Third-Party Components:** See [THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md) and [NOTICE](NOTICE) for Qt Framework and other dependencies

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
