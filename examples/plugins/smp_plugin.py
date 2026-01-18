#!/usr/bin/env python3
"""
SMP (Segmentation Models PyTorch) Plugin for PolySeg
License: MIT
Repository: https://github.com/qubvel/segmentation_models.pytorch

Supports 500+ model combinations (9 architectures × 60+ encoders × 5 tasks)
Architectures: Unet, UnetPlusPlus, MAnet, Linknet, FPN, PSPNet, DeepLabV3, DeepLabV3Plus, PAN
"""

import sys
import json
import argparse
import cv2
import numpy as np
from pathlib import Path

try:
    import torch
    import segmentation_models_pytorch as smp
except ImportError as e:
    print(json.dumps({
        "success": False,
        "error": f"Missing dependencies: {str(e)}. Run: pip install segmentation-models-pytorch"
    }))
    sys.exit(1)


def detect(image_path, architecture="Unet", encoder="resnet34", weights_path=None,
           conf_threshold=0.5, classes=1):
    """Run SMP model inference on image"""
    
    # Load image
    image = cv2.imread(image_path)
    if image is None:
        return {"success": False, "error": f"Could not load image: {image_path}"}
    
    h, w = image.shape[:2]
    image_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    
    # Prepare input
    preprocessing_fn = smp.encoders.get_preprocessing_fn(encoder, pretrained='imagenet')
    image_preprocessed = preprocessing_fn(image_rgb)
    image_tensor = torch.from_numpy(image_preprocessed).permute(2, 0, 1).unsqueeze(0).float()
    
    # Create model
    device = "cuda" if torch.cuda.is_available() else "cpu"
    model_class = getattr(smp, architecture)
    model = model_class(
        encoder_name=encoder,
        encoder_weights=None if weights_path else 'imagenet',
        in_channels=3,
        classes=classes
    )
    
    # Load custom weights if provided
    if weights_path and Path(weights_path).exists():
        model.load_state_dict(torch.load(weights_path, map_location=device))
    
    model.to(device)
    model.eval()
    
    # Run inference
    with torch.no_grad():
        image_tensor = image_tensor.to(device)
        output = model(image_tensor)
        probs = torch.sigmoid(output).cpu().numpy()[0]
    
    # Convert to PolySeg format
    result = {
        "success": True,
        "detections": []
    }
    
    # Process each class channel
    for class_id in range(classes):
        prob_mask = probs[class_id]
        
        # Threshold
        binary_mask = (prob_mask > conf_threshold).astype(np.uint8)
        
        # Find contours
        contours, _ = cv2.findContours(binary_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        for contour in contours:
            # Filter small contours
            if cv2.contourArea(contour) < 100:
                continue
            
            # Simplify polygon
            epsilon = 0.005 * cv2.arcLength(contour, True)
            approx = cv2.approxPolyDP(contour, epsilon, True)
            
            # Get average confidence for this region
            mask_region = np.zeros_like(binary_mask)
            cv2.drawContours(mask_region, [contour], -1, 1, -1)
            confidence = float(np.mean(prob_mask[mask_region > 0]))
            
            # Convert to normalized coordinates
            points = []
            for point in approx:
                x, y = point[0]
                points.extend([x / w, y / h])
            
            if len(points) >= 6:  # At least 3 points
                result["detections"].append({
                    "class_id": class_id,
                    "confidence": confidence,
                    "points": points
                })
    
    return result


def train(dataset_path, output_path, architecture="Unet", encoder="resnet34",
          classes=1, epochs=10, batch_size=4, learning_rate=1e-4):
    """Train SMP model (simplified example)"""
    
    # This is a minimal training stub
    # Real training would need proper dataset loader, augmentations, etc.
    
    device = "cuda" if torch.cuda.is_available() else "cpu"
    
    # Create model
    model_class = getattr(smp, architecture)
    model = model_class(
        encoder_name=encoder,
        encoder_weights='imagenet',
        in_channels=3,
        classes=classes
    )
    model.to(device)
    
    # Setup training
    optimizer = torch.optim.Adam(model.parameters(), lr=learning_rate)
    criterion = smp.losses.DiceLoss(smp.losses.BINARY_MODE, from_logits=True)
    
    # Training loop would go here
    # For now, just save the model
    Path(output_path).parent.mkdir(parents=True, exist_ok=True)
    torch.save(model.state_dict(), output_path)
    
    return {
        "success": True,
        "message": f"Model saved to {output_path}",
        "note": "This is a stub - implement full training with your dataset"
    }


def main():
    parser = argparse.ArgumentParser(description='SMP Plugin for PolySeg')
    parser.add_argument('action', choices=['detect', 'train'],
                       help='Action to perform')
    parser.add_argument('--image', help='Path to input image (for detect)')
    parser.add_argument('--dataset', help='Path to dataset (for train)')
    parser.add_argument('--output', help='Path to save trained model')
    parser.add_argument('--architecture', default='Unet',
                       choices=['Unet', 'UnetPlusPlus', 'MAnet', 'Linknet', 
                               'FPN', 'PSPNet', 'DeepLabV3', 'DeepLabV3Plus', 'PAN'],
                       help='Model architecture')
    parser.add_argument('--encoder', default='resnet34',
                       help='Encoder backbone (e.g., resnet34, efficientnet-b0, timm-mobilenetv3_large_100)')
    parser.add_argument('--weights', help='Path to model weights')
    parser.add_argument('--conf', type=float, default=0.5,
                       help='Confidence threshold')
    parser.add_argument('--classes', type=int, default=1,
                       help='Number of segmentation classes')
    parser.add_argument('--epochs', type=int, default=10,
                       help='Training epochs')
    parser.add_argument('--batch-size', type=int, default=4,
                       help='Batch size for training')
    parser.add_argument('--lr', type=float, default=1e-4,
                       help='Learning rate')
    
    args = parser.parse_args()
    
    if args.action == 'detect':
        if not args.image:
            print(json.dumps({"success": False, "error": "--image required for detect"}))
            sys.exit(1)
        
        result = detect(
            args.image,
            args.architecture,
            args.encoder,
            args.weights,
            args.conf,
            args.classes
        )
        print(json.dumps(result, indent=2))
        
    elif args.action == 'train':
        if not args.dataset or not args.output:
            print(json.dumps({"success": False, "error": "--dataset and --output required for train"}))
            sys.exit(1)
        
        result = train(
            args.dataset,
            args.output,
            args.architecture,
            args.encoder,
            args.classes,
            args.epochs,
            args.batch_size,
            args.lr
        )
        print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
