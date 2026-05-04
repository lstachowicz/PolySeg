#include <segcore/normalized_format_serializer.h>

#include <iomanip>
#include <sstream>

namespace segcore
{

std::string SegmentsToNormalizedFormat(const std::vector<Segment>& segs, int w, int h)
{
  std::ostringstream out;
  out << std::fixed << std::setprecision(3);
  for (const auto& seg : segs)
  {
    if (seg.points.size() < 3)
    {
      continue;
    }
    out << seg.class_id;
    for (const auto& p : seg.points)
    {
      out << ' ' << static_cast<double>(p.x) / w << ' ' << static_cast<double>(p.y) / h;
    }
    out << '\n';
  }
  return out.str();
}

std::vector<Segment> NormalizedFormatToSegments(const std::string& text, int w, int h)
{
  std::vector<Segment> result;
  std::istringstream stream(text);
  std::string line;
  SegmentId next_id = 1;
  while (std::getline(stream, line))
  {
    if (line.empty())
    {
      continue;
    }
    std::istringstream ls(line);
    std::vector<std::string> tokens;
    std::string token;
    while (ls >> token)
    {
      tokens.push_back(token);
    }
    if (tokens.size() < 7)
    {
      continue;
    }
    Segment seg;
    seg.id = next_id++;
    seg.class_id = std::stoi(tokens[0]);
    for (size_t i = 1; i + 1 < tokens.size(); i += 2)
    {
      double nx = std::stod(tokens[i]);
      double ny = std::stod(tokens[i + 1]);
      seg.points.push_back({static_cast<int>(nx * w), static_cast<int>(ny * h)});
    }
    result.push_back(std::move(seg));
  }
  return result;
}

}  // namespace segcore
