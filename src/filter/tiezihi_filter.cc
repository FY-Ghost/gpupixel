/*
 * GPUPixel
 *
 * Created by PixPark on 2021/6/24.
 * Copyright © 2021 PixPark. All rights reserved.
 */

#include "gpupixel/filter/tiezihi_filter.h"
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include "core/gpupixel_context.h"

namespace gpupixel {

const std::string kTieZhiVertexShaderString = R"(
    attribute vec2 position;
    attribute vec4 inputTextureCoordinate;
    uniform mat4 model;
    uniform mat4 projection; 
    varying vec2 textureCoordinate;
    void main() {
        gl_Position = projection * model * vec4(position, 0.0, 1.0);
        textureCoordinate = inputTextureCoordinate.xy;
    })";

const std::string kTieZhiFragmentShaderString = R"(
    uniform sampler2D inputImageTexture;
    varying vec2 textureCoordinate;
    uniform float test;
    void main() {
    if(test > 0.5) {
        gl_FragColor = texture2D(inputImageTexture, textureCoordinate);
    } else {
        gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
    })";

std::shared_ptr<TieZhiFilter> TieZhiFilter::Create() {
  auto ret = std::shared_ptr<TieZhiFilter>(new TieZhiFilter());
  gpupixel::GPUPixelContext::GetInstance()->SyncRunWithContext([&] {
    if (ret && !ret->Init()) {
      ret.reset();
    }
  });
  return ret;
}

bool TieZhiFilter::Init() {
  if (!InitWithShaderString(kTieZhiVertexShaderString,
                            kTieZhiFragmentShaderString)) {
    return false;
  }

  source_image_ = SourceImage::Create("/Users/admin/Documents/logo.png");

  return true;
}

bool TieZhiFilter::DoRender(bool updateSinks) {
  static const float image_vertices[] = {
      -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
  };

  GPUPixelContext::GetInstance()->SetActiveGlProgram(filter_program_);
  framebuffer_->Activate();
  GL_CALL(glClearColor(background_color_.r, background_color_.g,
                       background_color_.b, background_color_.a));
  GL_CALL(glClear(GL_COLOR_BUFFER_BIT));

  // 启用混合模式
  GL_CALL(glEnable(GL_BLEND));
  GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

  // 设置project_matrix uniform
  filter_program_->SetUniformValue("projection", glm::mat4(1.0f));
  filter_program_->SetUniformValue("model", glm::mat4(1.0f));

  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glBindTexture(GL_TEXTURE_2D,
                        input_framebuffers_[0].frame_buffer->GetTexture()));
  filter_program_->SetUniformValue("inputImageTexture", 0);
  filter_program_->SetUniformValue("test", 1.0f);
  // texcoord attribute
  uint32_t filter_tex_coord_attribute =
      filter_program_->GetAttribLocation("inputTextureCoordinate");
  GL_CALL(glEnableVertexAttribArray(filter_tex_coord_attribute));
  GL_CALL(glVertexAttribPointer(filter_tex_coord_attribute, 2, GL_FLOAT, 0, 0,
                                GetTextureCoordinate(NoRotation)));

  GL_CALL(glVertexAttribPointer(filter_position_attribute_, 2, GL_FLOAT, 0, 0,
                                image_vertices));
  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

  // 根据宽高比创建缩放矩阵
  float position_x = -0.1f;
  float position_y = -0.7f;

  float large_side =
      std::max(source_image_->GetWidth(), source_image_->GetHeight());
  float scale_x = source_image_->GetWidth() / large_side * 0.3f;
  float scale_y = source_image_->GetHeight() / large_side * 0.3f;

  float aspect = (float)framebuffer_->GetWidth() / framebuffer_->GetHeight();

  if (aspect > 1.0f) {
    scale_x /= aspect;
  } else {
    scale_y *= aspect;
  }
  glm::mat4 scale_matrix = glm::scale(glm::vec3(0.12f, 0.24f, 1.0f));

  float tr_x = 0.0f;
  float tr_y = 0.0f;

  if (face_landmarks_.size() > 0) {
    float x = face_landmarks_[43 * 2];      // x
    float y = face_landmarks_[43 * 2 + 1];  // y
    tr_x = x * 2 - 1;
    tr_y = y * 2 - 1;
  }
  // 创建平移矩阵
  glm::mat4 translation_matrix = glm::translate(glm::vec3(tr_x, tr_y, 0.0f));

  LOG_INFO("pitch_ = {}, yaw_ = {}, roll_ = {}", pitch_, yaw_, roll_);
  // 将旋转矩阵和缩放矩阵相乘
  // z
  static float angle = 0.0f;
  angle += 1.0f;
  scale_matrix = glm::rotate(scale_matrix, glm::radians(angle),
                             glm::vec3(0.0f, 0.0f, 1.0f));
  // 组合所有变换矩阵，注意变换顺序：先缩放，再旋转，最后平移
  // glm::mat4 final_matrix = translation_matrix * roll_matrix * scale_matrix;
  glm::mat4 final_matrix = translation_matrix * scale_matrix;

  // 设置project_matrix uniform

  // glm::mat4 projection =
  //     glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

  float viewHeight = 720.0f;
  float viewWidth = viewHeight * aspect;

  // 修改为与 NDC 空间匹配的正交投影
  float rt = (float)framebuffer_->GetHeight() / framebuffer_->GetWidth();

  filter_program_->SetUniformValue("test", 0.0f);
  glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -rt, rt, -1.0f, 1.0f);
  filter_program_->SetUniformValue("projection", projection);
  filter_program_->SetUniformValue("model", scale_matrix);

  // 设置叠加纹理
  GL_CALL(glBindTexture(GL_TEXTURE_2D,
                        source_image_->GetFramebuffer()->GetTexture()));

  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

  framebuffer_->Deactivate();

  return Source::DoRender(updateSinks);
}

void TieZhiFilter::SetAngle(float pitch, float yaw, float roll) {
  pitch_ = pitch;
  yaw_ = yaw;
  roll_ = roll;
}

glm::mat4 TieZhiFilter::CreateRotationMatrix(float angle,
                                             float x,
                                             float y,
                                             float z) {
  return glm::rotate(angle, glm::vec3(x, y, z));
}

void TieZhiFilter::SetLandmark(const std::vector<float>& landmarks) {
  face_landmarks_ = landmarks;
}

}  // namespace gpupixel
