/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/hsb_filter.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/gpupixel_context.h"
namespace gpupixel {

/* Matrix algorithms adapted from
 http://www.graficaobscura.com/matrix/index.html

 Note about luminance vector values below from that page:
 Where rwgt is 0.3086, gwgt is 0.6094, and bwgt is 0.0820. This is the luminance
 vector. Notice here that we do not use the standard NTSC weights of 0.299,
 0.587, and 0.114. The NTSC weights are only applicable to RGB colors in a
 gamma 2.2 color space. For linear RGB colors the values above are better.
 */
// #define RLUM (0.3086f)
// #define GLUM (0.6094f)
// #define BLUM (0.0820f)

/* This is the vector value from the PDF specification, and may be closer to
 * what Photoshop uses */
#define RLUM (0.3f)
#define GLUM (0.59f)
#define BLUM (0.11f)

std::shared_ptr<HSBFilter> HSBFilter::Create() {
  auto ret = std::shared_ptr<HSBFilter>(new HSBFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    }
  });
  return ret;
}

bool HSBFilter::Init() {
  if (!ColorMatrixFilter::Init()) {
    return false;
  }
  reset();

  return true;
}

void HSBFilter::reset() {
  setColorMatrix(glm::mat4(1.0f));
}

void HSBFilter::rotateHue(float h) {
  // todo
}

void HSBFilter::adjustSaturation(float sat) {
  glm::mat4 sMat(1.0f);
  float rwgt = RLUM;
  float gwgt = GLUM;
  float bwgt = BLUM;

  sMat[0][0] = (1.0 - sat) * rwgt + sat;
  sMat[0][1] = (1.0 - sat) * rwgt;
  sMat[0][2] = (1.0 - sat) * rwgt;
  sMat[0][3] = 0.0;
  sMat[1][0] = (1.0 - sat) * gwgt;
  sMat[1][1] = (1.0 - sat) * gwgt + sat;
  sMat[1][2] = (1.0 - sat) * gwgt;
  sMat[1][3] = 0.0;
  sMat[2][0] = (1.0 - sat) * bwgt;
  sMat[2][1] = (1.0 - sat) * bwgt;
  sMat[2][2] = (1.0 - sat) * bwgt + sat;
  sMat[2][3] = 0.0;
  sMat[3][0] = 0.0;
  sMat[3][1] = 0.0;
  sMat[3][2] = 0.0;
  sMat[3][3] = 1.0;

  saturation_matrix_ = sMat;

  color_matrix_ = glm::mat4(1.0f);
  color_matrix_ *= brightness_matrix_;
  color_matrix_ = saturation_matrix_ * color_matrix_;
}

void HSBFilter::adjustBrightness(float b) {
  brightness_matrix_ = glm::mat4(1.0f);
  brightness_matrix_ *= b;

  color_matrix_ = glm::mat4(1.0f);
  color_matrix_ *= brightness_matrix_;
  color_matrix_ = saturation_matrix_ * color_matrix_;
}

}  // namespace gpupixel
