/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/lipstick_filter.h"
#include "core/gpupixel_context.h"
#include "gpupixel/source/source_image.h"
#include "utils/util.h"
namespace gpupixel {

LipstickFilter::LipstickFilter() {}

std::shared_ptr<LipstickFilter> LipstickFilter::Create() {
  auto ret = std::shared_ptr<LipstickFilter>(new LipstickFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    }
  });
  return ret;
}

bool LipstickFilter::Init() {
  // 不再在这里直接创建 SourceImage，而是等待外部通过 SetMouthTexture 传入
  SetTextureBounds(FrameBounds{502.5, 710, 262.5, 167.5});
  return FaceMakeupFilter::Init();
}

void LipstickFilter::SetMouthTexture(
    std::shared_ptr<SourceImage> mouth_texture) {
  SetImageTexture(mouth_texture);
}

}  // namespace gpupixel
