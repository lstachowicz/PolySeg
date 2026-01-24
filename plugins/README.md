# Custom Plugins Directory

This directory is for **your personal custom plugins**. Files here are ignored by git.

## Quick Start

1. **Copy an example plugin:**
   ```bash
   cp ../examples/plugins/model_plugin.py ./my_custom_plugin.py
   ```

2. **Edit to use your model:**
   - Modify detection logic
   - Change model loading
   - Adjust JSON output format

3. **Configure in PolySeg:**
   - Tools → Configure Plugin
   - Set script path to `plugins/my_custom_plugin.py`
   - Add custom settings (model path, confidence, etc.)

## Official Example Plugins

See `examples/plugins/` for ready-to-use plugins:
- `detectron2_plugin.py` - Detectron2 Mask R-CNN
- `smp_plugin.py` - Segmentation Models PyTorch (500+ models)

## Documentation

- **Plugin Development:** `../docs/PLUGIN_DEVELOPMENT.md`
- **JSON Protocol:** `../docs/PLUGIN_ARCHITECTURE.md`
- **Available Plugins:** `../docs/AVAILABLE_PLUGINS.md`

## License

Your custom plugins can use any license. PolySeg communicates via subprocess, 
so there's no license contamination.
