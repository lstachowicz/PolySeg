#include <segcore/display.h>

#include <algorithm>
#include <cstdint>

namespace segcore
{

polyseg::Frame<Rgb24> NormaliseForDisplay(const Artifact& artifact)
{
  return std::visit(
      [](const auto& v) -> polyseg::Frame<Rgb24>
      {
        if constexpr (std::is_same_v<std::decay_t<decltype(v)>, ImageArtifact>)
        {
          return v.pixels;
        }
        else
        {
          int w = v.data.width();
          int h = v.data.height();
          polyseg::Frame<Rgb24> out(static_cast<uint16_t>(w), static_cast<uint16_t>(h));
          const float* src = v.data.data();
          Rgb24* dst = out.data();
          float range = v.range_max - v.range_min;
          if (range < 1e-10f)
          {
            range = 1.0f;
          }
          for (int i = 0; i < w * h; ++i)
          {
            float normalized = (src[i] - v.range_min) / range;
            normalized = std::clamp(normalized, 0.0f, 1.0f);
            uint8_t byte = static_cast<uint8_t>(normalized * 255.0f);
            dst[i] = {byte, byte, byte};
          }
          return out;
        }
      },
      artifact.payload);
}

}  // namespace segcore
