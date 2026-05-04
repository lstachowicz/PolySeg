#ifndef POLYSEG_IMGPROC_CONVOLUTION_H
#define POLYSEG_IMGPROC_CONVOLUTION_H

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>

#include <imgproc/frame.h>
#include <imgproc/kernel.h>

namespace polyseg {

class Convolution
{
 public:
  template <int N, typename T>
  static auto apply(Frame<T>& output, const Frame<T>& input, const Kernel<N>& kernel) -> void
  {
    assert(output.width() == input.width());
    assert(output.height() == input.height());
    detail<N, T>::apply(output, input, kernel);
  }

 private:
  template <int N, typename T>
  struct detail
  {
    static auto apply(Frame<T>& output, const Frame<T>& input, const Kernel<N>& kernel) -> void
    {
      const auto* k = kernel.data().data();
      const auto* src = input.data();
      const int width = input.width();
      const int height = input.height();
      const double factor = kernel.factor();
      const int border = N / 2;

      for (int y = border; y < height - border; ++y)
      {
        for (int x = border; x < width - border; ++x)
        {
          double sum = 0.0;
          for (int ky = 0; ky < N; ++ky)
          {
            for (int kx = 0; kx < N; ++kx)
            {
              sum += k[ky * N + kx] * src[(y + ky - border) * width + (x + kx - border)];
            }
          }
          sum = std::max(0.0, std::min(static_cast<double>(std::numeric_limits<T>::max()),
                                       sum * factor));
          output.data()[y * width + x] = static_cast<T>(sum);
        }
      }
    }
  };
};

// Specialization for 3x3 kernels

template <typename T>
struct Convolution::detail<3, T>
{
  static auto apply(Frame<T>& output, const Frame<T>& input, const Kernel<3>& kernel) -> void
  {
    const int width = input.width();
    const int height = input.height();

    const auto& kdata = kernel.data();
    const float factor = kernel.factor();
    const float kf[9] = {
        kdata[0] * factor, kdata[1] * factor, kdata[2] * factor,
        kdata[3] * factor, kdata[4] * factor, kdata[5] * factor,
        kdata[6] * factor, kdata[7] * factor, kdata[8] * factor,
    };

    const auto* r0 = input.data();
    const auto* r1 = input.data() + width;
    const auto* r2 = input.data() + width * 2;
    T* dst = output.data() + width;

    for (int i = 1; i < height - 1; ++i)
    {
      ++r0;
      ++r1;
      ++r2;
      ++dst;
      for (int j = 1; j < width - 1; ++j)
      {
        float value = kf[0] * r0[-1] + kf[1] * r0[0] + kf[2] * r0[1] + kf[3] * r1[-1] +
                      kf[4] * r1[0] + kf[5] * r1[1] + kf[6] * r2[-1] + kf[7] * r2[0] +
                      kf[8] * r2[1];
        value = std::max(0.0f, std::min(static_cast<float>(std::numeric_limits<T>::max()), value));
        *dst++ = static_cast<T>(value);
        ++r0;
        ++r1;
        ++r2;
      }
      ++r0;
      ++r1;
      ++r2;
      ++dst;
    }
  }
};

// Specialization for 5x5 kernels

template <typename T>
struct Convolution::detail<5, T>
{
  static auto apply(Frame<T>& output, const Frame<T>& input, const Kernel<5>& kernel) -> void
  {
    const int width = input.width();
    const int height = input.height();

    const auto& kdata = kernel.data();
    const float factor = kernel.factor();
    const float kf[25] = {
        kdata[0] * factor,  kdata[1] * factor,  kdata[2] * factor,  kdata[3] * factor,
        kdata[4] * factor,  kdata[5] * factor,  kdata[6] * factor,  kdata[7] * factor,
        kdata[8] * factor,  kdata[9] * factor,  kdata[10] * factor, kdata[11] * factor,
        kdata[12] * factor, kdata[13] * factor, kdata[14] * factor, kdata[15] * factor,
        kdata[16] * factor, kdata[17] * factor, kdata[18] * factor, kdata[19] * factor,
        kdata[20] * factor, kdata[21] * factor, kdata[22] * factor, kdata[23] * factor,
        kdata[24] * factor,
    };

    const auto* r0 = input.data();
    const auto* r1 = input.data() + width;
    const auto* r2 = input.data() + width * 2;
    const auto* r3 = input.data() + width * 3;
    const auto* r4 = input.data() + width * 4;
    T* dst = output.data() + width * 2;

    for (int i = 2; i < height - 2; ++i)
    {
      r0 += 2;
      r1 += 2;
      r2 += 2;
      r3 += 2;
      r4 += 2;
      dst += 2;
      for (int j = 2; j < width - 2; ++j)
      {
        float value =
            kf[0] * r0[-2] + kf[1] * r0[-1] + kf[2] * r0[0] + kf[3] * r0[1] + kf[4] * r0[2] +
            kf[5] * r1[-2] + kf[6] * r1[-1] + kf[7] * r1[0] + kf[8] * r1[1] + kf[9] * r1[2] +
            kf[10] * r2[-2] + kf[11] * r2[-1] + kf[12] * r2[0] + kf[13] * r2[1] +
            kf[14] * r2[2] + kf[15] * r3[-2] + kf[16] * r3[-1] + kf[17] * r3[0] +
            kf[18] * r3[1] + kf[19] * r3[2] + kf[20] * r4[-2] + kf[21] * r4[-1] +
            kf[22] * r4[0] + kf[23] * r4[1] + kf[24] * r4[2];
        value = std::max(0.0f, std::min(static_cast<float>(std::numeric_limits<T>::max()), value));
        *dst++ = static_cast<T>(value);
        ++r0;
        ++r1;
        ++r2;
        ++r3;
        ++r4;
      }
      r0 += 2;
      r1 += 2;
      r2 += 2;
      r3 += 2;
      r4 += 2;
      dst += 2;
    }
  }
};

}  // namespace polyseg

#endif  // POLYSEG_IMGPROC_CONVOLUTION_H
