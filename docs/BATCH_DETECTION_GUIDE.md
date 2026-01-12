# Batch Detection Workflow Guide

## Overview

**Batch Detection** allows you to run AI detection on hundreds of images automatically, while maintaining quality control through manual review.

### Key Concept: .meta Files

PolySeg uses a two-tier file system to ensure data quality:

```
AI Detection → .meta (pending review) → .txt (approved for training)
```

**Why?**
- Prevents unverified AI predictions from corrupting your training dataset
- Allows editing/correction before approval
- Tracks which images need review
- Only human-verified annotations used for model training

---

## Quick Start

### 1. Setup (One Time)

```
Tools → Configure Plugin
```

Configure your AI detection plugin:
- Command: `python3`
- Script: `./plugins/model_plugin.py`
- Detect Args: `detect --image {image} --model {model} --conf {confidence}`
- Settings:
  - model: `./models/model.pt`
  - confidence: `0.25`

### 2. Run Batch Detection

```
Tools → Batch Detect All (Ctrl+Shift+D)
```

**What happens:**
- AI processes every image in `project/images/`
- Results saved to `project/labels/[filename].meta`
- Skips images with existing `.txt` (already approved)
- Shows progress: "50/100 processed, 45 detected"

**Output:**
```
project/labels/
├── photo_001.txt     Already approved (skipped)
├── photo_002.meta    Pending review (AI detected objects)
├── photo_003.meta    Pending review
└── (photo_004 - no detections)
```

### 3. Review Detections

```
Tools → Review → Next Unreviewed (Ctrl+U)
```

**Review Loop:**

1. **Load next .meta file**
   - Image loads automatically
   - AI detections shown on canvas
   - Status bar: "Reviewing AI detections - Edit and Approve/Reject"

2. **Inspect detections**
   - Check if polygons are correct
   - Look for missing objects
   - Verify class labels

3. **Edit if needed**
   - Drag points to adjust boundaries
   - Delete incorrect polygons (Del)
   - Add missing polygons manually
   - Fix class labels

4. **Decide:**
   
   **Option A: Approve** (Ctrl+Enter)
   - Saves current annotations to `.txt`
   - Deletes `.meta` file
   - Ready for training!
   - Jumps to next unreviewed
   
   **Option B: Reject** (Ctrl+Backspace)
   - Deletes `.meta` file
   - Clears canvas
   - Start annotation manually
   - Jumps to next unreviewed

5. **Repeat** until status bar shows "All images reviewed!"

---

## Example Workflow

### Scenario: 500 images to annotate

**Traditional Manual Annotation:**
- Time: ~2-5 minutes per image
- Total: 16-40 hours 

**With Batch Detection:**

**Step 1: Batch Detect (10 minutes)**
```bash
# Configure plugin once
Tools → Configure Plugin

# Run batch detection
Tools → Batch Detect All
# Wait: AI processes all 500 images
# Result: 450 detections, 50 empty
```

**Step 2: Review (2-5 hours)**
```bash
# Start review loop
Ctrl+U  # Next unreviewed

# For each image:
# - Quick visual check (5-10 seconds)
# - Minor edits if needed (10-30 seconds)
# - Approve (Ctrl+Enter) or Reject (Ctrl+Backspace)

# Average: 20-30 seconds per image
# Total: 2.5-4 hours
```

**Time Saved: 80-90%** 

---

## File System Details

### Directory Structure

```
MyProject/
├── MyProject.polyseg
├── images/
│   ├── photo_001.jpg
│   ├── photo_002.jpg
│   └── ...
├── labels/
│   ├── photo_001.txt    # Approved (human-verified)
│   ├── photo_002.meta   # Pending (AI-generated)
│   ├── photo_003.txt    # Approved (edited from .meta)
│   └── ...
└── models/
    └── model.pt
```

### File Status

| File | Status | Training | Editable |
|------|--------|----------|----------|
| `.txt` |  Approved | Yes | Yes (manual) |
| `.meta` |  Pending | No | Yes (before approve) |
| (none) |  Not annotated | No | Yes (manual) |

### State Transitions

```
┌─────────────────┐
│  No annotation  │
└────────┬────────┘
         │
         ├─── Manual annotation ──→ .txt (approved)
         │
         └─── Batch Detect ──→ .meta (pending)
                                     │
                                     ├─── Approve ──→ .txt (approved)
                                     │
                                     └─── Reject ──→ (deleted, back to start)
```

---

## Advanced Tips

### 1. Partial Batch Detection

**Run batch on specific subset:**
```bash
# Move images to process into separate folder
project/images/batch_1/

# Run batch detect
# Only processes images without .txt

# Review
# Approve good ones, reject bad ones
```

### 2. Multi-Pass Strategy

**First pass: High confidence**
```
Settings: confidence: 0.7
Batch Detect → Quick approve obvious detections
```

**Second pass: Lower confidence**
```
Settings: confidence: 0.3
Batch Detect → Careful review, more edits needed
```

### 3. Class-Specific Review

**Review by class:**
```bash
# Batch detect all
# Filter images in file browser by class name in .meta
# Review similar objects together (better consistency)
```

### 4. Quality Metrics

**Track your review speed:**
```
Start: 500 images, 0 approved
After 1 hour: 180 approved (3 per minute)
Estimated remaining: 1.8 hours
```

---

## Keyboard Shortcuts (Review Mode)

| Shortcut | Action | Usage |
|----------|--------|-------|
| `Ctrl+U` | Next Unreviewed | Jump to next pending image |
| `Ctrl+Enter` | Approve & Save | Accept AI detections (optionally edited) |
| `Ctrl+Backspace` | Reject & Clear | Discard AI detections, start over |
| `Ctrl+Right` | Next Image | Navigate normally (saves to .txt) |
| `Ctrl+S` | Save | Manual save to .txt (approves) |
| `Del` | Delete Selected | Remove incorrect polygon |
| `Esc` | Deselect | Clear selection |

---

## Troubleshooting

### Q: Batch detect skips all images?
**A:** They already have `.txt` files. Delete `.txt` to re-detect.

### Q: No .meta files created?
**A:** AI didn't detect anything. Check:
- Plugin configuration (Tools → Configure Plugin)
- Model path correct?
- Confidence threshold too high? Try 0.25
- Images have objects of trained classes?

### Q: Can I edit .meta files?
**A:** Yes! Load with Ctrl+U, edit polygons, then Approve (Ctrl+Enter).

### Q: What if I approve by mistake?
**A:** Delete the `.txt` file, re-run batch detect on that image.

### Q: Can I approve without reviewing?
**A:** Not recommended, but possible:
```bash
# Rename all .meta → .txt in terminal
cd project/labels/
for f in *.meta; do mv "$f" "${f%.meta}.txt"; done
```

### Q: How to track progress?
**A:** Status bar shows "X unreviewed images remaining"

---

## Best Practices

###  DO:
- Review in batches (50-100 images per session)
- Edit detections before approving
- Use high confidence (0.7+) for first pass
- Approve quickly when detections are good
- Reject quickly when detections are bad

###  DON'T:
- Blindly approve all detections
- Approve without visual inspection
- Use .meta files for training (they're unverified!)
- Delete approved .txt files accidentally
- Skip the review process

---

## Integration with Training

### Before Training

**Check annotation quality:**
```bash
# Count annotations
ls project/labels/*.txt | wc -l

# Should match your approved count
# No .meta files should be used
```

### Training Command (Framework-specific example)

```bash
# Only .txt files used (approved annotations)
# Example command - depends on your framework
python train.py \
  --data project/dataset.yaml \
  --model model.pt \
  --epochs 100 \
  --device cuda
```

**dataset.yaml:**
```yaml
path: /path/to/project
train: images
val: images  # Use train/val split if needed

names:
  0: person
  1: car
  2: dog
```

### Active Learning Loop

```
1. Annotate → Train → Deploy
   ↓
2. Collect new data → Batch Detect
   ↓
3. Review & Approve → Retrain
   ↓
4. Better model → Repeat
```

---

## Summary

**Batch Detection = Fast + Accurate**

-  **Speed:** Process hundreds of images in minutes
-  **Quality:** Manual review ensures accuracy
-  **Iterative:** Edit before approving
-  **Trackable:** See progress in real-time
-  **Safe:** .meta files never used for training

**Workflow in 3 steps:**
1. Batch Detect (`Ctrl+Shift+D`)
2. Review Loop (`Ctrl+U` → Edit → `Ctrl+Enter`)
3. Train on approved `.txt` files

**Result:** Professional-quality dataset in fraction of the time! 

---

## Need Help?

- See [PLUGIN_ARCHITECTURE.md](PLUGIN_ARCHITECTURE.md) for plugin details
- Check [IMPLEMENTATION_PLAN.md](.github/IMPLEMENTATION_PLAN.md) for development roadmap
- Submit issues on [GitHub](https://github.com/lstachowicz/PolySeg/issues)

**Happy annotating! **
