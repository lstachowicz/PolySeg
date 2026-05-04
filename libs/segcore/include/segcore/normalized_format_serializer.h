#ifndef POLYSEG_SEGCORE_NORMALIZED_FORMAT_SERIALIZER_H
#define POLYSEG_SEGCORE_NORMALIZED_FORMAT_SERIALIZER_H

#include <string>
#include <vector>
#include <segcore/segment.h>

namespace segcore {

std::string SegmentsToNormalizedFormat(const std::vector<Segment>& segs, int w, int h);
std::vector<Segment> NormalizedFormatToSegments(const std::string& text, int w, int h);

}  // namespace segcore

#endif  // POLYSEG_SEGCORE_NORMALIZED_FORMAT_SERIALIZER_H
