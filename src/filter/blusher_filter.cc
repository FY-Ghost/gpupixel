/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/blusher_filter.h"
#include "core/gpupixel_context.h"
#include "gpupixel/source/source_image.h"
#include "utils/util.h"
namespace gpupixel {

BlusherFilter::BlusherFilter() {}

std::shared_ptr<BlusherFilter> BlusherFilter::Create() {
  auto ret = std::shared_ptr<BlusherFilter>(new BlusherFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    }
  });
  return ret;
}

bool BlusherFilter::Init() {
  // 不再在这里直接创建 SourceImage，而是等待外部通过 SetBlusherTexture 传入
  SetTextureBounds(FrameBounds{395, 520, 489, 209});
  return FaceMakeupFilter::Init();
}

void BlusherFilter::SetBlusherTexture(
    std::shared_ptr<SourceImage> blusher_texture) {
  SetImageTexture(blusher_texture);
}

}  // namespace gpupixel
