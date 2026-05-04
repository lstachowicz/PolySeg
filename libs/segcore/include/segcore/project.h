#ifndef POLYSEG_SEGCORE_PROJECT_H
#define POLYSEG_SEGCORE_PROJECT_H

#include <segcore/annotation_set.h>
#include <segcore/artifact.h>
#include <segcore/types.h>

#include <unordered_map>

namespace segcore
{

class Project
{
 public:
  ArtifactId AddArtifact(Artifact artifact);
  void RemoveArtifact(ArtifactId id);
  const Artifact* GetArtifact(ArtifactId id) const;
  AnnotationSet& GetAnnotations(ArtifactId id);
  const AnnotationSet& GetAnnotations(ArtifactId id) const;
  ArtifactId GetCurrentArtifactId() const;
  void SetCurrentArtifact(ArtifactId id);

 private:
  ArtifactId next_id_ = 1;
  ArtifactId current_id_ = kInvalidArtifactId;
  std::unordered_map<ArtifactId, Artifact> artifacts_;
  std::unordered_map<ArtifactId, AnnotationSet> annotations_;
};

}  // namespace segcore

#endif  // POLYSEG_SEGCORE_PROJECT_H
