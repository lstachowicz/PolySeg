#ifndef POLYSEG_IMGPROC_FRAME_H
#define POLYSEG_IMGPROC_FRAME_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace polyseg {

template <typename T>
class Frame
{
 public:
  Frame() : width_(0), height_(0) {}

  Frame(uint16_t width, uint16_t height)
      : width_(width), height_(height), data_(width * height)
  {
  }

  Frame(uint16_t width, uint16_t height, const T* data)
      : width_(width), height_(height), data_(data, data + width * height)
  {
  }

  Frame(uint16_t width, uint16_t height, T fill)
      : width_(width), height_(height), data_(width * height, fill)
  {
  }

  [[nodiscard]] auto width() const -> int { return width_; }
  [[nodiscard]] auto height() const -> int { return height_; }
  [[nodiscard]] auto size() const -> size_t { return data_.size(); }

  auto data() -> T* { return data_.data(); }
  [[nodiscard]] auto data() const -> const T* { return data_.data(); }

 private:
  uint16_t width_;
  uint16_t height_;
  std::vector<T> data_;
};

}  // namespace polyseg

#endif  // POLYSEG_IMGPROC_FRAME_H
