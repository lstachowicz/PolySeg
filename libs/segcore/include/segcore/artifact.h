#ifndef POLYSEG_SEGCORE_ARTIFACT_H
#define POLYSEG_SEGCORE_ARTIFACT_H

#include <imgproc/frame.h>
#include <segcore/types.h>

#include <string>
#include <variant>

namespace segcore
{

struct ImageArtifact
{
  polyseg::Frame<Rgb24> pixels;
  std::string source_path;
};

struct FloatArtifact
{
  polyseg::Frame<float> data;
  float range_min = 0.0f;
  float range_max = 1.0f;
  std::string source_path;
};

struct Artifact
{
  ArtifactId id = kInvalidArtifactId;
  std::variant<ImageArtifact, FloatArtifact> payload;

  int width() const;
  int height() const;
};

}  // namespace segcore

#endif  // POLYSEG_SEGCORE_ARTIFACT_H
