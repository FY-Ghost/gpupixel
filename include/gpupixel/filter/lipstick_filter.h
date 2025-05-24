/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#pragma once

#include "gpupixel/filter/face_makeup_filter.h"

namespace gpupixel {
class SourceImage;

class GPUPIXEL_API LipstickFilter : public FaceMakeupFilter {
 public:
  static std::shared_ptr<LipstickFilter> Create();

  bool Init() override;

  // 设置口红纹理图像的接口
  void SetMouthTexture(std::shared_ptr<SourceImage> mouth_texture);

 private:
  LipstickFilter();
};

}  // namespace gpupixel
