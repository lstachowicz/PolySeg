#include <segcore/project.h>

namespace segcore {

ArtifactId Project::AddArtifact(Artifact artifact)
{
  ArtifactId id = next_id_++;
  artifact.id = id;
  artifacts_.emplace(id, std::move(artifact));
  annotations_.emplace(id, AnnotationSet{});
  return id;
}

void Project::RemoveArtifact(ArtifactId id)
{
  artifacts_.erase(id);
  annotations_.erase(id);
  if (current_id_ == id)
  {
    current_id_ = kInvalidArtifactId;
  }
}

const Artifact* Project::GetArtifact(ArtifactId id) const
{
  auto it = artifacts_.find(id);
  return it != artifacts_.end() ? &it->second : nullptr;
}

AnnotationSet& Project::GetAnnotations(ArtifactId id)
{
  return annotations_.at(id);
}

const AnnotationSet& Project::GetAnnotations(ArtifactId id) const
{
  return annotations_.at(id);
}

ArtifactId Project::GetCurrentArtifactId() const
{
  return current_id_;
}

void Project::SetCurrentArtifact(ArtifactId id)
{
  current_id_ = id;
}

}  // namespace segcore
