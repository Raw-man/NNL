#include "NNL/game_asset/behavior/action.hpp"

namespace nnl {
namespace action {

std::map<u16, std::set<std::string>> GetAnimationNames(const ActionConfig& action_config) {
  std::map<u16, std::set<std::string>> animation_names;

  for (const auto& [id, action] : action_config) {
    for (std::size_t j = 0; j < action.animation_nodes.size();) {
      auto& node = action.animation_nodes.at(j);

      if (node.main.function == AnimationFunction::kPlayAnimation ||
          node.main.function == AnimationFunction::kTransitionToAnimation5 ||
          node.main.function == AnimationFunction::kTransitionToAnimation6) {
        auto animation_id = std::get<0>(ReadArgs<u16, u16>(j, action.animation_nodes));
        animation_names[animation_id].insert(action.name);
      }
      if (node.main.next_main_node == 0) {
        break;
      }
      j += node.main.next_main_node;
    }
  }

  return animation_names;
}

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  const std::size_t data_size = f.Len();

  if (data_size < sizeof(RHeader)) return false;

  auto rheader = f.ReadLE<RHeader>();

  for (std::size_t i = 0; i < kNumActionCategories; i++) {
    auto& cat = rheader.action_categories[i];

    if (cat.offset < sizeof(RHeader) || cat.offset % sizeof(u16) != 0 ||
        data_size < cat.offset + sizeof(RAction) * cat.num_actions || cat.num_actions > (kMaxActionIndex + 1))
      return false;

    if (i == 0) continue;

    auto& prev_cat = rheader.action_categories[i - 1];
    // memory regions defined by offset and size should not intersect
    if (cat.offset < prev_cat.offset + sizeof(RAction) * prev_cat.num_actions) return false;
  }

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

namespace raw {

ActionConfig Convert(RActionConfig&& raction_config) {
  ActionConfig action_config;
  // Iterate through action name offsets and find their corresponding action.
  // This is done to match the original binary file when exporting.

  // A map of action name offsets to their actions

  std::unordered_map<u16, std::pair<action::Id, const RAction*>> ractions_map;

  for (std::size_t i = 0; i < kNumActionCategories; i++) {
    auto& raction_category = raction_config.header.action_categories[i];
    auto& ractions = raction_config.actions.at(raction_category.offset);

    for (std::size_t j = 0; j < ractions.size() && j <= kMaxActionIndex; j++) {
      const RAction& raction = ractions.at(j);
      action::Id id{static_cast<ActionCategory>(i), utl::data::narrow_cast<u16>(j)};
      ractions_map.insert({raction.offset_action_name, {id, &raction}});
    }
  }

  for (auto iter = raction_config.action_names.rbegin(); iter != raction_config.action_names.rend(); ++iter) {
    auto& offset_action_name = iter->first;

    auto [id, action_ptr] = ractions_map.at(offset_action_name);
    auto& raction = *action_ptr;
    Action action;

    if (raction.offset_action_name > 0) {
      action.name = std::move(raction_config.action_names.at(raction.offset_action_name));
    }

    if (raction.offset_animation_nodes > 0) {
      auto& rnodes = raction_config.animation_nodes.at(raction.offset_animation_nodes);
      action.animation_nodes.resize(rnodes.size());

      std::memcpy(action.animation_nodes.data(), rnodes.data(), rnodes.size() * sizeof(RAnimationNode));
    }

    if (raction.offset_effect_nodes > 0) {
      auto& rnodes = raction_config.effect_nodes.at(raction.offset_effect_nodes);
      action.effect_nodes.resize(rnodes.size());

      std::memcpy(action.effect_nodes.data(), rnodes.data(), rnodes.size() * sizeof(REffectNode));
    }
    // Actions are inserted in the order they were written to the file.

    action_config.insert({id, std::move(action)});
  }

  raction_config = {};

  return action_config;
}

RActionConfig Parse(Reader& f) {
  f.Seek(0);
  RActionConfig rconfig;
  NNL_TRY {
    rconfig.header = f.ReadLE<RHeader>();

    for (std::size_t i = 0; i < kNumActionCategories; i++) {
      f.Seek(rconfig.header.action_categories[i].offset);
      rconfig.actions[rconfig.header.action_categories[i].offset] =
          f.ReadArrayLE<RAction>(rconfig.header.action_categories[i].num_actions);
    }

    for (auto& [offset, actions] : rconfig.actions) {
      for (auto& action : actions) {
        // name
        if (action.offset_action_name != 0) {
          f.Seek(action.offset_action_name);
          std::string name;
          char c = '\0';
          while ((c = f.ReadLE<char>()) != '\0') {
            name += c;
          }
          rconfig.action_names[action.offset_action_name] = std::move(name);
        }

        if (action.offset_animation_nodes != 0) {
          f.Seek(action.offset_animation_nodes);
          RAnimationNode node;
          do {
            node = f.ReadLE<RAnimationNode>();

            if (node.index_function > 0x1FF || node.next_main_node > 0x7F) break;  // likely invalid data

            rconfig.animation_nodes[action.offset_animation_nodes].push_back(node);

            if (node.next_main_node > 1) {
              auto& anim_nodes = rconfig.animation_nodes[action.offset_animation_nodes];
              auto arg_nodes = f.ReadArrayLE<RAnimationNode>(node.next_main_node - 1);
              anim_nodes.insert(anim_nodes.end(), arg_nodes.begin(), arg_nodes.end());
            }

          } while (node.next_main_node != 0 && node.index_function != 0);
        }

        if (action.offset_effect_nodes != 0) {
          f.Seek(action.offset_effect_nodes);
          REffectNode node;
          do {
            node = f.ReadLE<REffectNode>();

            if (node.next_main_node > 0x7F) break;  // likely invalid data

            rconfig.effect_nodes[action.offset_effect_nodes].push_back(node);

            if (node.next_main_node > 1) {
              auto& eff_nodes = rconfig.effect_nodes[action.offset_effect_nodes];
              auto arg_nodes = f.ReadArrayLE<REffectNode>(node.next_main_node - 1);
              eff_nodes.insert(eff_nodes.end(), arg_nodes.begin(), arg_nodes.end());
            }

          } while (node.next_main_node != 0 && node.index_function != 0);
        }
      }
    }
  }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_SRCTAG(e.what())}); }
  return rconfig;
}

}  // namespace raw

ActionConfig Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

ActionConfig Import(Reader& f) { return Import_(f); }

ActionConfig Import(BufferView buffer) { return Import_(buffer); }

ActionConfig Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

void Export_(const ActionConfig& action_config, Writer& f) {
  using namespace raw;

  f.Seek(0);

  RHeader header;

  for (auto& [id, action] : action_config) {
    NNL_EXPECTS(id.action_index <= kMaxActionIndex);
    auto& action_category = header.action_categories[utl::data::as_int(id.action_category)];
    if ((u32)id.action_index + 1 > action_category.num_actions) action_category.num_actions = id.action_index + 1;
  }

  auto header_off = f.WriteLE(header);

  auto action_start = f.MakeOffsetLE<RAction>();

  for (std::size_t i = 0; i < kNumActionCategories; i++) {
    auto action_cat_off = (header_off->*&RHeader::action_categories)[i];
    action_cat_off->*& RActionCategory::offset = utl::data::narrow<u32>(f.Tell());
    auto& action_category = header.action_categories[i];
    for (std::size_t j = 0; j < action_category.num_actions; j++) {
      f.WriteLE(RAction());
    }
  }

  auto calc_index = [&header](const action::Id& id) -> int {
    int action_index = 0;
    for (std::size_t i = 0; i < (std::size_t)id.action_category; i++) {
      action_index += header.action_categories[i].num_actions;
    }
    action_index += id.action_index;
    return action_index;
  };

  for (auto& [id, action] : action_config) {
    auto action_off = action_start + calc_index(id);
    action_off->*& RAction::offset_animation_nodes =
        !action.animation_nodes.empty() ? utl::data::narrow<u16>(f.Tell()) : 0;
    f.WriteArrayLE(action.animation_nodes);

    if (!action.animation_nodes.empty() && action.animation_nodes.back().main.function != AnimationFunction::kEnd) {
      RAnimationNode term;
      f.WriteLE(term);
    }
  }

  for (auto iter = action_config.rbegin(); iter != action_config.rend(); ++iter) {
    auto& action_id = iter->first;
    auto& action = iter->second;
    auto action_off = action_start + calc_index(action_id);

    NNL_EXPECTS(action.effect_nodes.empty() || !action.animation_nodes.empty());

    action_off->*& RAction::offset_effect_nodes = !action.effect_nodes.empty() ? utl::data::narrow<u16>(f.Tell()) : 0;
    f.WriteArrayLE(action.effect_nodes);

    if (!action.effect_nodes.empty() && action.effect_nodes.back().main.function != EffectFunction::kEnd) {
      REffectNode term;
      f.WriteLE(term);
    }
  }

  for (auto iter = action_config.rbegin(); iter != action_config.rend(); ++iter) {
    auto& action_id = iter->first;
    auto& action = iter->second;
    auto action_off = action_start + calc_index(action_id);
    if (!action.name.empty()) {
      action_off->*& RAction::offset_action_name = utl::data::narrow<u16>(f.Tell());
      f.WriteBuf(action.name.c_str(), action.name.size() + 1);
    }
  }
}

Buffer Export(const ActionConfig& action_config) {
  BufferRW f;
  Export_(action_config, f);
  return f;
}

void Export(const ActionConfig& action_config, Writer& f) { Export_(action_config, f); }

void Export(const ActionConfig& action_config, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(action_config, f);
}

}  // namespace action
}  // namespace nnl
