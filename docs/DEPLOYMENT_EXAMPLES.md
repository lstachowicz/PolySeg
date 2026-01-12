# Plugin Deployment Examples

This guide shows how to configure PolySeg plugins for different Python deployment scenarios.

## Quick Reference

| Environment | Command | Script Path |
|-------------|---------|-------------|
| Native Python | `python3` | `./plugins/script.py` |
| Docker | `docker` | `exec -i container python /app/script.py` |
| Virtual env | `bash` | `-c "source venv/bin/activate && python ./plugins/script.py"` |
| Conda | `conda` | `run -n env_name python ./plugins/script.py` |
| Poetry | `poetry` | `run python ./plugins/script.py` |
| Pipenv | `pipenv` | `run python ./plugins/script.py` |

---

## 1. Native Python

**When to use:** Simple setup, system-wide Python packages

**Configuration:**
```
Command: python3
Script Path: ./examples/plugins/model_plugin.py
Detect Args: detect --image {image} --model {model} --conf {confidence}
```

**Installation:**
```bash
pip3 install ultralytics opencv-python
```

**Pros:**  Simple, fast  
**Cons:**  May conflict with other projects

---

## 2. Docker Container

**When to use:** Isolated environment, GPU access, consistent deployment

### Option A: Interactive Container

**Configuration:**
```
Command: docker
Script Path: exec -i ai_container python /app/model_plugin.py
Detect Args: detect --image /app/data/{image_name} --model {model}

Settings:
  image_name: current.jpg
  model: /app/models/model.pt
```

**Setup:**
```bash
# 1. Create Dockerfile
cat > Dockerfile <<EOF
FROM ultralytics/ultralytics:latest
RUN pip install opencv-python
COPY model_plugin.py /app/model_plugin.py
WORKDIR /app
EOF

# 2. Build image
docker build -t ai_detector .

# 3. Run container with mounted volumes
docker run -d --name ai_container \
  --gpus all \
  -v /path/to/project:/app/data \
  -v /path/to/models:/app/models \
  ai_detector \
  tail -f /dev/null  # Keep container running
```

**Note:** Copy current image to Docker volume before detection, or use full path mapping.

### Option B: One-shot Containers

**Configuration:**
```
Command: docker
Script Path: run --rm -v {project}:/app/data -v ./models:/app/models ai_detector python /app/model_plugin.py
Detect Args: detect --image /app/data/images/{image_basename} --model {model}

Settings:
  image_basename: photo.jpg  # Extract from {image}
  model: /app/models/model.pt
```

**Pros:**  Clean isolation, GPU support  
**Cons:**  Slower startup, volume mapping complexity

---

## 3. Virtual Environment (venv)

**When to use:** Project-specific dependencies, no system pollution

**Configuration:**
```
Command: bash
Script Path: -c "source venv/bin/activate && python ./examples/plugins/model_plugin.py"
Detect Args: detect --image {image} --model {model} --conf {confidence}

Settings:
  model: ./models/model.pt
  confidence: 0.25
```

**Setup:**
```bash
# 1. Create virtual environment
python3 -m venv venv

# 2. Activate and install
source venv/bin/activate
pip install ultralytics opencv-python

# 3. Deactivate (PolySeg will activate automatically)
deactivate
```

**Alternative (Windows):**
```
Command: cmd
Script Path: /c "venv\\Scripts\\activate && python .\\examples\\plugins\\model_plugin.py"
```

**Pros:**  Isolated, standard Python tool  
**Cons:**  Manual activation command needed

---

## 4. Conda Environment

**When to use:** Complex dependencies, scientific packages, multiple Python versions

**Configuration:**
```
Command: conda
Script Path: run -n ai_env python ./examples/plugins/model_plugin.py
Detect Args: detect --image {image} --model {model} --conf {confidence}

Settings:
  model: ./models/model.pt
  confidence: 0.25
```

**Setup:**
```bash
# 1. Create conda environment
conda create -n ai_env python=3.11

# 2. Install packages
conda activate ai_env
conda install pytorch torchvision -c pytorch
pip install ultralytics opencv-python
conda deactivate
```

**Alternative - No shell activation:**
```
Command: /home/user/miniconda3/envs/ai_env/bin/python
Script Path: ./examples/plugins/model_plugin.py
Detect Args: detect --image {image} --model {model}
```

**Pros:**  Best for data science, handles complex deps  
**Cons:**  Slower than venv, larger disk usage

---

## 5. Poetry

**When to use:** Modern Python projects, dependency management

**Configuration:**
```
Command: poetry
Script Path: run python ./examples/plugins/model_plugin.py
Detect Args: detect --image {image} --model {model} --conf {confidence}

Settings:
  model: ./models/model.pt
  confidence: 0.25
```

**Setup:**
```bash
# 1. Initialize Poetry project
cd /path/to/project
poetry init

# 2. Add dependencies
poetry add ultralytics opencv-python

# 3. Test
poetry run python ./examples/plugins/model_plugin.py --help
```

**Pros:**  Modern, reproducible, lock files  
**Cons:**  Learning curve, overhead for simple projects

---

## 6. Pipenv

**When to use:** Alternative to venv with better dependency management

**Configuration:**
```
Command: pipenv
Script Path: run python ./examples/plugins/model_plugin.py
Detect Args: detect --image {image} --model {model} --conf {confidence}

Settings:
  model: ./models/model.pt
  confidence: 0.25
```

**Setup:**
```bash
# 1. Initialize Pipenv
cd /path/to/project
pipenv install ultralytics opencv-python

# 2. Test
pipenv run python ./examples/plugins/model_plugin.py --help
```

**Pros:**  Lockfile, automatic venv management  
**Cons:**  Slower than venv

---

## 7. System-wide with Modules (Environment Modules)

**When to use:** HPC clusters, shared servers

**Configuration:**
```
Command: bash
Script Path: -c "module load python/3.11 cuda/12.1 && python ./examples/plugins/model_plugin.py"
Detect Args: detect --image {image} --model {model}

Settings:
  model: ./models/model.pt
```

**Setup:**
```bash
# On cluster login node
module load python/3.11
pip install --user ultralytics opencv-python
```

**Pros:**  HPC-friendly, version management  
**Cons:**  HPC-specific

---

## 8. Singularity/Apptainer (HPC Alternative to Docker)

**When to use:** HPC environments without Docker support

**Configuration:**
```
Command: singularity
Script Path: exec ai_container.sif python /app/model_plugin.py
Detect Args: detect --image {image} --model {model}

Settings:
  model: ./models/model.pt
```

**Setup:**
```bash
# 1. Create definition file
cat > ai_model.def <<EOF
Bootstrap: docker
From: ultralytics/ultralytics:latest

%post
    pip install opencv-python

%files
    model_plugin.py /app/model_plugin.py

%runscript
    python /app/model_plugin.py "$@"
EOF

# 2. Build container
singularity build ai_container.sif ai_model.def
```

**Pros:**  HPC-compatible, no root needed  
**Cons:**  Build requires root (or cloud build)

---

## Comparison Table

| Environment | Setup Time | Isolation | GPU Support | Best For |
|-------------|------------|-----------|-------------|----------|
| Native Python |  Fast |  None |  Yes | Quick tests |
| Docker |  Slow |  Full |  Yes | Production |
| venv |  Fast |  Partial |  Yes | Development |
| Conda |  Medium |  Partial |  Yes | Data science |
| Poetry |  Fast |  Partial |  Yes | Modern projects |
| Pipenv |  Fast |  Partial |  Yes | Teams |
| Singularity |  Slow |  Full |  Yes | HPC clusters |

---

## Advanced: Custom Environment Variables

**Example: Set CUDA device in Docker**

```
Command: docker
Script Path: exec -e CUDA_VISIBLE_DEVICES=0 -i ai_container python /app/model_plugin.py
Detect Args: detect --image /app/data/{image_name} --model {model}
```

**Example: Set PyTorch settings in venv**

```
Command: bash
Script Path: -c "source venv/bin/activate && PYTORCH_ENABLE_MPS_FALLBACK=1 python ./plugins/model_plugin.py"
Detect Args: detect --image {image} --model {model}
```

---

## Troubleshooting

### Problem: "Python not found"
**Solution:** Use absolute path to Python:
```
Command: /usr/bin/python3
# or
Command: /home/user/miniconda3/bin/python
```

### Problem: Docker container exited
**Solution:** Use persistent container with `tail -f /dev/null`

### Problem: venv activation fails
**Solution:** Use absolute path:
```
Command: bash
Script Path: -c "source /absolute/path/to/venv/bin/activate && python ./plugins/script.py"
```

### Problem: Module not found in conda
**Solution:** Use full path to conda Python:
```
Command: /home/user/miniconda3/envs/myenv/bin/python
Script Path: ./plugins/script.py
```

### Problem: GPU not available in Docker
**Solution:** Add `--gpus all` to docker run, install nvidia-docker

---

## Testing Your Configuration

**Quick test script:**

```bash
#!/bin/bash
# test_plugin.sh

IMAGE="/path/to/test/image.jpg"
MODEL="./models/model.pt"

# Test your configured command
<YOUR_COMMAND> <YOUR_SCRIPT> detect --image "$IMAGE" --model "$MODEL" --conf 0.25

# Should output JSON like:
# {"status": "success", "detections": [...]}
```

**Debug mode:**
```bash
# Add verbose flag to your plugin
Detect Args: detect --image {image} --model {model} --verbose
```

---

## Best Practices

1. **Use virtual environments** for development (venv/conda)
2. **Use Docker** for production deployment
3. **Test manually** before configuring in PolySeg
4. **Use absolute paths** when relative paths cause issues
5. **Check plugin stdout** - only JSON should be printed
6. **Handle stderr** - errors should go to stderr, not stdout
7. **Document dependencies** in requirements.txt or environment.yml

---

## Need Help?

- See [PLUGIN_ARCHITECTURE.md](PLUGIN_ARCHITECTURE.md) for plugin API details
- Check [examples/plugins/](../examples/plugins/) for working examples
- Test command manually in terminal before using in PolySeg
- Ensure only JSON is printed to stdout (no debug prints!)

**Remember:** PolySeg reads **stdout only** - keep it clean! 🧹
