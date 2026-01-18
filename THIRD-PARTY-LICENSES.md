# Third-Party Licenses

PolySeg uses the following third-party software components:

## Qt Framework

**Version:** 6.8.0  
**License:** GNU Lesser General Public License v3 (LGPL v3)  
**Copyright:** The Qt Company Ltd.  
**Website:** https://www.qt.io/  
**Source Code:** https://code.qt.io/cgit/qt/

### License Summary

Qt is dynamically linked to this application. Under the terms of LGPL v3, users are free to:
- Use this application for any purpose (commercial or non-commercial)
- Replace Qt libraries with compatible versions
- Modify Qt libraries (if desired) following LGPL v3 terms

### LGPL v3 Full Text

The complete LGPL v3 license text is available at:
- https://www.gnu.org/licenses/lgpl-3.0.html
- Local Qt installation: `/usr/share/doc/qt6/LGPL_EXCEPTION.txt`

### Qt Components Used

This application uses the following Qt modules:
- **Qt Core** - Core non-GUI functionality
- **Qt Widgets** - Desktop widget toolkit
- **Qt GUI** - Base classes for graphical user interface components

All Qt libraries are dynamically linked (`.so` files on Linux, `.dll` on Windows, `.dylib` on macOS).

---

## Standard C++ Library

**License:** Part of the compiler toolchain (typically GPL with GCC Runtime Library Exception)  
**Note:** Dynamic linking ensures no license restrictions on this application

---

## Build Tools (Development Only)

The following tools are used during development but are not distributed with the application:

- **CMake / qmake** - Build system configuration
- **clang-format** - Code formatting (University of Illinois/NCSA Open Source License)
- **clang-tidy** - Static analysis (University of Illinois/NCSA Open Source License)

---

## License Compatibility

PolySeg's MIT License is fully compatible with Qt's LGPL v3 license through dynamic linking. This combination allows:

- PolySeg source code remains MIT  
- Qt libraries remain LGPL  
- Users can freely use, modify, and distribute PolySeg  
- Users can replace Qt libraries with different versions  

---

## Compliance

To comply with LGPL v3 requirements:

1. **Dynamic Linking:** Qt is linked dynamically (verified with `ldd` on Linux)
2. **User Modification:** Users can replace Qt libraries in their local installation
3. **License Notice:** This file provides notice of Qt usage and its license
4. **Source Availability:** Qt source code is publicly available at https://code.qt.io/

---

## Additional Notes

### Optional Plugin Dependencies

If you use AI plugins with this application, those plugins may have their own licenses:

- **YOLACT** - MIT License (compatible)
- **SAM (Segment Anything)** - Apache 2.0 (compatible)
- **Detectron2** - Apache 2.0 (compatible)
- **SMP (Segmentation Models PyTorch)** - MIT License (compatible)

Plugins are separate processes (subprocess communication) and do not affect PolySeg's licensing.

---

**Last Updated:** January 12, 2026
