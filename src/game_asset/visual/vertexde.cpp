#include "NNL/game_asset/visual/vertexde.hpp"

#include "NNL/utility/color.hpp"
#include "NNL/utility/math.hpp"
namespace nnl {

namespace vertexde {
std::string GetDescription(u32 vertex_format) {
  auto [offset_weights, offset_uv, offset_color, offset_normal, offset_position, vertex_size] =
      GetLayout(vertex_format);

  std::stringstream description;

  if (IsIndexed(vertex_format)) {
    description << "Indexed; ";
  }

  if (IsThrough(vertex_format)) {
    description << "Through Mode; ";
  }

  if (GetMorphNum(vertex_format) > 1) {
    description << GetMorphNum(vertex_format) << " Morph; ";
  }

  if (HasWeights(vertex_format)) {
    description << GetWeightNum(vertex_format) << " Weight_" << GetWeightSize(vertex_format) * 8 << " off."
                << offset_weights << "; ";
  }

  if (HasUV(vertex_format)) {
    description << "UV_" << GetUVSize(vertex_format) * 8 << " off." << offset_uv << "; ";
  }

  if (HasColor(vertex_format)) {
    description << " Color_" << GetColorSize(vertex_format) * 8;
    if (GetColorFormat(vertex_format) == fmt_code::k565)
      description << " RGB565";
    else if (GetColorFormat(vertex_format) == fmt_code::k5551)
      description << " RGBA5551";
    else if (GetColorFormat(vertex_format) == fmt_code::k4444)
      description << " RGBA4444";
    else if (GetColorFormat(vertex_format) == fmt_code::k8888)
      description << " RGBA8888";

    description << " off." << offset_color << "; ";
  }

  if (HasNormal(vertex_format)) {
    description << "Normal_" << GetNormalSize(vertex_format) * 8 << " off." << offset_normal << "; ";
  }

  if (HasPosition(vertex_format)) {
    description << "Position_" << GetPositionSize(vertex_format) * 8 << " off." << offset_position << "; ";
  }

  description << "Vertex size: " << vertex_size;
  return description.str();
}

VertexLayout GetLayout(u32 vertex_format) {
  VertexLayout layout;

  NNL_EXPECTS(HasPosition(vertex_format));

  layout.offset_weights = 0;

  layout.offset_uv = utl::math::RoundNum(
      layout.offset_weights + GetWeightNum(vertex_format) * GetWeightSize(vertex_format), GetUVSize(vertex_format));

  layout.offset_color = utl::math::RoundNum(layout.offset_uv + GetUVSize(vertex_format) * GetUVNum(vertex_format),
                                            GetColorSize(vertex_format));

  layout.offset_normal =
      utl::math::RoundNum(layout.offset_color + GetColorSize(vertex_format) * 1, GetNormalSize(vertex_format));

  layout.offset_position =
      utl::math::RoundNum(layout.offset_normal + GetNormalSize(vertex_format) * GetNormalNum(vertex_format),
                          GetPositionSize(vertex_format));
  // Size of the vertex struct aligned to its largest member:
  layout.vertex_size =
      utl::math::RoundNum(layout.offset_position + GetPositionSize(vertex_format) * GetPositionNum(vertex_format),
                          std::max({GetWeightSize(vertex_format), GetUVSize(vertex_format), GetColorSize(vertex_format),
                                    GetNormalSize(vertex_format), GetPositionSize(vertex_format)}));

  return layout;
}
// Format weights for further conversion: sort bones of a vertex, add
// missing bones
static std::array<f32, 8> FormatWeights_(const std::array<u16, kMaxNumVertWeight>& vertex_bones,
                                         const std::array<f32, kMaxNumVertWeight>& vertex_weights,
                                         const std::array<u16, 8>& bone_indices, u32 weight_num) {
  assert(weight_num != 0);

  std::array<f32, 8> sorted_weights{0.0f};

  for (std::size_t i = 0; i < kMaxNumVertWeight; i++) {
    u16 bone_index = vertex_bones[i];

    auto itr_bone_beg = bone_indices.begin();
    auto itr_bone_end = itr_bone_beg + weight_num;

    auto itr_ind = std::find(itr_bone_beg, itr_bone_end, bone_index);
    if (itr_ind == itr_bone_end) continue;

    auto pos = std::distance(itr_bone_beg, itr_ind);

    sorted_weights[pos] = vertex_weights[i];
  }

  assert(std::any_of(std::begin(sorted_weights), std::end(sorted_weights), [](f32 w) { return w != 0.0f; }));

  return sorted_weights;
}

Buffer Encode(const std::vector<SVertex>& vertices, u32 vertex_format, std::array<u16, 8> bone_indices) {
  NNL_EXPECTS(HasPosition(vertex_format));

  auto [offset_weights, offset_uv, offset_color, offset_normal, offset_position, vertex_size] =
      GetLayout(vertex_format);

  Buffer vertex_buffer;

  std::size_t num_vertices = vertices.size();
  vertex_buffer.resize(vertices.size() * vertex_size);

  for (std::size_t vertex_index = 0; vertex_index < num_vertices; vertex_index++) {
    const SVertex& vertex = vertices.at(vertex_index);

    const u8* vertex_data = vertex_buffer.data();

    const u8* vertex_ptr = vertex_data + (vertex_index * vertex_size);

    if (HasPosition(vertex_format)) {
      const u8* pos_ptr = vertex_ptr + offset_position;

      for (u16 i = 0; i < GetPositionNum(vertex_format); i++) {
        NNL_EXPECTS_DBG(utl::math::IsFinite(vertex.position[i]));

        // In the original assets, positions are mapped from +-1.0 to +-32735 (not +-32767)
        switch (GetPositionFormat(vertex_format)) {
          case fmt_code::k8:
            *(i8*)(pos_ptr + i * sizeof(i8)) = utl::math::FloatToFixed<i8>(vertex.position[i] * 0.99f);

            break;
          case fmt_code::k16:

            *(i16*)(pos_ptr + i * sizeof(i16)) = utl::math::FloatToFixed<i16>(vertex.position[i] * 0.999f);
            break;
          case fmt_code::k32:
            *(f32*)(pos_ptr + i * sizeof(f32)) = vertex.position[i];
            break;
        }
      }
    }

    if (HasNormal(vertex_format)) {
      const u8* norm_ptr = vertex_ptr + offset_normal;

      for (u16 i = 0; i < GetNormalNum(vertex_format); i++) {
        NNL_EXPECTS_DBG(utl::math::IsFinite(vertex.normal[i]));

        switch (GetNormalFormat(vertex_format)) {
          case fmt_code::k8:
            *(i8*)(norm_ptr + i * sizeof(i8)) = utl::math::FloatToFixed<i8>(vertex.normal[i] * 0.99f);
            break;

          case fmt_code::k16:
            *(i16*)(norm_ptr + i * sizeof(i16)) = utl::math::FloatToFixed<i16>(vertex.normal[i] * 0.999f);
            break;

          case fmt_code::k32:
            *(f32*)(norm_ptr + i * sizeof(f32)) = vertex.normal[i];
            break;
        }
      }
    }

    if (HasWeights(vertex_format)) {
      const u8* weights_ptr = vertex_ptr + offset_weights;

      const u16 weight_num = GetWeightNum(vertex_format);

      std::array<f32, 8> weights = FormatWeights_(vertex.bones, vertex.weights, bone_indices, weight_num);

      assert(weight_num <= weights.size());

      for (u16 i = 0; i < weight_num; i++) {
        NNL_EXPECTS_DBG(utl::math::IsFinite(weights[i]));

        switch (GetWeightFormat(vertex_format)) {
          case fmt_code::k8:
            *(u8*)(weights_ptr + i * sizeof(u8)) = utl::math::FloatToFixed<u8, 1>(weights[i]);
            break;

          case fmt_code::k16:
            *(u16*)(weights_ptr + i * sizeof(u16)) = utl::math::FloatToFixed<u16, 1>(weights[i]);
            break;

          case fmt_code::k32:
            *(f32*)(weights_ptr + i * sizeof(f32)) = weights[i];
            break;
        }
      }
    }

    if (HasUV(vertex_format)) {
      const u8* uv_ptr = vertex_ptr + offset_uv;

      for (u16 i = 0; i < GetUVNum(vertex_format); i++) {
        NNL_EXPECTS_DBG(utl::math::IsFinite(vertex.uv[i]));
        switch (GetUVFormat(vertex_format)) {
          case fmt_code::k8:
            *(u8*)(uv_ptr + i * sizeof(u8)) = utl::math::FloatToFixed<u8, 1>(vertex.uv[i]);
            break;

          case fmt_code::k16:
            *(u16*)(uv_ptr + i * sizeof(u16)) = utl::math::FloatToFixed<u16, 1>(vertex.uv[i]);
            break;

          case fmt_code::k32:

            *(f32*)(uv_ptr + i * sizeof(f32)) = vertex.uv[i];
            break;
        }
      }
    }

    if (HasColor(vertex_format)) {
      const u8* color_ptr = vertex_ptr + offset_color;

      switch (GetColorFormat(vertex_format)) {
        case fmt_code::k565:  // rgb565
          *(u16*)(color_ptr) = utl::color::RGBA8888ToRGB565(utl::color::FloatToInt(vertex.color));
          break;

        case fmt_code::k5551:  // rgba5551
          *(u16*)(color_ptr) = utl::color::RGBA8888ToRGBA5551(utl::color::FloatToInt(vertex.color));
          break;

        case fmt_code::k4444:  // rgba4444
          *(u16*)(color_ptr) = utl::color::RGBA8888ToRGBA4444(utl::color::FloatToInt(vertex.color));
          break;

        case fmt_code::k8888:  // rgba8888
          *(u32*)(color_ptr) = utl::color::FloatToInt(vertex.color);
          break;
      }
    }
  }

  return vertex_buffer;
}

std::vector<SVertex> Decode(BufferView vertex_buffer, u32 vertex_format, std::array<u16, 8> bone_indices) {
  std::vector<SVertex> svertices;

  auto [offset_weights, offset_uv, offset_color, offset_normal, offset_position, vertex_size] =
      GetLayout(vertex_format);

  std::size_t num_vertices = vertex_buffer.size() / (vertex_size);

  svertices.resize(num_vertices);

  for (std::size_t vertex_index = 0; vertex_index < num_vertices; vertex_index++) {
    SVertex& vertex = svertices[vertex_index];

    const u8* vertex_data = vertex_buffer.data();

    const u8* vertex_ptr = vertex_data + (vertex_index * vertex_size);

    if (HasPosition(vertex_format)) {
      const u8* pos_ptr = vertex_ptr + offset_position;
      // In the original assets, positions are mapped from +-1.0 to +-32735 (not +-32767)
      for (u16 i = 0; i < GetPositionNum(vertex_format); i++) {
        switch (GetPositionFormat(vertex_format)) {
          case fmt_code::k8:
            vertex.position[i] = (1.0f / 0.99f) * utl::math::FixedToFloat<i8>(*(i8*)(pos_ptr + i * sizeof(i8)));
            break;

          case fmt_code::k16:
            vertex.position[i] = (1.0f / 0.999f) * utl::math::FixedToFloat<i16>(*(i16*)(pos_ptr + i * sizeof(i16)));
            break;

          case fmt_code::k32:
            vertex.position[i] = *(f32*)(pos_ptr + i * sizeof(f32));
            break;
        }
      }
    }

    if (HasNormal(vertex_format)) {
      const u8* norm_ptr = vertex_ptr + offset_normal;

      for (u16 i = 0; i < GetNormalNum(vertex_format); i++) {
        switch (GetNormalFormat(vertex_format)) {
          case fmt_code::k8:
            vertex.normal[i] = (1.0f / 0.99f) * utl::math::FixedToFloat<i8>(*(i8*)(norm_ptr + i * sizeof(i8)));
            break;

          case fmt_code::k16:
            vertex.normal[i] = (1.0f / 0.999f) * utl::math::FixedToFloat<i16>(*(i16*)(norm_ptr + i * sizeof(i16)));
            break;

          case fmt_code::k32:
            vertex.normal[i] = *(f32*)(norm_ptr + i * sizeof(f32));
            break;
        }
      }
    }

    if (HasUV(vertex_format)) {
      const u8* uv_ptr = vertex_ptr + offset_uv;

      for (u16 i = 0; i < GetUVNum(vertex_format); i++) {
        switch (GetUVFormat(vertex_format)) {
          case fmt_code::k8:
            vertex.uv[i] = utl::math::FixedToFloat<u8, 1>(*((u8*)(uv_ptr + i * sizeof(u8))));
            break;

          case fmt_code::k16:
            vertex.uv[i] = utl::math::FixedToFloat<u16, 1>(*((u16*)(uv_ptr + i * sizeof(u16))));
            break;

          case fmt_code::k32:
            vertex.uv[i] = *(f32*)(uv_ptr + i * sizeof(f32));
            break;
        }
      }
    }

    if (HasColor(vertex_format)) {
      const u8* color_ptr = vertex_ptr + offset_color;

      for (u16 i = 0; i < 1; i++) {
        u32 color = *(u16*)(color_ptr);

        switch (GetColorFormat(vertex_format)) {
          case fmt_code::k565:  // rgb565
            vertex.color[0] = utl::color::Convert5To8(color & 0x1f) / 255.0f;
            vertex.color[1] = utl::color::Convert6To8((color >> 5) & 0x3f) / 255.0f;
            vertex.color[2] = utl::color::Convert5To8((color >> 11) & 0x1f) / 255.0f;
            vertex.color[3] = 1.0f;
            break;

          case fmt_code::k5551:  // rgba5551
            vertex.color[0] = utl::color::Convert5To8(color & 0x1f) / 255.0f;
            vertex.color[1] = utl::color::Convert5To8((color >> 5) & 0x1f) / 255.0f;
            vertex.color[2] = utl::color::Convert5To8((color >> 10) & 0x1f) / 255.0f;
            vertex.color[3] = (color >> 15) ? 1.0f : 0.0f;
            break;

          case fmt_code::k4444:  // rgba4444
            vertex.color[0] = utl::color::Convert4To8((color >> (0 * 4)) & 0xF) / 255.0f;
            vertex.color[1] = utl::color::Convert4To8((color >> (1 * 4)) & 0xF) / 255.0f;
            vertex.color[2] = utl::color::Convert4To8((color >> (2 * 4)) & 0xF) / 255.0f;
            vertex.color[3] = utl::color::Convert4To8((color >> (3 * 4)) & 0xF) / 255.0f;
            break;

          case fmt_code::k8888:  // rgba8888
            color = *(u32*)(color_ptr);
            vertex.color[0] = (color & 0xFF) / 255.0f;
            vertex.color[1] = ((color >> 8) & 0xFF) / 255.0f;
            vertex.color[2] = ((color >> 16) & 0xFF) / 255.0f;
            vertex.color[3] = ((color >> 24) & 0xFF) / 255.0f;
            ;
            break;
        }
      }
    }

    if (HasWeights(vertex_format)) {
      const u8* weights_ptr = vertex_ptr + offset_weights;

      const u16 weight_num = GetWeightNum(vertex_format);

      for (u16 i = 0, j = 0; i < weight_num && j < kMaxNumVertWeight; i++) {
        f32 weight = 0.0f;
        switch (GetWeightFormat(vertex_format)) {
          case fmt_code::k8:
            weight = utl::math::FixedToFloat<u8, 1>(*(u8*)(weights_ptr + i * sizeof(u8)));
            break;

          case fmt_code::k16:
            weight = utl::math::FixedToFloat<u16, 1>(*(u16*)(weights_ptr + i * sizeof(u16)));
            break;

          case fmt_code::k32:
            weight = *(f32*)(weights_ptr + i * sizeof(f32));
            break;
        }
        if (weight > 0.0f) {
          vertex.bones[j] = bone_indices[i];
          vertex.weights[j] = weight;
          j++;
        }
      }
    } else  // always at least 1 bone
    {
      vertex.bones[0] = bone_indices[0];
      vertex.weights[0] = 1.0f;
    }
  }
  return svertices;
}

}  // namespace vertexde

}  // namespace nnl
