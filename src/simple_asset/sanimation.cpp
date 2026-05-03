
#include "NNL/simple_asset/sanimation.hpp"

#include "simple_asset/sanimation.tpp"

namespace nnl {

void SAnimation::Unbake(float tolerance) {
  for (auto& channel : bone_channels) {
    auto& scale_keys = channel.scale_keys;
    auto& rotation_keys = channel.rotation_keys;
    auto& translation_keys = channel.translation_keys;

    channel.scale_keys = UnbakeChannel_<nnl::Lerp_>(scale_keys, tolerance);
    channel.rotation_keys = UnbakeChannel_<nnl::Slerp_>(rotation_keys, tolerance);
    channel.translation_keys = UnbakeChannel_<nnl::Lerp_>(translation_keys, tolerance);
  }
}

void SAnimation::Bake() {
  NNL_EXPECTS(duration >= 1);

  for (auto& channel : bone_channels) {
    auto& scale_keys = channel.scale_keys;
    auto& rotation_keys = channel.rotation_keys;
    auto& translation_keys = channel.translation_keys;

    if (scale_keys.size() < duration) scale_keys = BakeChannel_<nnl::Lerp_>(scale_keys, duration);

    if (rotation_keys.size() < duration) rotation_keys = BakeChannel_<nnl::Slerp_>(rotation_keys, duration);

    if (translation_keys.size() < duration) translation_keys = BakeChannel_<nnl::Lerp_>(translation_keys, duration);

    NNL_ENSURES_DBG(scale_keys.size() == rotation_keys.size() && rotation_keys.size() == translation_keys.size() &&
                    translation_keys.size() == duration);
  }
}

}  // namespace nnl
