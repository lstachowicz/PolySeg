#!/usr/bin/env python3
"""
Detectron2 Plugin for PolySeg - State-of-the-Art Instance Segmentation
License: Apache 2.0
Repository: https://github.com/facebookresearch/detectron2
"""

import sys
import json
import argparse
import os
import warnings
from pathlib import Path

# Redirect warnings to stderr before importing detectron2/torch
# This ensures only JSON goes to stdout for clean parsing
warnings.filterwarnings("default")
os.environ["PYTHONWARNINGS"] = "default"

import cv2
import numpy as np
from PIL import Image

try:
    import torch
    from detectron2.engine import DefaultPredictor, DefaultTrainer
    from detectron2.config import get_cfg
    from detectron2 import model_zoo
    from detectron2.data import DatasetCatalog, MetadataCatalog
    from detectron2.structures import BoxMode
except ImportError as e:
    print(json.dumps({
        "success": False,
        "error": f"Missing dependencies: {str(e)}. Run: pip install --no-build-isolation 'git+https://github.com/facebookresearch/detectron2.git'"
    }))
    sys.exit(1)


def get_device():
    """Get best available device: CUDA > MPS (Apple Silicon) > CPU"""
    if torch.cuda.is_available():
        return "cuda"
    elif hasattr(torch.backends, "mps") and torch.backends.mps.is_available():
        return "mps"
    return "cpu"


def yolo_to_coco_dataset(images_dir, labels_dir, class_names=None):
    """
    Convert YOLO segmentation format to COCO format for Detectron2.

    YOLO format: class_id x1 y1 x2 y2 x3 y3 ... (normalized 0-1)

    Args:
        images_dir: Directory containing images
        labels_dir: Directory containing .txt label files
        class_names: List of class names (default: ["object"])

    Returns:
        List of dicts in Detectron2 dataset format
    """
    if class_names is None:
        class_names = ["object"]

    images_dir = Path(images_dir)
    labels_dir = Path(labels_dir)

    dataset = []
    image_id = 0

    # Find all images
    image_extensions = {'.jpg', '.jpeg', '.png', '.bmp', '.tiff', '.tif'}
    image_files = [f for f in images_dir.iterdir()
                   if f.suffix.lower() in image_extensions]

    for img_path in sorted(image_files):
        # Find corresponding label file
        label_path = labels_dir / (img_path.stem + '.txt')

        if not label_path.exists():
            continue

        # Get image dimensions
        img = Image.open(img_path)
        width, height = img.size

        record = {
            "file_name": str(img_path),
            "image_id": image_id,
            "height": height,
            "width": width,
            "annotations": []
        }

        # Parse YOLO annotations
        with open(label_path, 'r', encoding='utf-8') as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) < 7:  # class_id + at least 3 points
                    continue

                class_id = int(parts[0])
                coords = list(map(float, parts[1:]))

                # Convert normalized coords to absolute pixels
                polygon = []
                for i in range(0, len(coords), 2):
                    x = coords[i] * width
                    y = coords[i + 1] * height
                    polygon.extend([x, y])

                # Calculate bounding box from polygon
                xs = polygon[0::2]
                ys = polygon[1::2]
                bbox = [min(xs), min(ys), max(xs), max(ys)]

                annotation = {
                    "bbox": bbox,
                    "bbox_mode": BoxMode.XYXY_ABS,
                    "segmentation": [polygon],
                    "category_id": class_id,
                    "iscrowd": 0
                }
                record["annotations"].append(annotation)

        if record["annotations"]:
            dataset.append(record)
            image_id += 1

    return dataset


def register_yolo_dataset(name, images_dir, labels_dir, class_names=None):
    """Register a YOLO dataset with Detectron2's DatasetCatalog"""
    if class_names is None:
        class_names = ["object"]

    # Remove if already registered
    if name in DatasetCatalog.list():
        DatasetCatalog.remove(name)
        MetadataCatalog.remove(name)

    DatasetCatalog.register(
        name,
        lambda: yolo_to_coco_dataset(images_dir, labels_dir, class_names)
    )
    MetadataCatalog.get(name).set(thing_classes=class_names)


def detect(image_path, config_file, model_weights, confidence=0.5, num_classes=None):
    """Run Detectron2 detection on image"""

    # Setup config
    cfg = get_cfg()
    cfg.merge_from_file(model_zoo.get_config_file(config_file))
    cfg.MODEL.WEIGHTS = model_weights
    cfg.MODEL.ROI_HEADS.SCORE_THRESH_TEST = confidence
    cfg.MODEL.DEVICE = get_device()

    # Set number of classes if specified (required for custom-trained models)
    if num_classes is not None:
        cfg.MODEL.ROI_HEADS.NUM_CLASSES = num_classes

    # Create predictor
    predictor = DefaultPredictor(cfg)

    # Load and process image
    im = cv2.imread(image_path)
    if im is None:
        return {"success": False, "error": f"Could not load image: {image_path}"}

    h, w = im.shape[:2]

    # Run detection
    outputs = predictor(im)

    # Extract instances
    instances = outputs["instances"].to("cpu")
    masks = instances.pred_masks.numpy()
    classes = instances.pred_classes.numpy()
    scores = instances.scores.numpy()

    # Convert to PolySeg format
    result = {
        "success": True,
        "detections": []
    }

    for i in range(len(masks)):
        # Extract mask contours
        mask = masks[i].astype(np.uint8) * 255
        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        if len(contours) > 0:
            # Use largest contour
            contour = max(contours, key=cv2.contourArea)

            # Simplify polygon
            epsilon = 0.005 * cv2.arcLength(contour, True)
            approx = cv2.approxPolyDP(contour, epsilon, True)

            # Convert to normalized coordinates
            points = []
            for point in approx:
                x, y = point[0]
                points.extend([x / w, y / h])

            if len(points) >= 6:  # At least 3 points
                result["detections"].append({
                    "class_id": int(classes[i]),
                    "confidence": float(scores[i]),
                    "points": points
                })

    return result


def train(images_dir, labels_dir, output_dir,
          config_path="COCO-InstanceSegmentation/mask_rcnn_R_50_FPN_3x.yaml",
          num_classes=1, batch_size=2, max_iter=1000, learning_rate=0.00025,
          class_names=None):
    """
    Train Detectron2 model with YOLO format dataset.

    Args:
        images_dir: Directory containing training images
        labels_dir: Directory containing YOLO .txt label files
        output_dir: Directory to save trained model
        config_path: Detectron2 config file
        num_classes: Number of classes to detect
        batch_size: Training batch size
        max_iter: Maximum training iterations
        learning_rate: Learning rate
        class_names: List of class names

    Returns:
        dict with training result
    """
    if class_names is None:
        class_names = ["object"] * num_classes if num_classes == 1 else [f"class_{i}" for i in range(num_classes)]

    # Register dataset
    dataset_name = "polyseg_train"
    register_yolo_dataset(dataset_name, images_dir, labels_dir, class_names)

    # Verify dataset has samples
    dataset = DatasetCatalog.get(dataset_name)
    if len(dataset) == 0:
        return {
            "success": False,
            "error": f"No valid image/label pairs found in {images_dir} and {labels_dir}"
        }

    # Setup config
    cfg = get_cfg()
    cfg.merge_from_file(model_zoo.get_config_file(config_path))
    cfg.MODEL.WEIGHTS = model_zoo.get_checkpoint_url(config_path)
    cfg.MODEL.DEVICE = get_device()

    cfg.DATASETS.TRAIN = (dataset_name,)
    cfg.DATASETS.TEST = ()

    cfg.DATALOADER.NUM_WORKERS = 2
    cfg.SOLVER.IMS_PER_BATCH = batch_size
    cfg.SOLVER.BASE_LR = learning_rate
    cfg.SOLVER.MAX_ITER = max_iter
    cfg.SOLVER.STEPS = []  # No LR decay

    cfg.MODEL.ROI_HEADS.BATCH_SIZE_PER_IMAGE = 128
    cfg.MODEL.ROI_HEADS.NUM_CLASSES = num_classes

    cfg.OUTPUT_DIR = output_dir
    os.makedirs(cfg.OUTPUT_DIR, exist_ok=True)

    # Train
    trainer = DefaultTrainer(cfg)
    trainer.resume_or_load(resume=False)
    trainer.train()

    # Save final model path
    final_model = os.path.join(output_dir, "model_final.pth")

    return {
        "success": True,
        "message": f"Training completed. Model saved to {final_model}",
        "model_path": final_model,
        "num_images": len(dataset),
        "iterations": max_iter
    }


def main():
    parser = argparse.ArgumentParser(description='Detectron2 Plugin for PolySeg')
    subparsers = parser.add_subparsers(dest='action', help='Action to perform')

    # Detect subcommand
    detect_parser = subparsers.add_parser('detect', help='Run detection on an image')
    detect_parser.add_argument('--image', required=True,
                               help='Path to input image')
    detect_parser.add_argument('--config',
                               default='COCO-InstanceSegmentation/mask_rcnn_R_50_FPN_3x.yaml',
                               help='Detectron2 config file')
    detect_parser.add_argument('--model',
                               help='Model weights path (default: pretrained from model zoo)')
    detect_parser.add_argument('--conf', type=float, default=0.5,
                               help='Confidence threshold (0-1)')
    detect_parser.add_argument('--num-classes', type=int, default=None,
                               help='Number of classes (required for custom-trained models)')

    # Train subcommand
    train_parser = subparsers.add_parser('train', help='Train model with YOLO format dataset')
    train_parser.add_argument('--images', required=True,
                              help='Directory containing training images')
    train_parser.add_argument('--labels', required=True,
                              help='Directory containing YOLO .txt label files')
    train_parser.add_argument('--output', required=True,
                              help='Directory to save trained model')
    train_parser.add_argument('--config',
                              default='COCO-InstanceSegmentation/mask_rcnn_R_50_FPN_3x.yaml',
                              help='Detectron2 config file')
    train_parser.add_argument('--num-classes', type=int, default=1,
                              help='Number of classes')
    train_parser.add_argument('--class-names', nargs='+',
                              help='List of class names (e.g., --class-names leg foot)')
    train_parser.add_argument('--batch-size', type=int, default=2,
                              help='Batch size for training')
    train_parser.add_argument('--max-iter', type=int, default=1000,
                              help='Maximum iterations')
    train_parser.add_argument('--lr', type=float, default=0.00025,
                              help='Learning rate')

    # Convert subcommand (utility to export COCO JSON if needed)
    convert_parser = subparsers.add_parser('convert', help='Convert YOLO to COCO JSON')
    convert_parser.add_argument('--images', required=True,
                                help='Directory containing images')
    convert_parser.add_argument('--labels', required=True,
                                help='Directory containing YOLO .txt label files')
    convert_parser.add_argument('--output', required=True,
                                help='Output COCO JSON file path')
    convert_parser.add_argument('--class-names', nargs='+', default=['object'],
                                help='List of class names')

    args = parser.parse_args()

    if args.action == 'detect':
        # Use model zoo pretrained if no model specified
        model_weights = args.model
        if model_weights is None:
            model_weights = model_zoo.get_checkpoint_url(args.config)
        result = detect(args.image, args.config, model_weights, args.conf, args.num_classes)
        print(json.dumps(result, indent=2))

    elif args.action == 'train':
        result = train(
            args.images, args.labels, args.output, args.config,
            args.num_classes, args.batch_size, args.max_iter, args.lr,
            args.class_names
        )
        print(json.dumps(result, indent=2))
        if not result.get("success", False):
            sys.exit(1)

    elif args.action == 'convert':
        # Export YOLO dataset to COCO JSON format
        dataset = yolo_to_coco_dataset(args.images, args.labels, args.class_names)
        coco_output = {
            "images": [],
            "annotations": [],
            "categories": [{"id": i, "name": name} for i, name in enumerate(args.class_names)]
        }
        ann_id = 0
        for record in dataset:
            coco_output["images"].append({
                "id": record["image_id"],
                "file_name": record["file_name"],
                "height": record["height"],
                "width": record["width"]
            })
            for ann in record["annotations"]:
                coco_output["annotations"].append({
                    "id": ann_id,
                    "image_id": record["image_id"],
                    "category_id": ann["category_id"],
                    "bbox": ann["bbox"],
                    "segmentation": ann["segmentation"],
                    "area": cv2.contourArea(
                        np.array(ann["segmentation"][0]).reshape(-1, 2).astype(np.float32)
                    ),
                    "iscrowd": 0
                })
                ann_id += 1

        with open(args.output, 'w', encoding='utf-8') as f:
            json.dump(coco_output, f, indent=2)

        print(json.dumps({
            "success": True,
            "message": f"Converted {len(dataset)} images to COCO format",
            "output": args.output
        }, indent=2))

    else:
        parser.print_help()
        sys.exit(1)


if __name__ == '__main__':
    main()
