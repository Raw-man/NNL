#include "NNL/game_asset/behavior/action.hpp"

#include <gtest/gtest.h>

#include "test_common.hpp"

using namespace nnl;

TEST(Action, IsOfType) {
  FileReader f{GetPath("naruto_action")};
  ASSERT_TRUE(action::IsOfType(f));
}

TEST(Action, ImportExport) {
  {
    nnl::FileReader f{GetPath("naruto_action")};

    auto bin_act = f.ReadArrayLE<u8>(f.Len());

    auto actions = action::Import(bin_act);
    auto bin_act_re = action::Export(actions);

    ASSERT_TRUE(bin_act == bin_act_re);
  }
}

TEST(Action, ReadWriteArgs) {
  std::vector<action::EffectNode> anim_nodes;

  std::size_t current_ind = anim_nodes.size();
  anim_nodes.push_back({{action::EffectFunction::kPlaySFX, 0, 0}});
  action::WriteArgs<u8, u8, float>(current_ind, anim_nodes, 0x10, 1, 1.0f);

  current_ind = anim_nodes.size();
  anim_nodes.push_back({{action::EffectFunction::kPlaySFX, 0, 0}});
  action::WriteArgs<u16, u16, float, float>(current_ind, anim_nodes, 0x10, 1, 1.0f, 2.0f);

  current_ind = anim_nodes.size();
  anim_nodes.push_back({{action::EffectFunction::kPlaySFX, 0, 0}});
  action::WriteArgs<u16, u16, float, float>(current_ind, anim_nodes, 0x10, 1, 3.0f, 4.25f);
  current_ind = 0;

  auto [a0, a1, a2] = action::ReadArgs<u8, u8, float>(current_ind, anim_nodes);

  current_ind = action::NextNodeInd(current_ind, anim_nodes);

  auto [a00, a01, a02, a03] = action::ReadArgs<u16, u16, float, float>(current_ind, anim_nodes);

  current_ind = action::NextNodeInd(current_ind, anim_nodes);

  auto [a000, a001, a002, a003] = action::ReadArgs<u16, u16, float, float>(current_ind, anim_nodes);

  ASSERT_EQ(a0, 0x10);
  ASSERT_EQ(a1, 1);
  ASSERT_EQ(a2, 1.0f);

  ASSERT_EQ(a00, 0x10);
  ASSERT_EQ(a01, 1);
  ASSERT_EQ(a02, 1.0f);
  ASSERT_EQ(a03, 2.0f);

  ASSERT_EQ(a000, 0x10);
  ASSERT_EQ(a001, 1);
  ASSERT_EQ(a002, 3.0f);
  ASSERT_EQ(a003, 4.25f);
}
