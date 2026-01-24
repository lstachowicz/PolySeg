# PolySeg - AI-Powered Polygon Segmentation Tool

**Smart Polygon Annotation with Universal AI Plugin Support**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Qt Version](https://img.shields.io/badge/Qt-6.8.0-green.svg)](https://www.qt.io/)

A professional Qt6-based desktop application for creating polygon annotations for computer vision training datasets. Features a **universal AI plugin system** that integrates with any AI detection framework.

---

## Table of Contents

1. [Overview](#overview)
2. [Key Features](#key-features)
3. [Requirements](#requirements)
4. [Building from Source](#building-from-source)
5. [Running the Application](#running-the-application)
6. [Application Settings](#application-settings)
7. [AI Plugin System](#ai-plugin-system)
8. [Batch Detection Workflow](#batch-detection-workflow)
9. [Available Plugins](#available-plugins)
10. [Usage Guide](#usage-guide)
11. [Keyboard Shortcuts](#keyboard-shortcuts)
12. [Annotation Format](#annotation-format)
13. [Troubleshooting](#troubleshooting)
14. [License](#license)
15. [Contributing](#contributing)

---

## Overview

PolySeg is designed for ML engineers, data scientists, and researchers who need to create high-quality polygon annotations for training computer vision models. The application combines efficient manual annotation tools with AI-assisted automation.

### What PolySeg Does

- **Manual Annotation**: Draw precise polygon masks around objects in images
- **AI-Assisted Annotation**: Use any AI model to auto-detect objects, then review and refine
- **Dataset Management**: Organize images with train/val/test splits
- **Quality Control**: Two-tier review system ensures only verified annotations enter training

### Who It's For

- Machine Learning Engineers building object detection/segmentation models
- Data Scientists preparing training datasets
- Research teams needing annotated ground truth data
- Anyone creating polygon-based annotations for computer vision

---

## Key Features

- **Interactive polygon drawing** - Point-and-click with drag-and-drop editing
- **Universal AI Plugin System** - Integrate ANY AI detection framework or custom model
- **Cloud Storage Integration** - One-click project creation in OneDrive, Google Drive, Dropbox, etc.
- **Train/Val/Test Split Management** - Automatic split assignment with model version tracking
- **Undo/Redo System** - Full history with Ctrl+Z/Ctrl+Y support (50 levels)
- **Copy/Paste Polygons** - Copy annotations between images with Ctrl+C/Ctrl+V
- **Class Navigation** - Quick switching with Tab/Shift+Tab and number keys 1-9
- **Customizable Shortcuts** - Edit any keyboard shortcut to match your workflow
- **Multi-polygon & classes** - Unlimited polygons per image with color-coded classes
- **Project management** - Organized `.polyseg` projects with auto-save
- **Zoom & navigation** - Zoom controls + keyboard shortcuts for image navigation
- **Export formats** - Normalized segmentation format, bounding boxes, COCO JSON
- **Selection & editing** - Click to select, drag points, delete polygons
- **Image formats** - PNG, JPG, BMP, TIFF support

---

## Requirements

### System Requirements

| Component | Requirement |
|-----------|-------------|
| Operating System | Linux, Windows, macOS |
| RAM | 4 GB minimum, 8 GB recommended |
| Disk Space | 100 MB for application + space for images |
| Display | 1280x720 minimum resolution |

### Build Requirements

- **Qt Framework**: 6.8.0 or later
- **Compiler**: C++17 compatible (GCC, Clang, MSVC)
- **Build System**: qmake
- **Optional**: clang-format and clang-tidy (for development)

### AI Plugin Requirements (Optional)

To use AI-assisted annotation, you need:

- **Python**: 3.8 or later
- **AI Framework**: PyTorch, TensorFlow, ONNX Runtime, or other
- **Environment**: Native Python, virtualenv, conda, or Docker

---

## Building from Source

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/lstachowicz/PolySeg.git
cd PolySeg

# Build with qmake
qmake PolySeg.pro
make

# Run the application
./PolySeg
```

### Development Tools

Format code using clang-format:
```bash
clang-format -i src/*.cpp src/*.h
```

Run static analysis with clang-tidy:
```bash
clang-tidy src/*.cpp -- -I/path/to/qt/include
```

---

## Running the Application

### First Launch

1. Run `./PolySeg` (Linux/macOS) or `PolySeg.exe` (Windows)
2. Create a new project: **File → New Project**
3. Choose project location and name
4. Copy images to the `images/` folder within your project

### Project Structure

A PolySeg project has the following structure:

```
MyProject/
├── MyProject.polyseg    # Project configuration (JSON)
├── images/              # Source images (JPG, PNG, BMP, TIFF)
├── labels/              # Annotation files
│   ├── image_001.txt    # Approved annotations (for training)
│   └── image_002.meta   # Pending AI detections (for review)
├── splits/              # Train/val/test lists (if enabled)
│   ├── train.txt
│   ├── val.txt
│   └── test.txt
└── models/              # Trained model files
```

---

## Application Settings

Access settings via **Tools → Project Settings**. Settings are organized into four tabs.

### Project Settings Tab

#### Basic Settings

| Setting | Description |
|---------|-------------|
| **Project Name** | Display name for your project |
| **Annotation Type** | Polygon (segmentation) or Bounding Box (detection) |

#### Auto-Save Settings

| Setting | Description | Default |
|---------|-------------|---------|
| **Enable auto-save** | Automatically save annotations while working | Enabled |
| **Auto-save interval** | Seconds between saves (10-300) | 30 seconds |

#### Annotation Classes

Define object classes with unique colors:
- Add/Edit/Remove classes
- Reorder with up/down arrows (affects keyboard shortcuts 1-9)
- First 9 classes accessible via number keys

#### Custom Folder Paths

| Folder | Default | Description |
|--------|---------|-------------|
| **Images Folder** | `images/` | Source images location |
| **Labels Folder** | `labels/` | Annotation output location |

---

### Import/Export Settings Tab

#### Image Crop Configuration

Automatically crop images during loading to remove unwanted areas.

| Setting | Description |
|---------|-------------|
| **Enable cropping** | Apply crop to all loaded images |
| **X (left)** | Left edge position in pixels |
| **Y (top)** | Top edge position in pixels |
| **Width** | Crop width (0 = full width) |
| **Height** | Crop height (0 = full height) |

**Use cases**: Remove camera timestamps, UI overlays, or watermarks.

#### Import Path Configuration

Generate meaningful filename prefixes based on source directory structure.

**Settings:**
- **Base Path to Strip**: Root directory to remove from file paths
- **Folders to Skip**: List of folder names to exclude from prefix

**Example:**
```
Source: /data/Projects/BMP/RawData/Location_A/photo.jpg
Base Path: /data/Projects
Skip Folders: BMP, RawData
Result: Location_A_photo.jpg
```

**More Examples:**

| Use Case | Base Path | Skip Folders | Result Pattern |
|----------|-----------|--------------|----------------|
| Simple | `/home/user/Images` | (none) | `subfolder_image.jpg` |
| Medical | `/mnt/Medical_Scans` | `Raw_Data` | `Patient_001_CT_slice.jpg` |
| Multi-Camera | `/recordings/2024` | `raw_footage` | `Camera_Front_frame.jpg` |
| Time-Series | `/timelapse/Project` | `backup` | `2024_Q1_January_site.jpg` |

#### Supported Image Extensions

Default: `jpg, jpeg, png, bmp, tiff`

Custom examples: `dcm, nii, nrrd` (medical), `raw, cr2, nef` (raw), `webp, avif` (web)

#### Export Settings

| Setting | Options | Description |
|---------|---------|-------------|
| **Export Format** | Segmentation, Bounding Box, COCO JSON | Output format |
| **Normalize coordinates** | Yes/No | Convert to 0.0-1.0 range |
| **Coordinate Precision** | 0-10 | Decimal places (default: 6) |

---

### AI/Model Settings Tab

#### AI Plugin Configuration

| Field | Description | Example |
|-------|-------------|---------|
| **Enable AI Plugin** | Activate plugin functionality | Checked |
| **Plugin Name** | Display name | "AI Plugin Segmentation" |
| **Env Setup** | Environment activation (optional) | `source venv/bin/activate` |
| **Command** | Program to execute | `python3`, `docker`, `bash` |
| **Script Path** | Path to plugin script | `./plugins/model_plugin.py` |
| **Detect Args** | Detection arguments | `detect --image {image} --model {model}` |
| **Train Args** | Training arguments (optional) | `train --data {project} --epochs {epochs}` |

#### Plugin Settings (Key-Value)

Custom settings available as `{key}` variables:

| Key | Example Value | Usage |
|-----|---------------|-------|
| `model` | `./models/best.pt` | `{model}` in args |
| `confidence` | `0.25` | `{confidence}` in args |
| `device` | `cuda` | `{device}` in args |
| `epochs` | `100` | `{epochs}` in args |

#### Dataset Splits (Train/Val/Test)

Automatic splitting of images for proper ML workflows.

| Setting | Description | Default |
|---------|-------------|---------|
| **Enable splits** | Activate split assignment | Disabled |
| **Train ratio** | Training data percentage | 70% |
| **Val ratio** | Validation data percentage | 20% |
| **Test ratio** | Test data percentage | 10% |
| **Random Seed** | UUID for deterministic assignment | Auto-generated |

**How It Works:**
1. Each image filename is hashed (MD5) with the seed
2. Hash determines split assignment (deterministic)
3. New images are assigned to maintain target ratios
4. Existing assignments never change (immutable)

**Live Statistics Display:**
```
Target: 70/20/10% | Actual: 71/19/10% (T:142 V:38 Te:20)
```

**Reset Splits:**
- Archives old models to `models_old_TIMESTAMP/`
- Generates new random seed
- Reassigns all images

#### Model Versions

Track trained model versions with metadata:

| Column | Description |
|--------|-------------|
| **Name** | Model identifier (e.g., "v1.0_baseline") |
| **Date** | Training completion date |
| **Images Count** | Number of training images |
| **Path** | Model file location |
| **Notes** | Hyperparameters, metrics, etc. |

**Actions**: Add Model, Edit Notes, Compare Models, Remove Entry

---

### Shortcuts Settings Tab

Configure keyboard shortcuts for all actions. Default shortcuts can be customized to match your workflow.

---

## AI Plugin System

### Architecture Overview

PolySeg uses a **universal plugin architecture** that works with ANY AI framework:

```
PolySeg (C++/Qt) ←→ subprocess ←→ Plugin (Python/other)
                     JSON
```

**Key Features:**
- **Framework agnostic**: Works with PyTorch, TensorFlow, ONNX, etc.
- **Zero dependencies**: PolySeg does NOT link to AI libraries
- **Simple protocol**: JSON-based stdin/stdout communication
- **Flexible deployment**: Native Python, Docker, virtualenv, conda

### Plugin Configuration Examples

#### Native Python

```
Command: python3
Script Path: ./plugins/model_plugin.py
Detect Args: detect --image {image} --model {model} --conf {confidence}
```

#### Virtual Environment

```
Command: bash
Script Path: -c "source venv/bin/activate && python ./plugins/model_plugin.py"
Detect Args: detect --image {image} --model {model}
```

#### Conda Environment

```
Command: conda
Script Path: run -n myenv python ./plugins/model_plugin.py
Detect Args: detect --image {image}
```

#### Docker Container

```
Command: docker
Script Path: exec -i ai_container python /app/model_plugin.py
Detect Args: detect --image /app/data/{image_name} --model {model}
```

### Communication Format

#### Input (Command Line)

```bash
python3 ./plugins/model_plugin.py detect \
  --image /path/to/image.jpg \
  --model ./models/model.pt \
  --confidence 0.25
```

#### Output (JSON to stdout)

**Success:**
```json
{
  "status": "success",
  "detections": [
    {
      "class": "person",
      "confidence": 0.89,
      "points": [
        [0.1, 0.2],
        [0.15, 0.18],
        [0.2, 0.22]
      ]
    }
  ]
}
```

**Coordinates are normalized (0.0 - 1.0):**
- `x = pixel_x / image_width`
- `y = pixel_y / image_height`

**Error:**
```json
{
  "status": "error",
  "message": "Model not found: model.pt"
}
```

### Variable Substitution

PolySeg automatically substitutes variables in `{braces}`:

| Variable | Description | Example |
|----------|-------------|---------|
| `{image}` | Current image path | `/path/to/project/images/photo.jpg` |
| `{project}` | Project directory | `/path/to/project` |
| `{splits}` | Splits directory | `/path/to/project/splits` |
| `{train_count}` | Training images count | `142` |
| `{val_count}` | Validation images count | `45` |
| `{test_count}` | Test images count | `21` |
| `{key}` | Any custom setting | Value from Plugin Settings |

### Deployment Quick Reference

| Environment | Command | Script Path |
|-------------|---------|-------------|
| Native Python | `python3` | `./plugins/script.py` |
| Docker | `docker` | `exec -i container python /app/script.py` |
| Virtual env | `bash` | `-c "source venv/bin/activate && python ./plugins/script.py"` |
| Conda | `conda` | `run -n env_name python ./plugins/script.py` |
| Poetry | `poetry` | `run python ./plugins/script.py` |
| Pipenv | `pipenv` | `run python ./plugins/script.py` |

---

## Batch Detection Workflow

Process hundreds of images automatically with AI, then review and approve.

### The .meta File System

PolySeg uses a two-tier system to ensure data quality:

```
AI Detection → .meta (pending review) → .txt (approved for training)
```

| File | Status | Used for Training | Editable |
|------|--------|-------------------|----------|
| `.txt` | Approved | Yes | Yes |
| `.meta` | Pending | No | Yes (before approve) |
| (none) | Not annotated | No | Yes (manual) |

### Quick Start

**Step 1: Setup (One Time)**

```
Tools → Configure Plugin
```

Configure your AI plugin (see AI Plugin System section).

**Step 2: Run Batch Detection**

```
Tools → Batch Detect All (Ctrl+Shift+D)
```

- AI processes every image in `project/images/`
- Results saved to `.meta` files
- Images auto-assigned to train/val/test splits (if enabled)
- Skips images with existing `.txt` (already approved)

**Step 3: Review Detections**

```
Tools → Review → Next Unreviewed (Ctrl+U)
```

**Review Loop:**

1. **Load**: Image with AI detections loads automatically
2. **Inspect**: Check polygon accuracy and class labels
3. **Edit**: Drag points, delete incorrect polygons, add missing ones
4. **Decide**:
   - **Approve** (Ctrl+Enter): Saves to `.txt`, ready for training
   - **Reject** (Ctrl+Backspace): Deletes `.meta`, clears canvas
5. **Repeat**: Continue until "All images reviewed!"

### Time Savings Example

**500 images to annotate:**

| Method | Time per Image | Total Time |
|--------|---------------|------------|
| Manual annotation | 2-5 minutes | 16-40 hours |
| Batch + Review | 20-30 seconds | 2.5-4 hours |

**Time saved: 80-90%**

### Best Practices

**DO:**
- Review in batches (50-100 images per session)
- Edit detections before approving
- Use high confidence (0.7+) for first pass
- Approve quickly when detections are good

**DON'T:**
- Blindly approve all detections
- Use `.meta` files for training
- Skip the review process

---

## Available Plugins

### Plugin Comparison

| Plugin | License | Speed | Accuracy | Best For |
|--------|---------|-------|----------|----------|
| **Detectron2** | Apache 2.0 | Slow | Very High | Research, highest accuracy |
| **SMP** | MIT | Medium | High | Flexible experimentation |

### Detectron2 Plugin

**License:** Apache 2.0

**Installation:**

Create a virtual environment in your project's `plugin/` directory:

```bash
# Navigate to your project's plugin directory
cd /path/to/your/project/plugin

# Create virtual environment
python3 -m venv detectron2_venv

# Activate the environment
source detectron2_venv/bin/activate

# Install dependencies from requirements file
# IMPORTANT: Use --no-build-isolation to prevent detectron2 from creating its own venv
pip install -r /path/to/PolySeg/examples/plugins/requirements/detectron2.txt --no-build-isolation

# Deactivate when done
deactivate
```

**Copy the plugin script:**
```bash
cp /path/to/PolySeg/examples/plugins/detectron2_plugin.py /path/to/your/project/plugin/
```

**Configuration in PolySeg:**
```
Enable AI Plugin: ✓
Plugin Name: Detectron2 Mask R-CNN
Env Setup: source plugin/detectron2_venv/bin/activate
Command: python3
Script Path: plugin/detectron2_plugin.py
Detect Args: detect --image {image} --config {config} --model {model} --conf {confidence}

Settings:
  config: COCO-InstanceSegmentation/mask_rcnn_R_50_FPN_3x.yaml
  model: models/my_model/model_final.pth
  confidence: 0.5
```

**Available Models:**
- `mask_rcnn_R_50_FPN_3x` - ResNet-50 backbone
- `mask_rcnn_R_101_FPN_3x` - ResNet-101 (more accurate)
- `pointrend_rcnn_R_50_FPN_3x_coco` - Better boundary quality
- `cascade_mask_rcnn_R_50_FPN_3x` - Highest accuracy

### SMP Plugin (Segmentation Models PyTorch)

**License:** MIT

**Installation:**

Create a virtual environment in your project's `plugin/` directory:

```bash
# Navigate to your project's plugin directory
cd /path/to/your/project/plugin

# Create virtual environment
python3 -m venv smp_venv

# Activate the environment
source smp_venv/bin/activate

# Install dependencies from requirements file
pip install -r /path/to/PolySeg/examples/plugins/requirements/smp.txt

# Deactivate when done
deactivate
```

**Copy the plugin script:**
```bash
cp /path/to/PolySeg/examples/plugins/smp_plugin.py /path/to/your/project/plugin/
```

**Configuration in PolySeg:**
```
Enable AI Plugin: ✓
Plugin Name: SMP Segmentation
Env Setup: source plugin/smp_venv/bin/activate
Command: python3
Script Path: plugin/smp_plugin.py
Detect Args: detect --image {image} --architecture {architecture} --encoder {encoder} --weights {model} --conf {confidence} --classes {classes}
Train Args: train --dataset {project}/data.yaml --output {model} --architecture {architecture} --encoder {encoder} --epochs {epochs} --batch-size {batch_size} --lr {lr} --img-size {img_size} --classes {classes}

Settings:
  architecture: Unet
  encoder: resnet34
  model: models/best.pt
  confidence: 0.5
  classes: 1
  epochs: 50
  batch_size: 4
  lr: 0.0001
  img_size: 512
```

**Architectures:** Unet, UnetPlusPlus, MAnet, Linknet, FPN, PSPNet, DeepLabV3, DeepLabV3Plus, PAN

**Encoders:** resnet18, resnet34, resnet50, resnet101, efficientnet-b0 to b7, mobilenet_v2, timm-mobilenetv3_large_100

### Creating Custom Plugins

Any plugin must:

1. **Accept command-line arguments** for image path, model, etc.
2. **Output JSON to stdout** with detections
3. **Use normalized coordinates** (0.0 - 1.0)
4. **Handle errors gracefully**

**Minimal Example:**
```python
#!/usr/bin/env python3
import json
import sys

def detect(image_path):
    # Your AI detection logic here
    detections = [
        {
            "class": "person",
            "confidence": 0.89,
            "points": [[0.1, 0.2], [0.15, 0.18], [0.2, 0.22]]
        }
    ]

    print(json.dumps({
        "status": "success",
        "detections": detections
    }))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(json.dumps({"status": "error", "message": "No image provided"}))
        sys.exit(1)
    detect(sys.argv[1])
```

**Supported Environments:**
- Native Python, Docker, virtualenv, conda
- Bash scripts, Node.js, compiled binaries
- Any executable that outputs JSON

---

## Usage Guide

### Manual Annotation

1. **Create Project**: File → New Project
2. **Add Images**: Copy images to `YourProject/images/`
3. **Select Class**: Choose from dropdown or create new (Ctrl+M)
4. **Draw Polygon**: Click to add points around object
5. **Edit Points**: Drag to adjust, Ctrl+Click to insert/delete
6. **Finish**: Press `Enter` or double-click
7. **Navigate**: Ctrl+Right/Left for next/previous image
8. **Save**: Auto-saves on navigation, or Ctrl+S

### AI-Assisted Annotation

**Single Image:**
1. Configure Plugin (Tools → Configure Plugin) - one-time setup
2. Load any image
3. Run detection (Ctrl+D or Tools → Auto Detect)
4. Review and edit AI-generated polygons
5. Save (auto or Ctrl+S)

**Batch Processing:**
1. Configure Plugin - one-time setup
2. Run batch detection (Ctrl+Shift+D)
3. Review loop: Ctrl+U → Edit → Ctrl+Enter (approve) or Ctrl+Backspace (reject)
4. Repeat until all reviewed
5. Train model on approved `.txt` files only

---

## Keyboard Shortcuts

### Navigation

| Shortcut | Action |
|----------|--------|
| `Right` | Next image |
| `Left` | Previous image |
| `Home` | First image |
| `End` | Last image |

### Annotation

| Shortcut | Action |
|----------|--------|
| `Enter` | Finish polygon |
| `Esc` | Cancel / Deselect |
| `Del` | Delete selected |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+C` | Copy selected polygon |
| `Ctrl+V` | Paste polygon |

### Classes

| Shortcut | Action |
|----------|--------|
| `Tab` | Next class |
| `Shift+Tab` | Previous class |
| `1-9` | Quick select class |
| `Ctrl+M` | Manage classes |

### AI Plugin

| Shortcut | Action |
|----------|--------|
| `Ctrl+D` | AI auto-detect (single image) |
| `Ctrl+Shift+D` | Batch detect all images |
| `Ctrl+U` | Next unreviewed image |
| `Ctrl+Enter` | Approve & save annotations |
| `Ctrl+Backspace` | Reject & clear AI detections |

### Project

| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | New project |
| `Ctrl+S` | Save annotations |

---

## Annotation Format

Exported `.txt` files (in `labels/` folder):

```
class_id x1 y1 x2 y2 x3 y3 ...
```

All coordinates normalized to `[0.0, 1.0]` range.

**Example:**
```
0 0.123456 0.234567 0.345678 0.456789 0.234567 0.567890
1 0.500000 0.300000 0.550000 0.280000 0.600000 0.320000
```

---

## Troubleshooting

### Settings Not Saving

**Check:**
- File permissions on `.polyseg` file
- Disk space available
- Project file not read-only

**Solution:** Close and reopen project

### AI Plugin Not Working

**Check:**
- Plugin enabled checkbox is checked
- Command path is correct (`python3` vs `python`)
- Script path is valid (relative to project dir)
- Plugin settings contain required values

**Debug:** Run plugin command manually in terminal:
```bash
python3 ./plugins/model_plugin.py detect --image test.jpg --model model.pt --conf 0.25
```

### No Detections from AI

**Check:**
- Confidence threshold not too high (try 0.15)
- Model matches your object classes
- Image loading correctly

### Split Statistics Showing 0/0/0

**Cause:** No images in project yet

**Solution:** Copy images to `images/` folder, then reopen settings

### Batch Detect Skips All Images

**Cause:** All images already have `.txt` files

**Solution:** Delete `.txt` files to re-detect

### CUDA Out of Memory

**Solutions:**
- Reduce batch size: `--batch 8` or `--batch 4`
- Use smaller model
- Use CPU: `--device cpu`

### Docker Container Issues

**Check:**
- Volume mounts are correct
- Container is running (`docker ps`)
- GPU access with `--gpus all`

---

## License

**PolySeg Application Code:** MIT License - see [LICENSE](LICENSE) for details

**Third-Party Components:** See [THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md) and [NOTICE](NOTICE) for Qt Framework and other dependencies

### Plugin Licensing

PolySeg does NOT impose licensing on plugins. Your plugins can use any license you choose.

**Note:** Some AI frameworks have restrictive licenses:
- AGPL-3.0 (YOLOv11): Keep plugin code separate from PolySeg core
- GPL-3.0: May require separate repositories

MIT and Apache 2.0 licensed plugins can be freely integrated.

---

## Dependencies

This application is built with:

- **Qt Framework 6.8.0** - LGPL v3 License ([https://www.qt.io/](https://www.qt.io/))
  - Qt is dynamically linked, allowing users to replace libraries under LGPL v3 terms

---

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

**Areas for contribution:**
- New plugin implementations
- Documentation improvements
- Bug fixes
- Feature requests

---

## Support

- Report bugs or request features via [GitHub Issues](https://github.com/lstachowicz/PolySeg/issues)
- Star the repository if this tool helps you

---

## Author

**Lukasz Stachowicz**

Perfect for creating training datasets for computer vision models requiring polygon-based ground truth annotations.
