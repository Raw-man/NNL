
#include "NNL/simple_asset/sanimation.hpp"

#include "NNL/utility/math.hpp"

namespace nnl {

template <typename T>
static bool UnbakeChannel(std::vector<SKeyFrame<T>>& keys, const float eq_range = 1.0e-4f) {
  bool simplified = false;

  std::vector<SKeyFrame<T>> new_keys;
  new_keys.reserve(keys.size());

  while (keys.size() >= 3) {
    auto key1 = keys[0];
    auto key2 = keys[1];
    auto key3 = keys[2];

    auto v1(key1.value);
    auto v2(key2.value);
    auto v3(key3.value);

    NNL_EXPECTS_DBG(key3.time > key2.time && key2.time > key1.time);

    u16 num_mid_frames = (key3.time - key1.time);

    float progression = (float)(key2.time - key1.time) / (float)num_mid_frames;

    bool are_equal = false;

    if constexpr (std::is_same_v<T, glm::vec3>) {
      glm::vec3 v2_interp = glm::mix(v1, v3, progression);
      are_equal = utl::math::IsApproxEqual(v2_interp, v2, eq_range);
    } else {
      glm::vec3 v2_interp_eul = glm::mix(utl::math::QuatToEuler(v1), utl::math::QuatToEuler(v3), progression);

      glm::quat v2_interp = glm::slerp(v1, v3, progression);

      are_equal = utl::math::IsApproxEqual(v2_interp, v2, eq_range) &&
                  utl::math::IsApproxEqual(v2_interp_eul, utl::math::QuatToEuler(v2), 10.0f);
    }

    if (are_equal) {
      keys.erase(keys.begin() + 1);
      simplified = true;

    } else {
      new_keys.push_back(keys.at(0));
      keys.erase(keys.begin() + 0);
    }
  }

  for (auto& key : keys) {
    new_keys.push_back(key);
  }

  keys = std::move(new_keys);

  if (keys.size() == 2) {
    auto key1 = keys[0];
    auto key2 = keys[1];

    NNL_EXPECTS_DBG(key2.time > key1.time);

    auto v1(key1.value);
    auto v2(key2.value);

    if (utl::math::IsApproxEqual(v1, v2, eq_range)) {
      keys.erase(keys.begin() + 1);
    }
  }

  return simplified;
}

void SAnimation::Unbake(float tolerance) {
  bool simplified_last_pass = false;

  do {
    simplified_last_pass = false;

    for (auto& channel : bone_channels) {
      simplified_last_pass |= UnbakeChannel(channel.scale_keys, tolerance);
      simplified_last_pass |= UnbakeChannel(channel.rotation_keys, tolerance);
      simplified_last_pass |= UnbakeChannel(channel.translation_keys, tolerance);
    }

  } while (simplified_last_pass);
}

template <typename T>
static void BakeChannel(std::vector<SKeyFrame<T>>& keys, const std::size_t duration) {
  if (keys.size() == duration) return;

  NNL_EXPECTS(!keys.empty());

  std::vector<SKeyFrame<T>> new_keys;
  new_keys.reserve(duration);

  for (std::size_t m = 0; m < keys.size(); m++) {
    const auto& key = keys[m];

    if (m == 0 && key.time != 0)  // Insert missing keyframes
    {
      for (std::size_t t = 0; t < key.time; t++) {
        SKeyFrame<T> new_key;
        new_key.time = t;
        new_key.value = key.value;

        if (new_key.time < duration)
          new_keys.push_back(new_key);
        else
          break;
      }
    }

    if (key.time < duration)
      new_keys.push_back(key);
    else
      break;

    assert(keys.size() >= 1);
    // If not the last keyframe, interpolate in betweens
    if (m < keys.size() - 1) {
      const auto& next_key = keys.at(m + 1);

      NNL_EXPECTS_DBG(key.time < next_key.time);

      u16 num_extra_keys = next_key.time - key.time;

      for (std::size_t t = 1; t < num_extra_keys; t++) {
        float progression = (float)t / (float)num_extra_keys;

        SKeyFrame<T> new_key;
        new_key.time = key.time + t;
        if constexpr (std::is_same_v<T, glm::vec3>) {
          T val_interp = glm::mix(key.value, next_key.value, progression);

          new_key.value = val_interp;
        } else {
          T val_interp = glm::slerp(key.value, next_key.value, progression);

          new_key.value = val_interp;
        }

        if (new_key.time < duration)
          new_keys.push_back(new_key);
        else
          break;
      }
    }
  }

  if (!new_keys.empty()) {
    auto last_key = new_keys.back();

    assert(duration > last_key.time);

    std::size_t num_end_keys = duration - last_key.time;

    for (std::size_t k = 1; k < num_end_keys; k++) {
      SKeyFrame<T> new_key = last_key;
      new_key.time += k;

      if (new_key.time < duration)
        new_keys.push_back(new_key);
      else
        break;
    }
  }

  keys = std::move(new_keys);
}

void SAnimation::Bake() {
  NNL_EXPECTS(duration >= 1);
  for (auto& sbone_animation : bone_channels) {
    BakeChannel(sbone_animation.scale_keys, duration);
    BakeChannel(sbone_animation.rotation_keys, duration);
    BakeChannel(sbone_animation.translation_keys, duration);

    NNL_ENSURES_DBG(sbone_animation.scale_keys.size() == sbone_animation.rotation_keys.size() &&
                sbone_animation.rotation_keys.size() == sbone_animation.translation_keys.size() &&
                sbone_animation.translation_keys.size() == duration);
  }
}

}  // namespace nnl
