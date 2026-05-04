# Development Guide

## Code Style & Analysis Tools

### clang-format
Automatically format code according to Google C++ Style Guide:

```bash
# Format all C++ files in libs/ and src/
find libs src -name "*.h" -o -name "*.cpp" | xargs clang-format -i --style=file
```

Configuration: `.clang-format`

### clang-tidy
Static analysis for code quality:

```bash
# Analyze C++ files (requires C++17)
clang-tidy -checks=* libs/imgproc/convolution.h -- -std=c++17 -Ilibs
```

Configuration: `.clang-tidy`

### cppcheck
Static code analysis tool:

```bash
# Run cppcheck with C++17 standard
cppcheck --std=c++17 --language=c++ \
  -Ilibs -Isrc \
  --exclude-dir=external \
  --exclude-dir=build \
  --exclude-dir=test123 \
  libs/ src/ tests/
```

**Important:** cppcheck requires `--language=c++` flag to properly analyze C++17 namespace syntax.

Configuration files:
- `.cppcheck` - Project-level configuration (for IDE integration)
- `.cppcheckignore` - Paths to exclude from analysis

### IDE Configuration

#### CLion / JetBrains IDEs
1. Go to Settings → Languages & Frameworks → C/C++ → Cppcheck
2. Enable Cppcheck
3. Set options:
   - Language: C++
   - Standard: C++17
   - Include paths: `-Ilibs -Isrc`
   - Exclude directories: external, build, test123

#### VSCode
Install cppcheck extension and configure in `.vscode/settings.json`:

```json
{
  "cppcheck.cppcheckArgs": ["--std=c++17", "--language=c++"],
  "cppcheck.includePaths": ["libs", "src"],
  "cppcheck.excludedPaths": ["external", "build", "test123"]
}
```

## Build Instructions

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

## Library Modules

### imgproc (Header-only)
Image processing components:
- `Frame<T>` - Generic frame/image container
- `Kernel<N>` - Convolution kernel
- `Convolution` - Convolution filter application

Pure C++ without Qt dependencies - can be used independently.

### segcore (Static library)
Annotation and project management:
- `AnnotationSet` - Manages polygon annotations with undo/redo
- `Artifact` - Image data container (supports multiple formats)
- `Segment` - Single polygon definition
- `Geometry` - Point-in-polygon, hit testing utilities
- `Project` - Multi-image annotation project management
- `NormalizedFormatSerializer` - Format export/import

Pure C++ without Qt dependencies - can be used independently.

## Testing

```bash
# Run all tests
ctest -j$(nproc) --output-on-failure

# Run specific test
ctest -R segcore_unit_tests --output-on-failure

# Run with verbose output
ctest -VV
```

## Commit Message Format

Follow Conventional Commits:

```
type(scope): subject

body explaining why

footer with references
```

Types: feat, fix, refactor, docs, test, chore, style

Example:
```
refactor(libs): extract image processing to imgproc module

Separate image manipulation code into header-only imgproc library
to enable reuse without Qt dependencies.
```
