#ifndef POLYSEG_SEGCORE_NORMALIZED_FORMAT_SERIALIZER_H
#define POLYSEG_SEGCORE_NORMALIZED_FORMAT_SERIALIZER_H

#include <segcore/segment.h>

#include <string>
#include <vector>

namespace segcore
{

std::string SegmentsToNormalizedFormat(const std::vector<Segment>& segs, int w, int h);
std::vector<Segment> NormalizedFormatToSegments(const std::string& text, int w, int h);

}  // namespace segcore

#endif  // POLYSEG_SEGCORE_NORMALIZED_FORMAT_SERIALIZER_H
