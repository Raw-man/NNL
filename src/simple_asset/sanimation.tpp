#pragma once

#include "NNL/simple_asset/sanimation.hpp"
#include "NNL/utility/math.hpp"

namespace nnl {

inline glm::quat Slerp_(glm::quat a, glm::quat b, float t) { return glm::slerp(a, b, t); }

inline glm::vec3 Lerp_(glm::vec3 a, glm::vec3 b, float t) { return glm::mix(a, b, t); }

inline glm::vec3 LerpShort_(glm::vec3 a, glm::vec3 b, float t) {
  return utl::math::EulerShortLerp(a, b, t);
}

template <auto LerpFunc = Lerp_, typename TKey>
[[nodiscard]] std::vector<TKey> BakeChannel_(const std::vector<TKey>& keys, const std::size_t duration) {
  assert(!keys.empty());
  assert(duration > 0);

  std::vector<TKey> new_keys;
  new_keys.reserve(duration);

  if (auto& key_0 = keys[0]; key_0.time != 0)  // Insert missing keyframes (hold)
  {
    for (std::size_t t = 0; t < key_0.time; t++) {
      TKey new_key;
      new_key.time = t;
      new_key.value = key_0.value;

      if (new_key.time < duration)
        new_keys.push_back(new_key);
      else
        break;
    }
  }

  for (std::size_t k = 0; k < keys.size(); k++) {
    const auto& key = keys[k];

    if (key.time < duration)
      new_keys.push_back(key);
    else
      break;

    if (k + 1 >= keys.size()) break;

    const auto& next_key = keys[k + 1];

    assert(key.time < next_key.time);

    u16 num_extra_keys = next_key.time - key.time;

    for (std::size_t t = 1; t < num_extra_keys; t++) {
      float progression = (float)t / (float)num_extra_keys;

      TKey new_key;
      new_key.time = key.time + t;

      auto val_interp = LerpFunc(key.value, next_key.value, progression);

      new_key.value = val_interp;

      if (new_key.time < duration)
        new_keys.push_back(new_key);
      else
        break;
    }
  }

  assert(!new_keys.empty());

  auto last_key = new_keys.back();

  assert(duration > last_key.time);

  std::size_t num_end_keys = duration - last_key.time;

  for (std::size_t k = 1; k < num_end_keys; k++) {
    TKey new_key = last_key;
    new_key.time += k;

    if (new_key.time < duration)
      new_keys.push_back(new_key);
    else
      break;
  }

  return new_keys;
}

template <auto LerpFunc_ = Lerp_, typename TKey>
[[nodiscard]] std::vector<TKey> UnbakeChannel_(const std::vector<TKey>& keys, const float eq_range = 1.0e-4f) {

  assert(!keys.empty());

  std::vector<TKey> new_keys;
  new_keys.reserve(keys.size());
  std::size_t k_a = 0, k_m = 1, k_b = 2;

  new_keys.push_back(keys.at(k_a));

  while (k_m < keys.size() && k_b < keys.size() ) {

    assert(k_a < keys.size());

    auto key1 = keys[k_a];
    auto key2 = keys[k_m];
    auto key3 = keys[k_b];

    auto v1(key1.value);
    auto v2(key2.value);
    auto v3(key3.value);

    assert(key3.time > key2.time && key2.time > key1.time);

    u16 num_mid_frames = (key3.time - key1.time);

    float progression = (float)(key2.time - key1.time) / (float)num_mid_frames;

    auto v2_interp = LerpFunc_(v1, v3, progression);

    bool can_be_erased = utl::math::IsApproxEqual(v2_interp, v2, eq_range);

    if (can_be_erased) {
      k_m++;
      k_b++;

    } else {
      new_keys.push_back(keys.at(k_m));
      k_a = k_m;

      k_m = k_a+1;

      k_b = k_a+2;

    }
  }

  assert(k_a < keys.size());

  if(k_m < keys.size()) new_keys.push_back(keys.at(k_m));
  if(k_b < keys.size()) new_keys.push_back(keys.at(k_b));

  if (new_keys.size() >= 2) {
    std::size_t ind_last = new_keys.size()-1;
    auto& key1 = new_keys[ind_last-1];
    auto& key2 = new_keys[ind_last];

    assert(key2.time > key1.time);

    auto v1(key1.value);
    auto v2(key2.value);

    if (utl::math::IsApproxEqual(v1, v2, eq_range)) {
      new_keys.pop_back();
    }
  }

  assert(new_keys.size() <= keys.size());

  return new_keys;
}

}  // namespace nnl
