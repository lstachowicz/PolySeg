#ifndef POLYSEG_IMGPROC_CONVOLUTION_KERNELS_H
#define POLYSEG_IMGPROC_CONVOLUTION_KERNELS_H

#include <array>

#include <imgproc/kernel.h>

namespace polyseg {

// Base for Laplacian-of-Gaussian kernels
template <int N>
class LaplacianKernel : public Kernel<N>
{
 public:
  LaplacianKernel(const std::array<float, N * N>& data, float factor = 1.0f)
      : Kernel<N>(data, factor)
  {
  }
};

// 3x3 discrete Laplacian
class LaplacianKernel3 : public LaplacianKernel<3>
{
 public:
  LaplacianKernel3()
      : LaplacianKernel<3>({
            0.0f, -1.0f, 0.0f,
            -1.0f, 4.0f, -1.0f,
            0.0f, -1.0f, 0.0f,
        })
  {
  }
};

// 5x5 Laplacian-of-Gaussian approximation
class LaplacianKernel5 : public LaplacianKernel<5>
{
 public:
  LaplacianKernel5()
      : LaplacianKernel<5>({
             0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
             0.0f, -1.0f, -2.0f, -1.0f,  0.0f,
            -1.0f, -2.0f, 16.0f, -2.0f, -1.0f,
             0.0f, -1.0f, -2.0f, -1.0f,  0.0f,
             0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        })
  {
  }
};

// 5x5 Gaussian blur (sigma ~1.0, sum = 159)
class GaussianKernel5 : public Kernel<5>
{
 public:
  GaussianKernel5()
      : Kernel<5>(
            {
                 2.0f,  4.0f,  5.0f,  4.0f,  2.0f,
                 4.0f,  9.0f, 12.0f,  9.0f,  4.0f,
                 5.0f, 12.0f, 15.0f, 12.0f,  5.0f,
                 4.0f,  9.0f, 12.0f,  9.0f,  4.0f,
                 2.0f,  4.0f,  5.0f,  4.0f,  2.0f,
            },
            1.0f / 159.0f)
  {
  }
};

}  // namespace polyseg

#endif  // POLYSEG_IMGPROC_CONVOLUTION_KERNELS_H
