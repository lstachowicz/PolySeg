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
import yaml

try:
    import torch
    from torch.utils.data import Dataset, DataLoader
    import segmentation_models_pytorch as smp
    import albumentations as A
    from albumentations.pytorch import ToTensorV2
except ImportError as e:
    print(json.dumps({
        "success": False,
        "error": f"Missing dependencies: {str(e)}. Run: pip install segmentation-models-pytorch albumentations"
    }))
    sys.exit(1)


def get_device():
    """Get best available device: CUDA > MPS (Apple Silicon) > CPU"""
    if torch.cuda.is_available():
        return torch.device("cuda")
    elif hasattr(torch.backends, "mps") and torch.backends.mps.is_available():
        return torch.device("mps")
    return torch.device("cpu")


class YOLOSegmentationDataset(Dataset):
    """Dataset that reads YOLO format and converts to masks"""

    def __init__(self, data_yaml_path, split='train', img_size=512, transform=None):
        self.img_size = img_size
        self.transform = transform

        # Parse data.yaml
        with open(data_yaml_path, 'r') as f:
            data_config = yaml.safe_load(f)

        self.base_path = Path(data_config.get('path', Path(data_yaml_path).parent))
        self.num_classes = data_config.get('nc', 1)
        self.class_names = data_config.get('names', {})

        # Get split file path
        split_file = data_config.get(split, f'splits/{split}.txt')
        split_path = self.base_path / split_file

        # Load image list from split file
        self.images = []
        self.labels = []

        if split_path.exists():
            with open(split_path, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line:
                        # Handle relative/absolute paths
                        if line.startswith('./'):
                            img_path = self.base_path / line[2:]
                        elif line.startswith('/'):
                            img_path = Path(line)
                        else:
                            img_path = self.base_path / line

                        if img_path.exists():
                            # Find corresponding label file
                            label_path = self.base_path / 'labels' / (img_path.stem + '.txt')
                            self.images.append(img_path)
                            self.labels.append(label_path if label_path.exists() else None)

        print(f"Loaded {len(self.images)} images for {split} split")

    def __len__(self):
        return len(self.images)

    def yolo_to_mask(self, label_path, img_height, img_width):
        """Convert YOLO polygon format to binary mask"""
        mask = np.zeros((img_height, img_width), dtype=np.uint8)

        if label_path is None or not label_path.exists():
            return mask

        with open(label_path, 'r') as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) < 7:  # class_id + at least 3 points (6 coords)
                    continue

                class_id = int(parts[0])
                coords = list(map(float, parts[1:]))

                # Convert normalized coords to pixel coords
                points = []
                for i in range(0, len(coords), 2):
                    x = int(coords[i] * img_width)
                    y = int(coords[i + 1] * img_height)
                    points.append([x, y])

                if len(points) >= 3:
                    pts = np.array(points, dtype=np.int32)
                    cv2.fillPoly(mask, [pts], class_id + 1)  # class_id + 1 (0 is background)

        return mask

    def __getitem__(self, idx):
        # Load image
        img_path = self.images[idx]
        image = cv2.imread(str(img_path))
        image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        h, w = image.shape[:2]

        # Load/create mask
        label_path = self.labels[idx]
        mask = self.yolo_to_mask(label_path, h, w)

        # Convert mask to binary (for single class) or keep multi-class
        if self.num_classes == 1:
            mask = (mask > 0).astype(np.uint8)

        # Apply transforms
        if self.transform:
            transformed = self.transform(image=image, mask=mask)
            image = transformed['image']
            mask = transformed['mask']

        # Ensure mask has correct shape for loss function
        if len(mask.shape) == 2:
            mask = mask.unsqueeze(0).float()

        return image, mask


def get_transforms(img_size, is_train=True):
    """Get augmentation transforms"""
    if is_train:
        return A.Compose([
            A.Resize(img_size, img_size),
            A.HorizontalFlip(p=0.5),
            A.VerticalFlip(p=0.5),
            A.RandomRotate90(p=0.5),
            A.ShiftScaleRotate(shift_limit=0.1, scale_limit=0.1, rotate_limit=15, p=0.5),
            A.RandomBrightnessContrast(p=0.3),
            A.Normalize(mean=(0.485, 0.456, 0.406), std=(0.229, 0.224, 0.225)),
            ToTensorV2(),
        ])
    else:
        return A.Compose([
            A.Resize(img_size, img_size),
            A.Normalize(mean=(0.485, 0.456, 0.406), std=(0.229, 0.224, 0.225)),
            ToTensorV2(),
        ])


def detect(image_path, architecture="Unet", encoder="resnet34", weights_path=None,
           conf_threshold=0.5, classes=1):
    """Run SMP model inference on image"""

    # Load image
    image = cv2.imread(image_path)
    if image is None:
        return {"success": False, "error": f"Could not load image: {image_path}"}

    h, w = image.shape[:2]
    image_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    # Prepare input with same transforms as training
    transform = get_transforms(512, is_train=False)
    transformed = transform(image=image_rgb)
    image_tensor = transformed['image'].unsqueeze(0)

    # Create model
    device = get_device()
    model_class = getattr(smp, architecture)
    model = model_class(
        encoder_name=encoder,
        encoder_weights=None,
        in_channels=3,
        classes=classes
    )

    # Load custom weights
    if weights_path and Path(weights_path).exists():
        model.load_state_dict(torch.load(weights_path, map_location=device))
    else:
        return {"success": False, "error": f"Weights file not found: {weights_path}"}

    model.to(device)
    model.eval()

    # Run inference
    with torch.no_grad():
        image_tensor = image_tensor.to(device)
        output = model(image_tensor)
        probs = torch.sigmoid(output).cpu().numpy()[0]

    # Resize probability map back to original size
    probs_resized = np.zeros((classes, h, w))
    for c in range(classes):
        probs_resized[c] = cv2.resize(probs[c], (w, h))

    # Convert to PolySeg format
    result = {
        "success": True,
        "detections": []
    }

    # Process each class channel
    for class_id in range(classes):
        prob_mask = probs_resized[class_id]

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
          classes=1, epochs=10, batch_size=4, learning_rate=1e-4, img_size=512):
    """Train SMP model with YOLO format dataset"""

    device = get_device()
    print(f"Using device: {device}")

    # Create datasets
    train_dataset = YOLOSegmentationDataset(
        dataset_path, split='train', img_size=img_size,
        transform=get_transforms(img_size, is_train=True)
    )
    val_dataset = YOLOSegmentationDataset(
        dataset_path, split='val', img_size=img_size,
        transform=get_transforms(img_size, is_train=False)
    )

    if len(train_dataset) == 0:
        return {"success": False, "error": "No training images found"}

    # Create dataloaders
    train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True, num_workers=4)
    val_loader = DataLoader(val_dataset, batch_size=batch_size, shuffle=False, num_workers=4) if len(val_dataset) > 0 else None

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
    optimizer = torch.optim.AdamW(model.parameters(), lr=learning_rate, weight_decay=1e-4)
    scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=epochs)

    # Loss: combination of Dice and BCE
    dice_loss = smp.losses.DiceLoss(smp.losses.BINARY_MODE, from_logits=True)
    bce_loss = smp.losses.SoftBCEWithLogitsLoss()

    def criterion(pred, target):
        return 0.5 * dice_loss(pred, target) + 0.5 * bce_loss(pred, target)

    # Training loop
    best_val_loss = float('inf')
    Path(output_path).parent.mkdir(parents=True, exist_ok=True)

    for epoch in range(epochs):
        # Training
        model.train()
        train_loss = 0.0

        for batch_idx, (images, masks) in enumerate(train_loader):
            images = images.to(device)
            masks = masks.to(device)

            optimizer.zero_grad()
            outputs = model(images)
            loss = criterion(outputs, masks)
            loss.backward()
            optimizer.step()

            train_loss += loss.item()

            if batch_idx % 10 == 0:
                print(f"Epoch {epoch+1}/{epochs}, Batch {batch_idx}/{len(train_loader)}, Loss: {loss.item():.4f}")

        train_loss /= len(train_loader)

        # Validation
        val_loss = 0.0
        if val_loader:
            model.eval()
            with torch.no_grad():
                for images, masks in val_loader:
                    images = images.to(device)
                    masks = masks.to(device)
                    outputs = model(images)
                    loss = criterion(outputs, masks)
                    val_loss += loss.item()
            val_loss /= len(val_loader)

            print(f"Epoch {epoch+1}/{epochs} - Train Loss: {train_loss:.4f}, Val Loss: {val_loss:.4f}")

            # Save best model
            if val_loss < best_val_loss:
                best_val_loss = val_loss
                torch.save(model.state_dict(), output_path)
                print(f"Saved best model with val_loss: {val_loss:.4f}")
        else:
            print(f"Epoch {epoch+1}/{epochs} - Train Loss: {train_loss:.4f}")
            # Save every epoch if no validation
            torch.save(model.state_dict(), output_path)

        scheduler.step()

    # Save final model
    final_path = str(Path(output_path).parent / 'last.pt')
    torch.save(model.state_dict(), final_path)

    return {
        "success": True,
        "message": f"Training complete. Best model saved to {output_path}",
        "best_val_loss": best_val_loss if val_loader else None,
        "epochs_trained": epochs
    }


def main():
    parser = argparse.ArgumentParser(description='SMP Plugin for PolySeg')
    parser.add_argument('action', choices=['detect', 'train'],
                       help='Action to perform')
    parser.add_argument('--image', help='Path to input image (for detect)')
    parser.add_argument('--dataset', help='Path to data.yaml (for train)')
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
    parser.add_argument('--img-size', type=int, default=512,
                       help='Image size for training')

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
            args.lr,
            args.img_size
        )
        print(json.dumps(result, indent=2))
        if not result.get("success", False):
            sys.exit(1)


if __name__ == '__main__':
    main()
