# PolySeg Plugin Architecture

## Overview

PolySeg uses a **universal plugin architecture** that allows integration with ANY AI tool for detection and model training.

You can use:
- **Any detection framework** (object detection, instance segmentation, semantic segmentation)
- **Custom models** in PyTorch, TensorFlow, ONNX, or any other framework
- **Any programming language** - Python, Node.js, Bash, compiled binaries
- **Any deployment method** - Docker containers, virtual environments, native execution

## How it works

### 1. **Process Separation**
- PolySeg (C++/Qt) runs as the main application
- Plugin (Python/other) runs as a separate process
- Communication via **stdin/stdout + JSON**

### 2. **Zero Dependencies**
- PolySeg does **NOT** link to AI libraries
- Plugin installed **optionally** by user
- Clean MIT license for PolySeg core

### 3. **Universal API**
Plugin must support two modes:
- `detect` - object detection on image
- `train` - model training (optional)

---

## Communication Format

### Input

Plugin is called with arguments:

```bash
<command> <script_path> <args...>
```

**Example (Native Python):**
```bash
python3 ./plugins/model_plugin.py detect \
  --image /path/to/image.jpg \
  --model ./models/model.pt \
  --confidence 0.25
```

**Example (Docker Container):**
```bash
docker exec -i ai_container python /app/model_plugin.py detect \
  --image /app/data/image.jpg \
  --model /app/models/model.pt \
  --confidence 0.25
```

**Example (Python Virtual Environment):**
```bash
bash -c "source venv/bin/activate && python ./plugins/model_plugin.py detect \
  --image /path/to/image.jpg \
  --model ./models/model.pt"
```

**Example (Conda Environment):**
```bash
conda run -n myenv python ./plugins/custom_plugin.py detect \
  --image /path/to/image.jpg \
  --model ./models/model.pt
```

### Output

Plugin **MUST** return JSON to stdout:

```json
{
  "status": "success",
  "detections": [
    {
      "class": "person",
      "confidence": 0.89,
      "points": [
        [0.1, 0.2],   // x, y normalized (0.0-1.0)
        [0.15, 0.18],
        [0.2, 0.22],
        ...
      ]
    },
    {
      "class": "car",
      "confidence": 0.95,
      "points": [[0.5, 0.3], [0.55, 0.28], ...]
    }
  ]
}
```

**Normalized coordinates:**
- `x = pixel_x / image_width`
- `y = pixel_y / image_height`
- Range: `0.0 - 1.0`

**Errors:**
```json
{
  "status": "error",
  "message": "Model not found: model.pt"
}
```

---

## Plugin Configuration in Project

Each PolySeg project (`.polyseg`) stores plugin configuration in JSON:

```json
{
  "version": "1.0",
  "name": "MyDataset",
  "classes": [...],
  "plugin": {
    "enabled": true,
    "name": "AI Model Detector",
    "command": "python3",
    "script_path": "./plugins/model_plugin.py",
    "detect_args": "detect --image {image} --model {model} --conf {confidence}",
    "train_args": "train --data {project}/dataset.yaml --epochs {epochs}",
    "settings": {
      "model": "./models/model.pt",
      "confidence": "0.25",
      "epochs": "100",
      "device": "cuda"
    }
  }
}
```

### Variable Substitution

PolySeg automatically substitutes variables in `{braces}`:

| Variable | Description | Example |
|----------|-------------|---------|
| `{image}` | Path to current image | `/path/to/project/images/photo.jpg` |
| `{project}` | Path to project directory | `/path/to/project` |
| `{model}` | From `settings.model` | `./models/model.pt` |
| `{confidence}` | From `settings.confidence` | `0.25` |
| `{epochs}` | From `settings.epochs` | `100` |
| **Any key** | From `settings.*` | Custom settings |

---

## GUI - Plugin Configuration

In PolySeg: **Tools → Configure Plugin...**

### Dialog sections:

1. **Plugin Information**
   - `Plugin Name` - display name (e.g., "My AI Model", "Custom Detector")

2. **Command Configuration**
   - `Command` - program to run (e.g., `python3`, `bash`, `docker`)
   - `Script Path` - path to plugin script (relative or absolute)
   - `Detect Args` - arguments for detection
   - `Train Args` - arguments for training (optional)

3. **Plugin Settings**
   - Key-value pairs for any settings
   - Use **Add Setting** to add new ones
   - All available as `{key}` in args

### Example Configuration: Native Python

```
Plugin Name: AI Model Detection
Command: python3
Script Path: ./plugins/model_plugin.py
Detect Args: detect --image {image} --model {model} --conf {confidence}
Train Args: train --data {project}/dataset.yaml --epochs {epochs} --device {device}

Settings:
  model: ./models/model.pt
  confidence: 0.25
  epochs: 100
  device: cuda
```

### Example Configuration: Docker Container

```
Plugin Name: AI Docker
Command: docker
Script Path: exec -i ai_container python /app/model_plugin.py
Detect Args: detect --image /app/data/{image_name} --model {model} --conf {confidence}
Train Args: train --data /app/data/{project_name}/dataset.yaml --epochs {epochs}

Settings:
  model: /app/models/model.pt
  confidence: 0.25
  epochs: 100
  image_name: current.jpg  # Copy image to Docker volume first
  project_name: project
```

**Note:** For Docker, you may need to mount project directory:
```bash
docker run -v /path/to/project:/app/data ai_image
```

### Example: Python Virtual Environment

```
Plugin Name: AI Model with venv
Command: bash
Script Path: -c "source venv/bin/activate && python ./plugins/model_plugin.py"
Detect Args: detect --image {image} --model {model} --conf {confidence}
Train Args: train --data {project}/dataset.yaml --epochs {epochs}

Settings:
  model: ./models/model.pt
  confidence: 0.25
  epochs: 100
```

### Example: Conda Environment

```
Plugin Name: Custom Model (Conda)
Command: conda
Script Path: run -n myenv python ./plugins/custom_plugin.py
Detect Args: detect --image {image} --checkpoint {checkpoint}
Train Args: train --data {project} --lr {learning_rate}

Settings:
  checkpoint: ./models/best.pth
  learning_rate: 0.001
```

### Example Configuration: Alternative Model

```
Plugin Name: Auto-Segmentation Model
Command: python3
Script Path: ./plugins/model_plugin.py
Detect Args: auto_segment --image {image} --checkpoint {checkpoint} --points {points}

Settings:
  checkpoint: ./models/model.pth
  points: auto
```

---

## Example Plugin Implementation

**Note:** Below is a complete example showing how to integrate a specific detection framework. Your plugin can use any framework or custom model - this is just one possible implementation.

**File:** `./plugins/model_plugin.py`

```python
#!/usr/bin/env python3
import argparse
import json
import sys
# Import your framework here (example uses a popular detection library)
from your_framework import YourModel
import cv2

def detect(args):
    """Run detection and return JSON"""
    try:
        # Load model
        model = YourModel(args.model)
        
        # Run inference
        results = model(args.image, conf=float(args.confidence))
        
        detections = []
        
        # Process results
        for result in results:
            if result.masks is None:
                continue
                
            boxes = result.boxes
            masks = result.masks
            
            for i, mask in enumerate(masks.xy):
                cls_id = int(boxes.cls[i])
                conf = float(boxes.conf[i])
                class_name = model.names[cls_id]
                
                # Normalize coordinates
                img_h, img_w = result.orig_shape
                points = [[float(x/img_w), float(y/img_h)] for x, y in mask]
                
                detections.append({
                    "class": class_name,
                    "confidence": conf,
                    "points": points
                })
        
        # Return JSON to stdout
        output = {
            "status": "success",
            "detections": detections
        }
        print(json.dumps(output))
        
    except Exception as e:
        error = {
            "status": "error",
            "message": str(e)
        }
        print(json.dumps(error), file=sys.stderr)
        sys.exit(1)

def train(args):
    """Train model (framework-specific)"""
    try:
        model = YourModel(args.model)
        model.train(
            data=args.data,
            epochs=int(args.epochs),
            device=args.device
        )
        
        output = {
            "status": "success",
            "message": "Training complete"
        }
        print(json.dumps(output))
        
    except Exception as e:
        error = {
            "status": "error",
            "message": str(e)
        }
        print(json.dumps(error), file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest='mode')
    
    # Detect mode
    detect_parser = subparsers.add_parser('detect')
    detect_parser.add_argument('--image', required=True)
    detect_parser.add_argument('--model', required=True)
    detect_parser.add_argument('--conf', default=0.25)
    
    # Train mode
    train_parser = subparsers.add_parser('train')
    train_parser.add_argument('--data', required=True)
    train_parser.add_argument('--model', default='model.pt')
    train_parser.add_argument('--epochs', default=100)
    train_parser.add_argument('--device', default='cuda')
    
    args = parser.parse_args()
    
    if args.mode == 'detect':
        detect(args)
    elif args.mode == 'train':
        train(args)
    else:
        print("Usage: plugin.py {detect|train} [options]")
        sys.exit(1)
```

---

## Alternative Plugin Implementation Example

**Note:** This shows a different approach using an auto-segmentation model. The exact implementation depends on your chosen framework.

**File:** `./plugins/autoseg_plugin.py`

```python
#!/usr/bin/env python3
import argparse
import json
import sys
import cv2
import numpy as np
# Import your auto-segmentation framework
from your_autoseg_framework import AutoSegmentModel

def auto_segment(args):
    """Run automatic segmentation"""
    try:
        # Load model (example - adapt to your framework)
        model = AutoSegmentModel.load(args.checkpoint)
        
        # Load image
        image = cv2.imread(args.image)
        image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        
        # Generate masks
        masks = mask_generator.generate(image)
        
        detections = []
        img_h, img_w = image.shape[:2]
        
        for mask_data in masks:
            # Extract segmentation mask
            mask = mask_data['segmentation']
            
            # Find contours
            contours, _ = cv2.findContours(
                mask.astype(np.uint8), 
                cv2.RETR_EXTERNAL, 
                cv2.CHAIN_APPROX_SIMPLE
            )
            
            if len(contours) == 0:
                continue
                
            # Get largest contour
            contour = max(contours, key=cv2.contourArea)
            
            # Simplify polygon (Douglas-Peucker)
            epsilon = 0.005 * cv2.arcLength(contour, True)
            approx = cv2.approxPolyDP(contour, epsilon, True)
            
            # Normalize coordinates
            points = [
                [float(pt[0][0]/img_w), float(pt[0][1]/img_h)] 
                for pt in approx
            ]
            
            detections.append({
                "class": "object",  # Auto-segmentation - no classification
                "confidence": float(mask_data.get('stability_score', 1.0)),
                "points": points
            })
        
        output = {
            "status": "success",
            "detections": detections
        }
        print(json.dumps(output))
        
    except Exception as e:
        error = {
            "status": "error",
            "message": str(e)
        }
        print(json.dumps(error), file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('mode', choices=['auto_segment'])
    parser.add_argument('--image', required=True)
    parser.add_argument('--checkpoint', required=True)
    parser.add_argument('--points', default='auto')
    
    args = parser.parse_args()
    
    if args.mode == 'auto_segment':
        auto_segment(args)
```

---

## Plugin Installation

### For Users:

1. Create or download a plugin script (e.g., `model_plugin.py`)
2. Place it in `<project>/plugins/`
3. Install framework-specific dependencies:
   ```bash
   # Example for PyTorch-based model
   pip install torch opencv-python numpy
   # Or for TensorFlow
   pip install tensorflow opencv-python
   # Or for ONNX Runtime
   pip install onnxruntime opencv-python
   ```
4. In PolySeg: **Tools → Configure Plugin**
5. Configure:
   - Command: `python3`
   - Script Path: `./plugins/model_plugin.py`
   - Detect Args: `detect --image {image} --model {model} --conf {confidence}`
   - Settings: model, confidence, etc.

### For Developers:

You can create a plugin in **any language**:
- Python + pip install
- Node.js + npm install
- Bash script
- Compiled binary (C++, Rust)

Only requirement: **return JSON to stdout**

---

## User Workflow

### Single Image Detection

1. **Open project** in PolySeg
2. **Configure plugin** (Tools → Configure Plugin)
3. **Load image**
4. **Run detection** (Tools → Auto Detect or `Ctrl+D`)
5. Plugin returns polygons → automatically added to canvas
6. **Edit manually** as needed
7. **Save** (`Ctrl+S`)

### Batch Detection with Review Workflow

**New feature!** Run AI detection on all images with manual verification:

1. **Open project** with multiple images
2. **Configure plugin** (Tools → Configure Plugin)
3. **Run batch detection** (Tools → Batch Detect All or `Ctrl+Shift+D`)
   - AI processes all images in project
   - Results saved to `.meta` files (temporary, unapproved)
   - Skips already approved images (with `.txt`)
4. **Review results** (Tools → Review → Next Unreviewed or `Ctrl+U`)
   - Automatically loads next image with `.meta`
   - Shows AI detections on canvas
5. **Edit/Correct** detections as needed
6. **Approve or Reject:**
   - **Approve** (`Ctrl+Enter`) → `.meta` → `.txt` (ready for training)
   - **Reject** (`Ctrl+Backspace`) → deletes `.meta`, clears canvas
7. **Repeat** for next images
8. **Train model** on approved data

**Benefits:**
- AI does not overwrite existing annotations
- Every detection requires manual verification
- Only approved data (`.txt`) used for training
- Edit before approval
- Automatic skipping of already reviewed images

---

## FAQ

### How does the .meta file system work?

**Problem:** We don't want unverified AI detections to enter model training.

**Solution:** Two-tier file system:

1. **`.meta` files** - temporary AI results (unapproved)
   - Created by batch detection
   - Standard normalized format (same as `.txt` files)
   - Not used for training
   - Can be edited before approval

2. **`.txt` files** - approved annotations (ready for training)
   - Created manually or through Approve
   - Only these files used for model training
   - Guaranteed quality (human-verified)

**Workflow:**
```
Batch Detect → image.meta (AI detections)
    ↓ Review & Edit
    ↓ Approve → image.txt (approved for training)
    ↓ Reject → delete .meta (start over manually)
```

**File structure:**
```
project/
├── images/
│   ├── photo_001.jpg
│   ├── photo_002.jpg
│   └── photo_003.jpg
├── labels/
│   ├── photo_001.txt     Approved (ready for training)
│   ├── photo_002.meta    Pending review
│   └── (photo_003 - no annotations yet)
```

### Do I need a specific framework?
No! You can use ANY AI tool or framework that can output polygon coordinates.

### How to use a custom PyTorch model?
Write a plugin in Python, load model with `torch.load()`, return JSON with detections.

### Can I use cloud APIs (OpenAI, Roboflow)?
Yes! Plugin can send HTTP requests and parse responses to JSON format.

### Can the plugin work without GPU?
Yes, set `device: cpu` in settings. It will be slower but works.

### How to debug a plugin?
Run the plugin manually in terminal:
```bash
python3 ./plugins/model_plugin.py detect --image test.jpg --model model.pt --conf 0.25
```
Check if it returns valid JSON.

---

## Licensing

### PolySeg Core (MIT License)
- Main application (Qt/C++)
- Plugin interface (JSON API)
- **Free commercial use**

### Plugins (Your Choice)
- Your plugin: **You decide the license**
- Framework dependencies inherit their own licenses
- PolySeg core stays MIT regardless

**PolySeg does NOT impose licensing on plugins!**
