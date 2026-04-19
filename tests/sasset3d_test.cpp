#include <gtest/gtest.h>

#include "NNL/nnl.hpp"
#include "test_common.hpp"

using namespace nnl;

static std::vector<u8> glb_naruto_buffer;
static std::vector<u8> glb_map_buffer;
static std::vector<u8> glb_map_collision_buffer;
static std::vector<u8> glb_map_shadow_buffer;

TEST(SAsset3D, ExportNaruto) {
  auto asset = asset::Import(GetPath("naruto"));

  SAsset3D sasset;

  sasset.extras["source"] = "TEST";

  sasset.model = model::Convert(model::Import(asset[asset::Asset3D::kModel]));

  sasset.textures = texture::Convert(texture::Import(asset[asset::Asset3D::kTextureContainer]));

  sasset.animations = animation::Convert(animation::Import(asset[asset::Asset3D::kAnimationContainer]));

  sasset.visibility_animations =
      visanimation::Convert(visanimation::Import(asset[asset::Asset3D::kVisanimationContainer]));

  ASSERT_EQ(sasset.model.meshes.size(), 55);
  ASSERT_EQ(sasset.textures.size(), 29);
  ASSERT_EQ(sasset.animations.size(), 67);
  ASSERT_EQ(sasset.visibility_animations.size(), sasset.animations.size());
  ASSERT_EQ(sasset.model.material_variants.size(), 12);
  ASSERT_EQ(sasset.model.materials.size(), 29);
  ASSERT_TRUE(!sasset.model.skeleton.roots.empty());

  auto action = action::Import(asset[asset::Asset3D::kActionConfig]);

  auto names = action::GetAnimationNames(action);

  for (std::size_t i = 0; i < sasset.animations.size(); i++) {
    auto& sanim = sasset.animations.at(i);

    if (!names[i].empty()) sanim.name += *names[i].begin();
  }

  glb_naruto_buffer = sasset.ExportGLB();
}

TEST(SAsset3D, ImportNaruto) {
  ASSERT_TRUE(!glb_naruto_buffer.empty());
  SAsset3D sasset = SAsset3D::Import(glb_naruto_buffer);

  ASSERT_EQ(sasset.model.meshes.size(), 55);
  ASSERT_EQ(sasset.textures.size(), 29);
  ASSERT_EQ(sasset.animations.size(), 67);
  ASSERT_EQ(sasset.visibility_animations.size(), sasset.animations.size());
  ASSERT_EQ(sasset.model.material_variants.size(), 12);
  ASSERT_EQ(sasset.model.materials.size(), 29);
  ASSERT_TRUE(!sasset.model.skeleton.roots.empty());
  ASSERT_TRUE(!sasset.extras.IsEmpty());

  model::ConvertParam conv_p;

  asset::Asset base_asset = asset::Import(GetPath("naruto"));

  base_asset.erase(asset::Asset3D::kVisanimationContainer);

  model::Model model = model::Convert(std::move(sasset.model), conv_p, true);

  base_asset[asset::Asset3D::kModel] = model::Export(model);

  base_asset[asset::Asset3D::kTextureContainer] = texture::Export(texture::Convert(std::move(sasset.textures)));

  animation::AnimationContainer anim = animation::Convert(std::move(sasset.animations));

  base_asset[asset::Asset3D::kAnimationContainer] = animation::Export(anim);

  base_asset[asset::Asset3D::kVisanimationContainer] =
      visanimation::Export(visanimation::Convert(std::move(sasset.visibility_animations)));
}

TEST(SAsset3D, ExportMapTower) {
  auto asset = asset::Import(GetPath("before_tower"));

  SAsset3D sasset;

  sasset.model = model::Convert(model::Import(asset[asset::Asset3D::kModel]));

  sasset.textures = texture::Convert(texture::Import(asset[asset::Asset3D::kTextureContainer]));
  ASSERT_EQ(sasset.model.meshes.size(), 304);
  ASSERT_EQ(sasset.textures.size(), 21);
  SAsset3D scol, sshadow;

  scol.model = collision::Convert(collision::Import(asset[asset::Asset3D::kCollision]));
  sshadow.model = shadow_collision::Convert(shadow_collision::Import(asset[asset::Asset3D::kShadowCollision]));

  ASSERT_EQ(scol.model.meshes.size(), 3);
  ASSERT_EQ(sshadow.model.meshes.size(), 1);

  glb_map_buffer = sasset.ExportGLB();
  glb_map_collision_buffer = scol.ExportGLB();
  glb_map_shadow_buffer = sshadow.ExportGLB();
}

TEST(SAsset3D, ImportMapTower) {
  ASSERT_TRUE(!glb_map_buffer.empty());
  ASSERT_TRUE(!glb_map_collision_buffer.empty());
  ASSERT_TRUE(!glb_map_shadow_buffer.empty());

  SAsset3D sasset = SAsset3D::Import(glb_map_buffer);
  SAsset3D scol = SAsset3D::Import(glb_map_collision_buffer);
  SAsset3D sshadow = SAsset3D::Import(glb_map_shadow_buffer);

  ASSERT_EQ(sasset.model.meshes.size(), 304);
  ASSERT_EQ(sasset.textures.size(), 21);
  ASSERT_EQ(scol.model.meshes.size(), 3);
  ASSERT_EQ(sshadow.model.meshes.size(), 1);

  asset::Asset asset;

  model::ConvertParam conv_p;

  conv_p.use_bbox = true;

  asset[asset::Asset3D::kModel] = model::Export(model::Convert(std::move(sasset.model), conv_p));
  asset[asset::Asset3D::kTextureContainer] = texture::Export(texture::Convert(std::move(sasset.textures)));
  asset[asset::Asset3D::kCollision] = collision::Export(collision::Convert(std::move(scol.model)));
  asset[asset::Asset3D::kShadowCollision] =
      shadow_collision::Export(shadow_collision::Convert(std::move(sshadow.model)));
}
