#include "NNL/game_asset/visual/animation.hpp"

#include <gtest/gtest.h>

#include "NNL/nnl.hpp"
#include "test_common.hpp"
using namespace nnl;

TEST(Animation, IsOfType) {
  FileReader bin_anim(GetPath("naruto_animation"));

  ASSERT_TRUE(animation::IsOfType(bin_anim));
}

TEST(Animation, ImportExport) {
  {
    nnl::FileReader f{GetPath("naruto_animation")};

    auto bin_anim = f.ReadArrayLE<u8>(f.Len());

    auto anim_container = animation::Import(bin_anim);
    auto bin_anim_re = animation::Export(anim_container);

    ASSERT_TRUE(bin_anim == bin_anim_re);
  }
}

TEST(Animation, ConvertAnimationToSAnimation) {
  nnl::FileReader f{GetPath("naruto_animation")};

  auto bin_anim = f.ReadArrayLE<u8>(f.Len());

  auto anim_container = animation::Import(bin_anim);

  auto sanimations = animation::Convert(anim_container);

  ASSERT_TRUE(anim_container.animations.size() == sanimations.size());
}

TEST(Animation, ConvertSAnimationToAnimation) {
  nnl::FileReader f{GetPath("naruto_animation")};

  auto bin_anim = f.ReadArrayLE<u8>(f.Len());

  auto anim_container = animation::Import(bin_anim);

  auto anim_container_re = animation::Convert(animation::Convert(anim_container));

  ASSERT_TRUE(anim_container.animations.size() == anim_container_re.animations.size());
}
