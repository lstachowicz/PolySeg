# Available PolySeg Plugins - Comparison Guide

This document helps you choose the right AI plugin for your project.

##  Quick Comparison Table

| Feature | SAM | YOLACT | Detectron2 | SMP |
|---------|-----|--------|------------|-----|
| **License** | Apache 2.0 | MIT  | Apache 2.0  | MIT  |
| **Inference Speed** | 5-10 FPS | 30+ FPS | 10-15 FPS | 15-25 FPS |
| **COCO mAP** | N/A | 29.8 | 46.5 | 40-45 |
| **GPU Memory** | 8-12 GB | 3-5 GB | 6-8 GB | 4-6 GB |
| **Training** |  No |  Medium |  Advanced |  Easy |
| **Zero-shot** |  Yes |  No |  No |  No |
| **Real-time** |  No |  Yes |  Limited |  Limited |
| **Best For** | Exploration | Speed | Accuracy | Flexibility |

## 🔍 Detailed Comparison

### 1. SAM (Segment Anything Model)

**License:** Apache 2.0   
**GitHub:** `facebookresearch/segment-anything`  
**Status:** 🔜 **Planned (Phase 15)**

#### Pros:
-  **Zero-shot segmentation** - No training needed!
-  **Segment anything** - Works on any object
-  **High quality masks** - Superior boundary precision
-  **Prompt-based** - Click, box, or mask input
-  **Apache 2.0 license** - Commercial friendly

#### Cons:
-  **No training** - Can't adapt to specific classes
-  **Slow inference** - 5-10 FPS
-  **High memory** - Requires 8-12 GB VRAM
-  **No class labels** - Segments without classification

#### When to Use:
- **Exploring new datasets** - Don't know classes yet
- **One-time annotation** - Not retraining
- Need **perfect boundaries** - Medical, satellite imagery
- **No labeled data** available

#### Performance:
- **Speed:** 5-10 FPS (RTX 3090, ViT-H)
- **IoU:** 0.93 (on validation set)
- **Inference time:** 100-200ms per image

---

### 2. YOLACT (Real-time Instance Segmentation)

**License:** MIT  🔥  
**GitHub:** `dbolya/yolact`  
**Status:** 🔜 **Planned (Phase 15)**

#### Pros:
-  **MIT license** - Use in any project!
-  **Real-time** - 30+ FPS on GPU
-  **Lightweight** - Only 3-5 GB VRAM
-  **Simple architecture** - Easy to understand/modify
-  **Good for production** - Stable, predictable

#### Cons:
-  **Lower accuracy** - mAP 29.8 on COCO
-  **No official training guide** for custom data
-  **Older codebase** - Less active development

#### When to Use:
- Need **MIT license** for commercial project
- Building **real-time application** (embedded, edge devices)
- **Resource-constrained** environment (< 4GB VRAM)
- Don't need SOTA accuracy

#### Performance:
- **Speed:** 30-40 FPS (RTX 3090, YOLACT-550)
- **COCO mAP:** 29.8
- **Training time:** 8-12 hours (COCO subset)

---

### 4. Detectron2 (Facebook AI Research)

**License:** Apache 2.0   
**GitHub:** `facebookresearch/detectron2`  
**Status:** 🔜 **Planned (Phase 15)**

#### Pros:
-  **SOTA accuracy** - Best on COCO (mAP 46.5+)
-  **Apache 2.0 license** - Commercial friendly
-  **Multiple architectures** - Mask R-CNN, PointRend, Cascade
-  **Research-grade** - Replicate papers exactly
-  **Extensive model zoo** - 100+ pretrained models
-  **Panoptic segmentation** - Instance + semantic

#### Cons:
-  **Slower inference** - 10-15 FPS
-  **Complex setup** - Steep learning curve
-  **High memory** - 6-8 GB VRAM minimum
-  **Longer training** - 12-24 hours for good results

#### When to Use:
- Need **highest accuracy** possible
- **Research project** - Benchmarking, paper reproduction
- Have **powerful GPU** (RTX 3090, A100)
- **Accuracy > Speed** priority

#### Performance:
- **Speed:** 10-15 FPS (RTX 3090, Mask R-CNN R-50)
- **COCO mAP:** 46.5 (PointRend)
- **Training time:** 12-24 hours (COCO)

---

### 5. SMP (Segmentation Models PyTorch)

**License:** MIT  🔥  
**GitHub:** `qubvel-org/segmentation_models.pytorch`  
**Status:** 🔜 **Planned (Phase 15)**

#### Pros:
-  **MIT license** - Use in any project!
-  **500+ models** - UNet, PSPNet, DeepLabV3, FPN, PAN
-  **50+ encoders** - ResNet, EfficientNet, MobileNet, etc.
-  **Easy fine-tuning** - Simple API
-  **Great docs** - Well-documented, active community
-  **Flexible** - Customize architecture easily

#### Cons:
-  **Semantic segmentation** - No instance separation
-  **Requires conversion** - Output is masks, need post-processing
-  **Training needed** - No zero-shot capability

#### When to Use:
- Need **MIT license** with flexibility
- **Semantic segmentation** is sufficient
- Want to **experiment** with architectures
- Need **mobile deployment** (MobileNet encoder)
- Have **limited training data** (pretrained encoders help)

#### Performance:
- **Speed:** 15-25 FPS (RTX 3090, UNet-ResNet34)
- **Accuracy:** Depends on architecture (IoU 0.75-0.85 typical)
- **Training time:** 6-10 hours (custom dataset)

---

##  Decision Tree

### I need MIT/Apache license:
- **Speed priority:** → YOLACT (MIT)
- **Accuracy priority:** → Detectron2 (Apache 2.0)
- **Flexibility:** → SMP (MIT)

### I have no labeled data:
- → SAM (zero-shot)

### I need highest accuracy:
- → Detectron2 (PointRend)

### I'm building research project:
- → Detectron2 (reproducibility)

### I have limited GPU (< 4GB):
- → YOLACT

---

## 📈 Benchmark Results

**Dataset:** COCO 2017 validation  
**Hardware:** NVIDIA RTX 3090 (24GB)  
**Metric:** Mask mAP (instance segmentation)

| Model | Mask mAP | FPS | Memory |
|-------|----------|-----|--------|
| **YOLACT-550** | 29.8 | 35 | 4.2 GB |
| **Mask R-CNN R-50** | 38.6 | 15 | 6.5 GB |
| **PointRend R-50** | 39.3 | 12 | 7.1 GB |
| **SAM ViT-H** | N/A (zero-shot) | 8 | 11.2 GB |

---

## 💡 Tips

### For Production:
1. Start with **YOLACT** for real-time applications (MIT license)
2. Use **Detectron2** if accuracy is critical
3. Try **SMP** for semantic segmentation tasks

### For Research:
1. Use **Detectron2** for SOTA baselines
2. Try **SMP** for architecture experiments
3. Use **SAM** for data exploration

### For Embedded/Edge:
1. **YOLACT** with lightweight backbone
2. **SMP** with MobileNet encoder
3. Export to ONNX/TensorRT for optimization

---

## 📚 Further Reading

- **SAM:** https://segment-anything.com/
- **YOLACT:** https://github.com/dbolya/yolact
- **Detectron2:** https://detectron2.readthedocs.io/
- **SMP:** https://smp.readthedocs.io/

---

**Last Updated:** 2026-01-11  
**PolySeg Version:** 1.0.0
