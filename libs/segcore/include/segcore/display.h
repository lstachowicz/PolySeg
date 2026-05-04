#ifndef POLYSEG_SEGCORE_DISPLAY_H
#define POLYSEG_SEGCORE_DISPLAY_H

#include <imgproc/frame.h>
#include <segcore/artifact.h>
#include <segcore/types.h>

namespace segcore
{

polyseg::Frame<Rgb24> NormaliseForDisplay(const Artifact& artifact);

}  // namespace segcore

#endif  // POLYSEG_SEGCORE_DISPLAY_H
