
#include "NNL/game_asset/visual/visanimation.hpp"

#include "NNL/utility/data.hpp"
namespace nnl {

namespace visanimation {

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  const std::size_t data_size = f.Len();

  if (data_size < sizeof(RHeader)) return false;

  auto rheader = f.ReadLE<RHeader>();

  if (rheader.magic_bytes != kMagicBytes) return false;

  if (rheader.num_animations == 0) return false;

  std::size_t full_header_size = sizeof(RHeader) + rheader.num_animations * sizeof(RAnimation);

  if (data_size < full_header_size) return false;

  auto animations = f.ReadArrayLE<RAnimation>(rheader.num_animations);

  for (auto& anim : animations) {
    if (anim.offset_keyframes % sizeof(u16) != 0 || anim.offset_keyframes < full_header_size ||
        data_size < anim.offset_keyframes +
                        (anim.num_left_channel_keyframes + anim.num_right_channel_keyframes) * sizeof(RKeyFrame))
      return false;
  }

  return true;
}

bool IsOfType(Reader& f) { return IsOfType_(f); }

bool IsOfType(BufferView buffer) { return IsOfType_(buffer); }

bool IsOfType(const std::filesystem::path& path) {
  FileReader f{path};
  return IsOfType_(f);
}

Animation Convert(SVisibilityAnimation&& sanimation) {
  Animation subanim;

  u16 duration = 0;

  for (std::size_t i = 0; i < sanimation.visibility_channels.size(); i++) {
    auto& schannel = sanimation.visibility_channels.at(i);

    std::vector<SKeyFrame<bool>> new_channel;

    // Bake the animation
    for (std::size_t i = 0; i < schannel.size(); i++) {
      auto& skey = schannel.at(i);

      i16 prev_time = i > 0 ? new_channel.back().time : 0;

      if (skey.time > duration) duration = skey.time;

      if (i == 0 && skey.time != 0) {
        for (i16 n = 0; n < skey.time - 1; n++) {
          new_channel.push_back({(u16)skey.time, skey.value});
        }
      } else {
        for (i16 n = 1; n < (skey.time - prev_time); n++) {
          new_channel.push_back({(u16)skey.time, new_channel.back().value});
        }
      }

      new_channel.push_back({(u16)skey.time, skey.value});
    }

    schannel = std::move(new_channel);
  }

  duration++;

  for (std::size_t t = 0; t < duration; t++) {
    KeyFrame key;

    key.time_tick = t;
    key.flags_enable_groups = 1;

    for (std::size_t i = 0; i < sanimation.visibility_channels.size(); i++) {
      auto& schannel = sanimation.visibility_channels.at(i);

      if (!schannel.empty()) {
        bool visible = t < schannel.size() ? schannel.at(t).value : schannel.back().value;

        key.flags_enable_groups |= (u16)(visible << i);
      }
    }

    if (subanim.left_channel.empty() || key.flags_enable_groups != subanim.left_channel.back().flags_enable_groups)
      subanim.left_channel.push_back(key);
  }

  sanimation = {};

  return subanim;
}

AnimationContainer Convert(std::vector<SVisibilityAnimation>&& sanimations) {
  AnimationContainer anim_container;

  anim_container.flags_disable_left_channel = 0;  // Disable animated groups
  anim_container.flags_disable_right_channel = 0;

  for (auto& sanimation : sanimations) {
    for (std::size_t i = 0; i < sanimation.visibility_channels.size(); i++) {
      if (!sanimation.visibility_channels.at(i).empty()) anim_container.flags_disable_left_channel |= (u16)(1 << i);
    }

    anim_container.animations.push_back(Convert(std::move(sanimation)));
  }

  sanimations.clear();

  return anim_container;
}

SVisibilityAnimation Convert(const Animation& animation, u16 flags_disable_left, u16 flags_disable_right) {
  SVisibilityAnimation sanimation;

  u16 enabled = 0xFF'FF ^ (flags_disable_left | flags_disable_right);

  u16 default_enable_l = 0;

  u16 default_enable_r = 0;

  for (std::size_t i = 0; i < 16; i++) {
    if ((flags_disable_left & (1 << i))) {
      default_enable_l = (u16)(1 << i);
      break;
    }
  }

  for (std::size_t i = 0; i < 16; i++) {
    if ((flags_disable_right & (1 << i))) {
      default_enable_r = (u16)(1 << i);
      break;
    }
  }

  std::vector<KeyFrame> nl_channel;

  std::vector<KeyFrame> nr_channel;

  u16 duration = 0;

  for (std::size_t i = 0; i < animation.left_channel.size(); i++) {
    const KeyFrame& l_key = animation.left_channel.at(i);

    i16 prev_time = i > 0 ? nl_channel.back().time_tick : 0;

    if (l_key.time_tick > duration) duration = l_key.time_tick;

    u16 enable = l_key.flags_enable_groups != 0 ? l_key.flags_enable_groups : default_enable_l;

    if (i == 0 && l_key.time_tick != 0) {
      for (i16 n = 0; n < l_key.time_tick - 1; n++) {
        nl_channel.push_back({l_key.time_tick, (u16)(enabled | enable)});
      }
    } else {
      for (i16 n = 1; n < ((i16)l_key.time_tick - prev_time); n++) {
        nl_channel.push_back({l_key.time_tick, nl_channel.back().flags_enable_groups});
      }
    }

    nl_channel.push_back({l_key.time_tick, (u16)(enabled | enable)});
  }

  for (std::size_t i = 0; i < animation.right_channel.size(); i++) {
    const KeyFrame& r_key = animation.right_channel.at(i);

    i16 prev_time = i > 0 ? nr_channel.back().time_tick : 0;

    if (r_key.time_tick > duration) duration = r_key.time_tick;

    u16 enable = r_key.flags_enable_groups != 0 ? r_key.flags_enable_groups : default_enable_r;

    if (i == 0 && r_key.time_tick != 0) {
      for (i16 n = 0; n < r_key.time_tick - 1; n++) {
        nr_channel.push_back({r_key.time_tick, (u16)(enabled | enable)});
      }
    } else {
      for (i16 n = 1; n < ((i16)r_key.time_tick - prev_time); n++) {
        nr_channel.push_back({r_key.time_tick, nr_channel.back().flags_enable_groups});
      }
    }

    nr_channel.push_back({r_key.time_tick, (u16)(enabled | enable)});
  }

  duration++;

  if (nr_channel.empty()) {
    nr_channel.resize(duration, {0, (u16)(enabled | default_enable_r)});
  }

  if (nl_channel.empty()) {
    nl_channel.resize(duration, {0, (u16)(enabled | default_enable_l)});
  }

  if (nr_channel.size() < duration) {
    nr_channel.resize(duration, nr_channel.back());
  }

  if (nl_channel.size() < duration) {
    nl_channel.resize(duration, nl_channel.back());
  }

  for (std::size_t t = 0; t < duration; t++) {
    u16 l_flags = t < nl_channel.size() ? nl_channel.at(t).flags_enable_groups : 0;

    u16 r_flags = t < nr_channel.size() ? nr_channel.at(t).flags_enable_groups : 0;

    u16 u_flags = l_flags | r_flags;

    for (std::size_t i = 0; i < sanimation.visibility_channels.size(); i++) {
      // If the group is always visible, do not animate it
      if ((enabled & (1 << i)) != 0) continue;

      auto& schannel = sanimation.visibility_channels.at(i);

      bool visible = (u_flags & (1 << i));

      if (schannel.empty() || schannel.back().value != visible) schannel.push_back({(u16)t, visible});
    }
  }

  return sanimation;
}

std::vector<SVisibilityAnimation> Convert(const AnimationContainer& animation_container) {
  std::vector<SVisibilityAnimation> sanimations;

  sanimations.reserve(animation_container.animations.size());

  for (std::size_t i = 0; i < animation_container.animations.size(); i++) {
    auto& animation = animation_container.animations.at(i);
    sanimations.push_back(Convert(animation, animation_container.flags_disable_left_channel,
                                  animation_container.flags_disable_right_channel));
  }

  return sanimations;
}

namespace raw {

AnimationContainer Convert(const RAnimationContainer& rsubanim) {
  AnimationContainer subanim;
  subanim.flags_disable_left_channel = rsubanim.header.flags_disable_left_channel;
  subanim.flags_disable_right_channel = rsubanim.header.flags_disable_right_channel;

  for (auto& ranim : rsubanim.animations) {
    Animation anim;

    for (std::size_t i = 0; i < ranim.num_left_channel_keyframes; i++) {
      auto& rkey = rsubanim.keyframes.at(ranim.offset_keyframes).at(i);
      anim.left_channel.push_back({rkey.time_tick, rkey.flags_enable_groups});
    }

    for (std::size_t i = ranim.num_left_channel_keyframes;
         i < ranim.num_left_channel_keyframes + ranim.num_right_channel_keyframes; i++) {
      auto& rkey = rsubanim.keyframes.at(ranim.offset_keyframes).at(i);
      anim.right_channel.push_back({rkey.time_tick, rkey.flags_enable_groups});
    }

    subanim.animations.push_back(std::move(anim));
  }

  return subanim;
}

RAnimationContainer Parse(Reader& f) {
  RAnimationContainer subanim_container;
  f.Seek(0);
  auto& header = subanim_container.header;

  NNL_TRY {
    header = f.ReadLE<RHeader>();

    if (header.magic_bytes != kMagicBytes) NNL_THROW(ParseError(NNL_SRCTAG("invalid magic bytes")));

    subanim_container.animations = f.ReadArrayLE<RAnimation>(header.num_animations);

    for (auto& ranim : subanim_container.animations) {
      f.Seek(ranim.offset_keyframes);
      subanim_container.keyframes[ranim.offset_keyframes] =
          f.ReadArrayLE<RKeyFrame>(ranim.num_left_channel_keyframes + ranim.num_right_channel_keyframes);
    }
  }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_SRCTAG(e.what())}); }
  return subanim_container;
}
}  // namespace raw

AnimationContainer Import_(Reader& f) { return raw::Convert(raw::Parse(f)); }

AnimationContainer Import(Reader& f) { return Import_(f); }

AnimationContainer Import(BufferView buffer) { return Import_(buffer); }

AnimationContainer Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

void Export_(const AnimationContainer& subanim, Writer& f) {
  using namespace raw;
  f.Seek(0);
  NNL_EXPECTS(!subanim.animations.empty());
  RHeader header;
  header.flags_disable_left_channel = subanim.flags_disable_left_channel;
  header.flags_disable_right_channel = subanim.flags_disable_right_channel;
  header.num_animations = utl::data::narrow<u16>(subanim.animations.size());

  f.WriteLE(header);

  auto animation_start = f.MakeOffsetLE<RAnimation>();

  for (auto& anim : subanim.animations) {
    RAnimation ranim;
    ranim.num_left_channel_keyframes = utl::data::narrow<u16>(anim.left_channel.size());
    ranim.num_right_channel_keyframes = utl::data::narrow<u16>(anim.right_channel.size());
    f.WriteLE(ranim);
  }

  for (auto& anim : subanim.animations) {
    animation_start->*& RAnimation::offset_keyframes = utl::data::narrow<u32>(f.Tell());

    for (std::size_t i = 0; i < anim.left_channel.size(); i++) {
      auto& key = anim.left_channel[i];
      NNL_EXPECTS(i == 0 || key.time_tick > anim.left_channel[i - 1].time_tick);
      f.WriteLE(RKeyFrame{key.time_tick, key.flags_enable_groups});
    }

    for (std::size_t i = 0; i < anim.right_channel.size(); i++) {
      auto& key = anim.right_channel[i];
      NNL_EXPECTS(i == 0 || key.time_tick > anim.right_channel[i - 1].time_tick);
      f.WriteLE(RKeyFrame{key.time_tick, key.flags_enable_groups});
    }
    ++animation_start;
  }
}

Buffer Export(const AnimationContainer& animation_container) {
  BufferRW f;
  Export_(animation_container, f);
  return f;
}

void Export(const AnimationContainer& animation_container, Writer& f) { Export_(animation_container, f); }

void Export(const AnimationContainer& animation_container, const std::filesystem::path& path) {
  FileRW f{path, true};
  Export_(animation_container, f);
}

}  // namespace visanimation
}  // namespace nnl
