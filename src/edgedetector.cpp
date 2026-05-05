#include "edgedetector.h"

#include <imgproc/convolution.h>
#include <imgproc/convolution_kernels.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "logger.h"

namespace {

polyseg::Frame<uint8_t> ToGrayscaleFrame(const QImage& image)
{
  QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
  const auto w = static_cast<uint16_t>(gray.width());
  const auto h = static_cast<uint16_t>(gray.height());
  polyseg::Frame<uint8_t> frame(w, h);
  for (int y = 0; y < gray.height(); ++y)
  {
    const uchar* row = gray.constScanLine(y);
    for (int x = 0; x < gray.width(); ++x)
    {
      frame.data()[y * w + x] = row[x];
    }
  }
  return frame;
}

// Applies LoG kernel without clamping negatives; returns absolute response.
polyseg::Frame<float> ApplyLogAbs(const polyseg::Frame<uint8_t>& input)
{
  const polyseg::LaplacianKernel5 kernel;
  const auto& kdata = kernel.data();
  const int w = input.width();
  const int h = input.height();
  polyseg::Frame<float> output(static_cast<uint16_t>(w), static_cast<uint16_t>(h), 0.0f);

  for (int y = 2; y < h - 2; ++y)
  {
    for (int x = 2; x < w - 2; ++x)
    {
      float sum = 0.0f;
      for (int ky = 0; ky < 5; ++ky)
      {
        for (int kx = 0; kx < 5; ++kx)
        {
          sum += kdata[ky * 5 + kx] *
                 static_cast<float>(input.data()[(y + ky - 2) * w + (x + kx - 2)]);
        }
      }
      output.data()[y * w + x] = std::abs(sum);
    }
  }
  return output;
}

uint8_t OtsuThreshold(const uint8_t* data, size_t n)
{
  std::array<int, 256> hist = {};
  for (size_t i = 0; i < n; ++i) ++hist[data[i]];

  double total_sum = 0.0;
  for (int i = 0; i < 256; ++i) total_sum += i * hist[i];

  double sum_bg = 0.0;
  int w_bg = 0;
  double best_var = 0.0;
  int best_t = 0;
  const int N = static_cast<int>(n);

  for (int t = 0; t < 256; ++t)
  {
    w_bg += hist[t];
    if (w_bg == 0 || w_bg == N)
    {
      sum_bg += t * hist[t];
      continue;
    }
    sum_bg += t * hist[t];
    double mean_bg = sum_bg / w_bg;
    double mean_fg = (total_sum - sum_bg) / (N - w_bg);
    double diff = mean_bg - mean_fg;
    double var = static_cast<double>(w_bg) * static_cast<double>(N - w_bg) * diff * diff;
    if (var > best_var)
    {
      best_var = var;
      best_t = t;
    }
  }
  return static_cast<uint8_t>(best_t);
}

std::vector<uint8_t> Threshold(const polyseg::Frame<float>& log_abs)
{
  const size_t n = log_abs.size();
  const float* src = log_abs.data();

  float max_val = 0.0f;
  for (size_t i = 0; i < n; ++i) max_val = std::max(max_val, src[i]);

  std::vector<uint8_t> norm(n, 0);
  if (max_val < 1e-6f) return std::vector<uint8_t>(n, 0);

  const float scale = 255.0f / max_val;
  for (size_t i = 0; i < n; ++i)
    norm[i] = static_cast<uint8_t>(std::min(255.0f, src[i] * scale));

  const uint8_t otsu = OtsuThreshold(norm.data(), n);
  const uint8_t t = std::max(static_cast<uint8_t>(otsu * 65 / 100), uint8_t{8});
  spdlog::debug("[EdgeDetect] max_log={} otsu={} threshold={}", max_val, static_cast<int>(otsu),
               static_cast<int>(t));

  std::vector<uint8_t> edges(n, 0);
  for (size_t i = 0; i < n; ++i) edges[i] = (norm[i] >= t) ? 1 : 0;
  return edges;
}

// Removes edge pixels with fewer than 1 edge neighbor (isolated single pixels).
void Denoise(std::vector<uint8_t>& edges, int w, int h)
{
  std::vector<uint8_t> result = edges;
  for (int y = 1; y < h - 1; ++y)
  {
    for (int x = 1; x < w - 1; ++x)
    {
      if (!edges[y * w + x]) continue;
      int count = 0;
      for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
          if ((dy || dx) && edges[(y + dy) * w + (x + dx)]) ++count;
      if (count < 1) result[y * w + x] = 0;
    }
  }
  edges = std::move(result);
}

// Zhang-Suen thinning: reduces edge map to 1-pixel skeleton.
void ThinZhangSuen(std::vector<uint8_t>& img, int w, int h)
{
  auto get = [&](int x, int y) -> int {
    if (x < 0 || x >= w || y < 0 || y >= h) return 0;
    return img[y * w + x] ? 1 : 0;
  };

  bool changed = true;
  std::vector<int> to_remove;
  to_remove.reserve(4096);

  while (changed)
  {
    changed = false;

    // Sub-iteration 1
    for (int y = 1; y < h - 1; ++y)
    {
      for (int x = 1; x < w - 1; ++x)
      {
        if (!img[y * w + x]) continue;
        int p2 = get(x, y - 1), p3 = get(x + 1, y - 1);
        int p4 = get(x + 1, y), p5 = get(x + 1, y + 1);
        int p6 = get(x, y + 1), p7 = get(x - 1, y + 1);
        int p8 = get(x - 1, y), p9 = get(x - 1, y - 1);
        int B = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
        if (B < 2 || B > 6) continue;
        int seq[8] = {p2, p3, p4, p5, p6, p7, p8, p9};
        int A = 0;
        for (int i = 0; i < 8; ++i)
          if (seq[i] == 0 && seq[(i + 1) % 8] == 1) ++A;
        if (A != 1) continue;
        if (p2 * p4 * p6 != 0) continue;
        if (p4 * p6 * p8 != 0) continue;
        to_remove.push_back(y * w + x);
      }
    }
    for (int idx : to_remove)
    {
      img[idx] = 0;
      changed = true;
    }
    to_remove.clear();

    // Sub-iteration 2
    for (int y = 1; y < h - 1; ++y)
    {
      for (int x = 1; x < w - 1; ++x)
      {
        if (!img[y * w + x]) continue;
        int p2 = get(x, y - 1), p3 = get(x + 1, y - 1);
        int p4 = get(x + 1, y), p5 = get(x + 1, y + 1);
        int p6 = get(x, y + 1), p7 = get(x - 1, y + 1);
        int p8 = get(x - 1, y), p9 = get(x - 1, y - 1);
        int B = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
        if (B < 2 || B > 6) continue;
        int seq[8] = {p2, p3, p4, p5, p6, p7, p8, p9};
        int A = 0;
        for (int i = 0; i < 8; ++i)
          if (seq[i] == 0 && seq[(i + 1) % 8] == 1) ++A;
        if (A != 1) continue;
        if (p2 * p4 * p8 != 0) continue;
        if (p2 * p6 * p8 != 0) continue;
        to_remove.push_back(y * w + x);
      }
    }
    for (int idx : to_remove)
    {
      img[idx] = 0;
      changed = true;
    }
    to_remove.clear();
  }
}

QImage CreateOverlay(const std::vector<uint8_t>& edges, int w, int h)
{
  QImage overlay(w, h, QImage::Format_ARGB32);
  overlay.fill(Qt::transparent);
  for (int y = 0; y < h; ++y)
  {
    QRgb* line = reinterpret_cast<QRgb*>(overlay.scanLine(y));
    for (int x = 0; x < w; ++x)
    {
      if (edges[y * w + x]) line[x] = qRgba(255, 255, 0, 230);
    }
  }
  return overlay;
}

}  // namespace

EdgeDetector::Result EdgeDetector::Detect(const QImage& image)
{
  if (image.isNull()) return {};

  // Grayscale conversion
  polyseg::Frame<uint8_t> gray = ToGrayscaleFrame(image);
  const int w = gray.width();
  const int h = gray.height();

  // Gaussian 5x5 blur (denoising before LoG)
  polyseg::Frame<uint8_t> blurred(static_cast<uint16_t>(w), static_cast<uint16_t>(h), uint8_t{0});
  polyseg::Convolution::apply(blurred, gray, polyseg::GaussianKernel5{});

  // Replicate border pixels: Gaussian skips 2px border, leaving it at 0.
  // Those zeros would produce fake strong LoG responses at image boundary.
  for (int y = 0; y < h; ++y)
  {
    for (int x = 0; x < w; ++x)
    {
      if (x >= 2 && x < w - 2 && y >= 2 && y < h - 2) continue;
      const int sx = std::clamp(x, 2, w - 3);
      const int sy = std::clamp(y, 2, h - 3);
      blurred.data()[y * w + x] = blurred.data()[sy * w + sx];
    }
  }

  // LoG 5x5 (absolute response)
  polyseg::Frame<float> log_response = ApplyLogAbs(blurred);

  // Threshold via Otsu on normalized response
  std::vector<uint8_t> edges = Threshold(log_response);

  // Denoise: remove isolated edge pixels
  Denoise(edges, w, h);

  // Zhang-Suen thinning to 1-pixel skeleton
  ThinZhangSuen(edges, w, h);

  Result result;
  result.width = w;
  result.height = h;
  result.edge_map = std::move(edges);
  result.overlay = CreateOverlay(result.edge_map, w, h);
  return result;
}

QPoint EdgeDetector::SnapToEdge(const QPoint& img_pos, const Result& edges, int radius)
{
  if (!edges.isValid()) return img_pos;

  int best_dist_sq = radius * radius + 1;
  QPoint best = img_pos;

  for (int dy = -radius; dy <= radius; ++dy)
  {
    for (int dx = -radius; dx <= radius; ++dx)
    {
      const int nx = img_pos.x() + dx;
      const int ny = img_pos.y() + dy;
      if (nx < 0 || nx >= edges.width || ny < 0 || ny >= edges.height) continue;
      if (!edges.edge_map[ny * edges.width + nx]) continue;
      const int dist_sq = dx * dx + dy * dy;
      if (dist_sq < best_dist_sq)
      {
        best_dist_sq = dist_sq;
        best = QPoint(nx, ny);
      }
    }
  }
  return best;
}
