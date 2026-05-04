#ifndef POLYSEG_IMGPROC_KERNEL_H
#define POLYSEG_IMGPROC_KERNEL_H

#include <array>

namespace polyseg {

template <int N>
class Kernel
{
 public:
  Kernel(const std::array<float, N * N>& data, float factor = 1.0f)
      : data_(data), factor_(factor)
  {
  }

  [[nodiscard]] constexpr int size() const { return N; }
  [[nodiscard]] constexpr int width() const { return N; }
  [[nodiscard]] constexpr int height() const { return N; }

  [[nodiscard]] const std::array<float, N * N>& data() const { return data_; }
  [[nodiscard]] float factor() const { return factor_; }

 protected:
  std::array<float, N * N> data_;
  float factor_;
};

}  // namespace polyseg

#endif  // POLYSEG_IMGPROC_KERNEL_H
