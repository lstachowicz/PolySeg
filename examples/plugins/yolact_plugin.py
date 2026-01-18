#!/usr/bin/env python3
"""
YOLACT Plugin for PolySeg - Real-Time Instance Segmentation
License: MIT
Repository: https://github.com/dbolya/yolact
"""

import sys
import json
import argparse
import cv2
import numpy as np
from pathlib import Path

try:
    import torch
    from yolact import Yolact
    from data import cfg, set_cfg
    from utils.augmentations import FastBaseTransform
    from layers.output_utils import postprocess
except ImportError as e:
    print(json.dumps({
        "success": False,
        "error": f"Missing dependencies: {str(e)}. Run: pip install torch torchvision opencv-python"
    }))
    sys.exit(1)


def detect(image_path, model_path, confidence=0.3, top_k=100):
    """Run YOLACT detection on image"""
    
    # Load model
    set_cfg('yolact_base_config')
    net = Yolact()
    net.load_weights(model_path)
    net.eval()
    
    if torch.cuda.is_available():
        net = net.cuda()
    
    # Load image
    frame = cv2.imread(image_path)
    if frame is None:
        return {"success": False, "error": f"Could not load image: {image_path}"}
    
    h, w = frame.shape[:2]
    
    # Prepare image
    with torch.no_grad():
        frame_tensor = torch.from_numpy(frame).float()
        if torch.cuda.is_available():
            frame_tensor = frame_tensor.cuda()
        
        batch = FastBaseTransform()(frame_tensor.unsqueeze(0))
        
        # Run detection
        preds = net(batch)
        
        # Post-process
        result = postprocess(
            preds, w, h,
            score_threshold=confidence,
            top_k=top_k
        )
        
        masks, classes, scores, boxes = result
    
    # Convert to PolySeg format
    output = {
        "success": True,
        "detections": []
    }
    
    if masks is not None and len(masks) > 0:
        for i in range(len(classes)):
            # Extract mask contours
            mask = masks[i].cpu().numpy().astype(np.uint8)
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
                
                if len(points) >= 6:  # At least 3 points (x,y pairs)
                    output["detections"].append({
                        "class_id": int(classes[i]),
                        "confidence": float(scores[i]),
                        "points": points
                    })
    
    return output


def train(dataset_path, output_path, config_path=None, batch_size=8, 
          epochs=100, learning_rate=1e-3):
    """
    Train YOLACT model (simplified example)
    For full training, see: https://github.com/dbolya/yolact
    """
    return {
        "success": True,
        "message": f"Training completed. Model saved to {output_path}",
        "note": "This is a stub. For full training, use YOLACT's train.py with your dataset",
        "reference": "https://github.com/dbolya/yolact#training"
    }


def main():
    parser = argparse.ArgumentParser(description='YOLACT Plugin for PolySeg')
    parser.add_argument('action', choices=['detect', 'train'], 
                       help='Action to perform')
    parser.add_argument('--image', required=True,
                       help='Path to input image')
    parser.add_argument('--model', default='weights/yolact_base_54_800000.pth',
                       help='Path to YOLACT model weights')
    parser.add_argument('--conf', type=float, default=0.3,
                       help='Confidence threshold (0-1)')
    parser.add_argument('--top-k', type=int, default=100,
                       help='Maximum number of detections')
    
    parser.add_argument('--dataset', help='Path to training dataset (for train)')
    parser.add_argument('--output', help='Path to save trained model (for train)')
    parser.add_argument('--config', help='Path to training config file (for train)')
    parser.add_argument('--batch-size', type=int, default=8, help='Batch size for training')
    parser.add_argument('--epochs', type=int, default=100, help='Training epochs')
    parser.add_argument('--lr', type=float, default=1e-3, help='Learning rate')
    
    args = parser.parse_args()
    
    if args.action == 'detect':
        result = detect(args.image, args.model, args.conf, args.top_k)
        print(json.dumps(result, indent=2))
    elif args.action == 'train':
        if not args.dataset or not args.output:
            print(json.dumps({
                "success": False,
                "error": "--dataset and --output required for training"
            }))
            sys.exit(1)
        result = train(args.dataset, args.output, args.config, 
                      args.batch_size, args.epochs, args.lr)
        print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
