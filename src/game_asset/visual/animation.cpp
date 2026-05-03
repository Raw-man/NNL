#include "NNL/game_asset/visual/animation.hpp"

#include "NNL/utility/data.hpp"
#include "NNL/utility/math.hpp"
#include "NNL/utility/string.hpp"
#include "simple_asset/sanimation.tpp"

namespace nnl {
namespace animation {

SAnimation Convert(const Animation& animation) {
  SAnimation sanimation;

  NNL_EXPECTS_DBG(animation.duration >= 1);

  const u16 duration = animation.duration;

  const u16 last_valid_time = (u16)(duration - 1);

  sanimation.duration = duration;

  for (const BoneChannel& bone_animation : animation.animation_channels) {
    SBoneChannel sbone_animation;

    std::vector<SKeyFrame<glm::vec3>>& sscale_keys = sbone_animation.scale_keys;
    std::vector<SKeyFrame<glm::quat>>& srotation_keys = sbone_animation.rotation_keys;
    std::vector<SKeyFrame<glm::vec3>>& stranslation_keys = sbone_animation.translation_keys;

    sscale_keys.reserve(duration);
    srotation_keys.reserve(duration);
    stranslation_keys.reserve(duration);

    const std::vector<KeyFrame>& scale_keys = bone_animation.scale_keys;
    const std::vector<KeyFrame>& translation_keys = bone_animation.translation_keys;
    const std::vector<KeyFrame> rotation_keys = BakeChannel_<nnl::LerpShort_>(bone_animation.rotation_keys, duration);

    NNL_EXPECTS_DBG(!scale_keys.empty());
    NNL_EXPECTS_DBG(!rotation_keys.empty());
    NNL_EXPECTS_DBG(!translation_keys.empty());

    if (auto& k0 = scale_keys.front(); k0.time != 0) {
      sscale_keys.push_back({0, k0.value});
    }

    for (const auto& key : scale_keys) {
      if (key.time >= duration) break;

      auto& skey = sscale_keys.emplace_back();

      skey.time = key.time;

      skey.value = key.value;
    }

    if (auto& kl = sscale_keys.back(); kl.time != last_valid_time) {
      sscale_keys.push_back({last_valid_time, kl.value});
    }

    glm::quat prev_q{1.0f, 0.0f, 0.0f, 0.0f};

    for (const auto& key : rotation_keys) {
      if (key.time >= duration) break;

      auto& skey = srotation_keys.emplace_back();

      skey.time = key.time;

      skey.value = utl::math::EulerToQuatCompat(key.value, prev_q);

      prev_q = skey.value;
    }

    if (auto& k0 = translation_keys.front(); k0.time != 0) {
      stranslation_keys.push_back({0, k0.value});
    }

    for (const auto& key : translation_keys) {
      if (key.time >= duration) break;

      auto& skey = stranslation_keys.emplace_back();

      skey.time = key.time;

      skey.value = key.value;
    }

    if (auto& kl = stranslation_keys.back(); kl.time != last_valid_time) {
      stranslation_keys.push_back({last_valid_time, kl.value});
    }

    sanimation.bone_channels.push_back(std::move(sbone_animation));
  }

  return sanimation;
}

std::vector<SAnimation> Convert(const AnimationContainer& animations) {
  std::vector<SAnimation> sanimations;

  for (std::size_t i = 0; i < animations.animations.size(); i++) {
    auto& sanimation = sanimations.emplace_back(Convert(animations.animations.at(i)));

    sanimation.name = utl::string::PrependZero(i);
  }

  return sanimations;
}

Animation Convert(SAnimation&& sanimation, const ConvertParam& anim_param) {
  NNL_EXPECTS(sanimation.duration >= 1);

  Animation animation;

  animation.duration = sanimation.duration;

  for (auto& sbone_animation : sanimation.bone_channels) {
    std::vector<SKeyFrame<glm::vec3>>& sscale_keys = sbone_animation.scale_keys;
    std::vector<SKeyFrame<glm::quat>>& srotation_keys = sbone_animation.rotation_keys;
    std::vector<SKeyFrame<glm::vec3>>& stranslation_keys = sbone_animation.translation_keys;

    NNL_EXPECTS(!sbone_animation.scale_keys.empty());
    NNL_EXPECTS(!sbone_animation.rotation_keys.empty());
    NNL_EXPECTS(!sbone_animation.translation_keys.empty());

    // Ensures safe Quaternion to Euler conversion.

    srotation_keys = BakeChannel_<nnl::Slerp_>(srotation_keys, sanimation.duration);

    BoneChannel bone_animation;
    std::vector<KeyFrame>& scale_keys = bone_animation.scale_keys;
    std::vector<KeyFrame>& rotation_keys = bone_animation.rotation_keys;
    std::vector<KeyFrame>& translation_keys = bone_animation.translation_keys;

    scale_keys.reserve(sscale_keys.size());
    rotation_keys.reserve(srotation_keys.size());
    translation_keys.reserve(stranslation_keys.size());

    for (auto& skey : sscale_keys) {
      KeyFrame key;

      key.time = skey.time;

      key.value = skey.value;

      bone_animation.scale_keys.push_back(key);
    }

    glm::vec3 prev_rot{0.0f};
    for (auto& skey : srotation_keys) {
      KeyFrame key;

      key.time = skey.time;

      key.value = utl::math::QuatToEulerCompat(skey.value, prev_rot);
      prev_rot = key.value;

      bone_animation.rotation_keys.push_back(key);
    }

    for (auto& skey : stranslation_keys) {
      KeyFrame key;

      key.time = skey.time;

      key.value = skey.value;

      bone_animation.translation_keys.push_back(key);
    }

    if (anim_param.unbake) {
      scale_keys = UnbakeChannel_<nnl::Lerp_>(bone_animation.scale_keys);
      rotation_keys = UnbakeChannel_<nnl::LerpShort_>(bone_animation.rotation_keys, 0.05f);
      translation_keys = UnbakeChannel_<nnl::Lerp_>(bone_animation.translation_keys);
    }

    animation.animation_channels.push_back(std::move(bone_animation));
  }

  sanimation = {};

  return animation;
}

AnimationContainer Convert_(std::vector<SAnimation>&& sanimations, const std::vector<ConvertParam>& anim_params,
                            bool move_with_root) {
  AnimationContainer animation_archive;

  animation_archive.move_with_root = move_with_root;

  for (std::size_t i = 0; i < sanimations.size(); i++) {
    animation_archive.animations.push_back(Convert(std::move(sanimations.at(i)), anim_params.at(i)));
  }

  sanimations.clear();

  return animation_archive;
}

AnimationContainer Convert(std::vector<SAnimation>&& sanimations, const ConvertParam& anim_params,
                           bool move_with_root) {
  std::vector<ConvertParam> params(sanimations.size(), anim_params);
  return Convert_(std::move(sanimations), params, move_with_root);
}

AnimationContainer Convert(std::vector<SAnimation>&& sanimations, const std::vector<ConvertParam>& anim_params,
                           bool move_with_root) {
  NNL_EXPECTS(sanimations.size() == anim_params.size());
  return Convert_(std::move(sanimations), anim_params, move_with_root);
}

bool IsOfType_(Reader& f) {
  using namespace raw;

  f.Seek(0);

  const std::size_t data_size = f.Len();
  if (data_size < sizeof(RHeader)) return false;

  auto rheader = f.ReadLE<RHeader>();

  if (rheader.magic_bytes != kMagicBytes) return false;

  if (rheader.offset_duration_table < sizeof(RHeader) ||
      data_size < rheader.offset_duration_table + sizeof(u16) * rheader.num_animations)
    return false;

  if (rheader.offset_keyframe_table < sizeof(RHeader) || data_size < rheader.offset_keyframe_table) return false;

  if (rheader.offset_rotation_table < sizeof(RHeader) || data_size < rheader.offset_rotation_table) return false;

  if (rheader.offset_scale_traslation_table < sizeof(RHeader) || data_size < rheader.offset_scale_traslation_table)
    return false;

  if (data_size < sizeof(RHeader) + rheader.num_animations * rheader.num_bones_per_animation * sizeof(RBoneChannel))
    return false;

  if (rheader.num_animations != 0 && rheader.num_bones_per_animation == 0) return false;

  if (rheader.move_with_root > 1) {
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

namespace raw {

AnimationContainer Convert(const RAnimationContainer& ranimation_container) {
  using namespace raw;

  AnimationContainer animation_archive;

  animation_archive.move_with_root = ranimation_container.header.move_with_root;

  for (std::size_t i = 0; i < ranimation_container.header.num_animations; i++) {
    Animation animation;

    animation.duration = ranimation_container.duration_table.at(i);

    for (std::size_t j = 0; j < ranimation_container.header.num_bones_per_animation; j++) {
      auto& rbone_animation =
          ranimation_container.bone_animations.at(i * ranimation_container.header.num_bones_per_animation + j);
      BoneChannel bone_animation;

      bone_animation.scale_keys.reserve(rbone_animation.num_scale_transforms);
      bone_animation.rotation_keys.reserve(rbone_animation.num_rotation_transforms);

      bone_animation.translation_keys.reserve(rbone_animation.num_translation_transforms);

      for (std::size_t k = 0; k < std::max<u16>(rbone_animation.num_scale_transforms, 1); k++) {
        KeyFrame scale;

        if (ranimation_container.keyframe_table.size() > 0)
          scale.time = ranimation_container.keyframe_table.at(rbone_animation.index_keyframe_scale + k);

        auto value = ranimation_container.scale_translation_table.at(rbone_animation.index_scale_table + k);
        scale.value = glm::vec3(value.x, value.y, value.z);

        bone_animation.scale_keys.push_back(scale);
      }

      for (std::size_t k = 0; k < std::max<u16>(rbone_animation.num_translation_transforms, 1); k++) {
        KeyFrame translation;

        if (ranimation_container.keyframe_table.size() > 0)
          translation.time = ranimation_container.keyframe_table.at(rbone_animation.index_keyframe_translation + k);

        auto value = ranimation_container.scale_translation_table.at(rbone_animation.index_translation_table + k);

        translation.value = glm::vec3(value.x, value.y, value.z);

        bone_animation.translation_keys.push_back(translation);
      }

      for (std::size_t k = 0; k < std::max<u16>(rbone_animation.num_rotation_transforms, 1); k++) {
        KeyFrame rotation;

        if (ranimation_container.keyframe_table.size() > 0)
          rotation.time = ranimation_container.keyframe_table.at(rbone_animation.index_keyframe_rotation + k);

        auto rotationS16 = ranimation_container.rotation_table.at(rbone_animation.index_rotation_table + k);

        rotation.value =
            glm::vec3(utl::math::FixedToFloat<i16, 1>(rotationS16.x), utl::math::FixedToFloat<i16, 1>(rotationS16.y),
                      utl::math::FixedToFloat<i16, 1>(rotationS16.z));
        rotation.value *= 90.0f;

        bone_animation.rotation_keys.push_back(rotation);
      }

      animation.animation_channels.push_back(bone_animation);
    }

    animation_archive.animations.push_back(animation);
  }

  return animation_archive;
}

RAnimationContainer Parse(Reader& f) {
  f.Seek(0);

  RAnimationContainer animation_container;
  NNL_TRY {
    animation_container.header = f.ReadLE<RHeader>();

    if (animation_container.header.magic_bytes != kMagicBytes) NNL_THROW(ParseError(NNL_SRCTAG("invalid magic bytes")));

    animation_container.bone_animations = f.ReadArrayLE<RBoneChannel>(
        animation_container.header.num_bones_per_animation * animation_container.header.num_animations);

    f.Seek(animation_container.header.offset_scale_traslation_table);

    {
      std::size_t num_entries = (animation_container.header.offset_rotation_table -
                                 animation_container.header.offset_scale_traslation_table) /
                                sizeof(Vec3<float>);

      animation_container.scale_translation_table = f.ReadArrayLE<Vec3<float>>(num_entries);
    }

    // Read rotations table
    f.Seek(animation_container.header.offset_rotation_table);
    {
      std::size_t num_entries =
          (animation_container.header.offset_keyframe_table - animation_container.header.offset_rotation_table) /
          sizeof(Vec3<i16>);

      animation_container.rotation_table = f.ReadArrayLE<Vec3<i16>>(num_entries);
    }

    // Read keyframe table
    f.Seek(animation_container.header.offset_keyframe_table);
    {
      std::size_t num_entries =
          (animation_container.header.offset_duration_table - animation_container.header.offset_keyframe_table) /
          sizeof(u16);

      animation_container.keyframe_table = f.ReadArrayLE<u16>(num_entries);
    }

    f.Seek(animation_container.header.offset_duration_table);

    animation_container.duration_table = f.ReadArrayLE<u16>(animation_container.header.num_animations);
  }
  NNL_CATCH(std::exception) { NNL_THROW(ParseError{NNL_SRCTAG(e.what())}); }

  return animation_container;
}

}  // namespace raw

AnimationContainer Import_(Reader& f) { return Convert(raw::Parse(f)); }

AnimationContainer Import(Reader& f) { return Import_(f); }

AnimationContainer Import(BufferView buffer) { return Import_(buffer); }

AnimationContainer Import(const std::filesystem::path& path) {
  FileReader f{path};

  return Import_(f);
}

void Export_(const AnimationContainer& animation_archive, Writer& f) {
  using namespace raw;

  f.Seek(0);

  RHeader header;

  header.num_animations = utl::data::narrow<u16>(animation_archive.animations.size());

  header.num_bones_per_animation =
      !animation_archive.animations.empty()
          ? utl::data::narrow<u16>(animation_archive.animations[0].animation_channels.size())
          : 0;

  header.move_with_root = animation_archive.move_with_root;

  auto header_off = f.WriteLE(header);
  std::vector<glm::vec3> scale_translation_table;
  std::vector<glm::vec3> rotation_table;
  std::vector<u16> keyframe_table;
  std::vector<u16> duration_table;

  for (std::size_t i = 0; i < animation_archive.animations.size(); i++) {
    auto& animation = animation_archive.animations.at(i);

    NNL_EXPECTS(animation.duration >= 1);

    duration_table.push_back(animation.duration);

    NNL_EXPECTS(animation.animation_channels.size() == header.num_bones_per_animation);

    for (std::size_t j = 0; j < animation.animation_channels.size(); j++) {
      auto& bone_animation = animation.animation_channels.at(j);

      RBoneChannel rbone_animation;

      // Write scale
      rbone_animation.num_scale_transforms =
          bone_animation.scale_keys.size() > 1 ? utl::data::narrow<u16>(bone_animation.scale_keys.size()) : 0;

      {
        std::vector<u16> key_sequence;
        std::vector<glm::vec3> value_sequence;

        key_sequence.reserve(bone_animation.scale_keys.size());
        value_sequence.reserve(bone_animation.scale_keys.size());

        NNL_EXPECTS(!bone_animation.scale_keys.empty());

        for (std::size_t k = 0; k < bone_animation.scale_keys.size(); k++) {
          auto& scale_key = bone_animation.scale_keys.at(k);
          NNL_EXPECTS(key_sequence.empty() || key_sequence.back() < scale_key.time);
          value_sequence.push_back(scale_key.value);
          key_sequence.push_back(scale_key.time);
        }

        auto itr = std::search(scale_translation_table.begin(), scale_translation_table.end(), value_sequence.begin(),
                               value_sequence.end());
        if (itr != scale_translation_table.end()) {
          auto index = itr - scale_translation_table.begin();
          rbone_animation.index_scale_table = index;
        } else {
          rbone_animation.index_scale_table = scale_translation_table.size();
          scale_translation_table.insert(scale_translation_table.end(), value_sequence.begin(), value_sequence.end());
        }

        if (key_sequence.size() > 1) {
          auto key_itr =
              std::search(keyframe_table.begin(), keyframe_table.end(), key_sequence.begin(), key_sequence.end());
          if (key_itr != keyframe_table.end()) {
            auto index = key_itr - keyframe_table.begin();
            rbone_animation.index_keyframe_scale = index;
          } else {
            rbone_animation.index_keyframe_scale = keyframe_table.size();
            keyframe_table.insert(keyframe_table.end(), key_sequence.begin(), key_sequence.end());
          }
        }
      }

      // Write rotation

      rbone_animation.num_rotation_transforms =
          bone_animation.rotation_keys.size() > 1 ? utl::data::narrow<u16>(bone_animation.rotation_keys.size()) : 0;

      {
        std::vector<u16> key_sequence;
        std::vector<glm::vec3> value_sequence;

        key_sequence.reserve(bone_animation.rotation_keys.size());
        value_sequence.reserve(bone_animation.rotation_keys.size());

        NNL_EXPECTS(!bone_animation.rotation_keys.empty());

        for (std::size_t k = 0; k < bone_animation.rotation_keys.size(); k++) {
          auto& rotation_key = bone_animation.rotation_keys.at(k);
          NNL_EXPECTS(key_sequence.empty() || key_sequence.back() < rotation_key.time);
          value_sequence.push_back(rotation_key.value);
          key_sequence.push_back(rotation_key.time);
        }

        auto itr =
            std::search(rotation_table.begin(), rotation_table.end(), value_sequence.begin(), value_sequence.end());
        if (itr != rotation_table.end()) {
          auto index = itr - rotation_table.begin();
          rbone_animation.index_rotation_table = index;
        } else {
          rbone_animation.index_rotation_table = rotation_table.size();
          rotation_table.insert(rotation_table.end(), value_sequence.begin(), value_sequence.end());
        }

        if (key_sequence.size() > 1) {
          auto key_itr =
              std::search(keyframe_table.begin(), keyframe_table.end(), key_sequence.begin(), key_sequence.end());
          if (key_itr != keyframe_table.end()) {
            auto index = key_itr - keyframe_table.begin();
            rbone_animation.index_keyframe_rotation = index;
          } else {
            rbone_animation.index_keyframe_rotation = keyframe_table.size();
            keyframe_table.insert(keyframe_table.end(), key_sequence.begin(), key_sequence.end());
          }
        }
      }

      // Write translations

      rbone_animation.num_translation_transforms = bone_animation.translation_keys.size() > 1
                                                       ? utl::data::narrow<u16>(bone_animation.translation_keys.size())
                                                       : 0;

      {
        std::vector<u16> key_sequence;
        std::vector<glm::vec3> value_sequence;

        key_sequence.reserve(bone_animation.translation_keys.size());
        value_sequence.reserve(bone_animation.translation_keys.size());

        NNL_EXPECTS(!bone_animation.translation_keys.empty());

        for (std::size_t k = 0; k < bone_animation.translation_keys.size(); k++) {
          auto& translation_key = bone_animation.translation_keys.at(k);
          NNL_EXPECTS(key_sequence.empty() || key_sequence.back() < translation_key.time);
          value_sequence.push_back(translation_key.value);
          key_sequence.push_back(translation_key.time);
        }

        auto itr = std::search(scale_translation_table.begin(), scale_translation_table.end(), value_sequence.begin(),
                               value_sequence.end());
        if (itr != scale_translation_table.end()) {
          auto index = itr - scale_translation_table.begin();
          rbone_animation.index_translation_table = index;
        } else {
          rbone_animation.index_translation_table = scale_translation_table.size();
          scale_translation_table.insert(scale_translation_table.end(), value_sequence.begin(), value_sequence.end());
        }

        if (key_sequence.size() > 1) {
          auto key_itr =
              std::search(keyframe_table.begin(), keyframe_table.end(), key_sequence.begin(), key_sequence.end());
          if (key_itr != keyframe_table.end()) {
            auto index = key_itr - keyframe_table.begin();
            rbone_animation.index_keyframe_translation = index;
          } else {
            rbone_animation.index_keyframe_translation = keyframe_table.size();
            keyframe_table.insert(keyframe_table.end(), key_sequence.begin(), key_sequence.end());
          }
        }
      }

      f.WriteLE(rbone_animation);
    }
  }

  header_off->*& RHeader::offset_scale_traslation_table = utl::data::narrow<u32>(f.Tell());

  f.WriteArrayLE(scale_translation_table);

  {
    std::vector<Vec3<i16>> rotation_table_s;
    rotation_table_s.reserve(rotation_table.size());
    for (auto& rotation : rotation_table) {
      NNL_EXPECTS(std::abs(rotation.x) <= 180.0f);
      NNL_EXPECTS(std::abs(rotation.y) <= 180.0f);
      NNL_EXPECTS(std::abs(rotation.z) <= 180.0f);

      rotation_table_s.push_back(Vec3<i16>{utl::math::FloatToFixed<i16, 1>(rotation.x / 90.0f),
                                           utl::math::FloatToFixed<i16, 1>(rotation.y / 90.0f),
                                           utl::math::FloatToFixed<i16, 1>(rotation.z / 90.0f)});
    }

    header_off->*& RHeader::offset_rotation_table = utl::data::narrow<u32>(f.Tell());

    f.WriteArrayLE(rotation_table_s);
  }

  header_off->*& RHeader::offset_keyframe_table = utl::data::narrow<u32>(f.Tell());

  f.WriteArrayLE(keyframe_table);

  header_off->*& RHeader::offset_duration_table = utl::data::narrow<u32>(f.Tell());

  f.WriteArrayLE(duration_table);
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

}  // namespace animation
}  // namespace nnl
