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

class GPUPIXEL_API BlusherFilter : public FaceMakeupFilter {
 public:
  static std::shared_ptr<BlusherFilter> Create();
  bool Init() override;

  // 设置腮红纹理图像的接口
  void SetBlusherTexture(std::shared_ptr<SourceImage> blusher_texture);

 private:
  BlusherFilter();
};

}  // namespace gpupixel
