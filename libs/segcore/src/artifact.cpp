#include <segcore/artifact.h>

namespace segcore
{

int Artifact::width() const
{
  return std::visit(
      [](const auto& v) -> int
      {
        if constexpr (std::is_same_v<std::decay_t<decltype(v)>, ImageArtifact>)
        {
          return v.pixels.width();
        }
        else
        {
          return v.data.width();
        }
      },
      payload);
}

int Artifact::height() const
{
  return std::visit(
      [](const auto& v) -> int
      {
        if constexpr (std::is_same_v<std::decay_t<decltype(v)>, ImageArtifact>)
        {
          return v.pixels.height();
        }
        else
        {
          return v.data.height();
        }
      },
      payload);
}

}  // namespace segcore
