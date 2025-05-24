/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>

#include "gpupixel/source/source.h"
namespace gpupixel {
class GPUPIXEL_API SourceImage : public Source {
 public:
  static std::shared_ptr<SourceImage> Create(const std::string name);

  // 从已解码的像素数据创建SourceImage
  static std::shared_ptr<SourceImage> CreateFromPixelData(
      int width,
      int height,
      int channel_count,
      const unsigned char* pixels);

  // 从编码的图像数据创建SourceImage（内部使用stbi_load_from_memory解码）
  static std::shared_ptr<SourceImage> CreateFromEncodedData(
      const uint8_t* image_data,
      size_t data_size);

  ~SourceImage() {};

  const unsigned char* GetRgbaImageBuffer() const;
  int GetWidth() const;
  int GetHeight() const;

  void Render();

  void Init(int width,
            int height,
            int channel_count,
            const unsigned char* pixels);

#if defined(GPUPIXEL_ANDROID)
  static std::shared_ptr<SourceImage> CreateImageForAndroid(std::string name);
#endif
  std::vector<unsigned char> image_bytes_;

 private:
  SourceImage() {}
};

}  // namespace gpupixel
