# PolySeg Plugin Examples

**Example plugin implementations** demonstrating the universal plugin architecture with specific AI frameworks.

**Note:** These are implementation examples. PolySeg's plugin system works with ANY framework - these just show how to integrate popular ones.

##  **License Notice**

Some AI frameworks have restrictive licenses (GPL, AGPL). Keep them in separate repositories to maintain PolySeg's MIT license.

📦 **GPL-licensed plugins (separate repositories):**
```bash
# Example: Installing a GPL-licensed plugin from external repository
git clone https://github.com/user/polyseg-gpl-plugin
cp polyseg-gpl-plugin/plugin.py examples/plugins/
pip install -r polyseg-gpl-plugin/requirements.txt
```

**Why separate?** GPL-3.0 license requires entire project to be GPL if included. PolySeg core remains MIT by keeping GPL plugins external.

##  Quick Comparison

| Plugin | License | Speed | Accuracy | Best For | Status |
|--------|---------|-------|----------|----------|--------|
| **SAM** | Apache 2.0 |  | ⭐⭐⭐⭐⭐ | Zero-shot, any object | 🔜 Planned |
| **YOLACT** | MIT  |  | ⭐⭐⭐ | Real-time instance seg | 🔜 Planned |
| **Detectron2** | Apache 2.0  |  | ⭐⭐⭐⭐⭐ | High accuracy, research | 🔜 Planned |
| **SMP** | MIT  |  | ⭐⭐⭐⭐ | Custom semantic seg | 🔜 Planned |

## Available Plugins

### 1. Example Plugin Implementation

**Note:** For GPL-licensed frameworks, see separate repositories.  
**MIT/Apache alternatives:** YOLACT, Detectron2, SMP

**Features:**
-  Instance segmentation detection
-  Auto-fallback to bounding boxes if no masks
-  Polygon simplification for complex shapes
-  Model training support
-  Confidence threshold control

**Installation:**
```bash
pip install ultralytics opencv-python
```

**Configuration Examples:**

**A) Native Python:**
```
Plugin Name: AI Model Detection
Command: python3
Script Path: ./examples/plugins/model_plugin.py
Detect Args: detect --image {image} --model {model} --conf {confidence}
Train Args: train --data {project}/dataset.yaml --epochs {epochs} --device {device}

Settings:
  model: ./models/model.pt
  confidence: 0.25
  epochs: 100
  device: cuda
```

**B) Docker Container:**
```
Plugin Name: AI Docker
Command: docker
Script Path: exec -i ai_container python /app/model_plugin.py
Detect Args: detect --image /app/data/{image_name} --model {model}
Train Args: train --data /app/data/dataset.yaml --epochs {epochs}

Settings:
  model: /app/models/model.pt
  confidence: 0.25
  image_name: current.jpg
```

**C) Virtual Environment:**
```
Plugin Name: AI venv
Command: bash
Script Path: -c "source venv/bin/activate && python ./examples/plugins/model_plugin.py"
Detect Args: detect --image {image} --model {model} --conf {confidence}

Settings:
  model: ./models/model.pt
  confidence: 0.25
```

**D) Conda Environment:**
```
Plugin Name: AI Conda
Command: conda
Script Path: run -n ai_env python ./examples/plugins/model_plugin.py
Detect Args: detect --image {image} --model {model} --conf {confidence}

Settings:
  model: ./models/model.pt
  confidence: 0.25
```

**Test manually:**
```bash
# Download model first
wget https://github.com/ultralytics/assets/releases/download/v8.3.0/model.pt

# Run detection
python3 model_plugin.py detect \
  --image /path/to/image.jpg \
  --model model.pt \
  --conf 0.25
```

---

## Creating Your Own Plugin

See `docs/PLUGIN_ARCHITECTURE.md` for complete guide.

### Minimum requirements:

1. **Accept command-line arguments**
   - Use your preferred argument parser
   - Support at least `detect` mode

2. **Output JSON to stdout**
   ```json
   {
     "status": "success",
     "detections": [
       {
         "class": "object_name",
         "confidence": 0.95,
         "points": [[x1, y1], [x2, y2], ...]
       }
     ]
   }
   ```

3. **Use normalized coordinates (0.0 - 1.0)**
   ```python
   x_normalized = pixel_x / image_width
   y_normalized = pixel_y / image_height
   ```

4. **Handle errors gracefully**
   ```json
   {
     "status": "error",
     "message": "Description of what went wrong"
   }
   ```

### Plugin execution environments:
-  Native Python (python3)
-  Docker containers (docker exec)
-  Virtual environments (source venv/bin/activate)
-  Conda environments (conda run -n env)
-  Bash scripts
-  Node.js
-  Compiled binaries (C++, Rust, Go)
-  Any executable that can output JSON

---

## Example: Minimal Plugin (Python)

```python
#!/usr/bin/env python3
import json
import sys

def detect(image_path):
    # Your AI detection logic here
    # ...
    
    detections = [
        {
            "class": "person",
            "confidence": 0.89,
            "points": [[0.1, 0.2], [0.15, 0.18], [0.2, 0.22]]
        }
    ]
    
    output = {
        "status": "success",
        "detections": detections
    }
    print(json.dumps(output))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(json.dumps({"status": "error", "message": "No image provided"}))
        sys.exit(1)
    
    detect(sys.argv[1])
```

---

## FAQ

**Q: Can I use models from Hugging Face?**  
A: Yes! Download the model and configure your plugin to use it.

**Q: Do I need GPU?**  
A: No, but it's faster. Set `device: cpu` in plugin settings.

**Q: Can I use cloud APIs (OpenAI, Roboflow)?**  
A: Yes! Create a plugin that sends HTTP requests and parses responses.

**Q: How do I debug my plugin?**  
A: Run it manually in terminal first:
```bash
python3 my_plugin.py detect --image test.jpg
```

**Q: Can I charge for my plugin?**  
A: Yes! PolySeg doesn't restrict plugin licensing.

---

## Contributing

Have a cool plugin? Submit a PR to add it to the examples!

**Popular requests:**
- SAM (Segment Anything Model)
- Detectron2
- MMDetection
- Custom PyTorch models
- Cloud API integrations
