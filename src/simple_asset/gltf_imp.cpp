#include <stb_image.h>
#include <stb_image_write.h>
#include <tiny_gltf.h>

#include <numeric>

#include "NNL/common/io.hpp"
#include "NNL/common/logger.hpp"
#include "NNL/simple_asset/sasset3d.hpp"
#include "NNL/utility/color.hpp"
#include "NNL/utility/data.hpp"
#include "NNL/utility/filesys.hpp"
#include "NNL/utility/math.hpp"
#include "NNL/utility/string.hpp"
namespace nnl {

template <typename T>
constexpr inline float NormalizeToFloat(T value) {
  if constexpr (std::is_signed<T>::value)
    return std::max(static_cast<float>(value) / std::numeric_limits<T>::max(), -1.0f);
  else
    return static_cast<float>(value) / std::numeric_limits<T>::max();
}

template <typename D, typename S>
std::vector<D> NormalizeToFloat(const std::vector<S>& vec) {
  static_assert(std::is_same_v<typename D::value_type, float>, "");

  static_assert(S::length() == D::length(), "");

  std::vector<D> n_vec(vec.size());

  for (std::size_t k = 0; k < vec.size(); k++) {
    auto& v = vec[k];

    auto& n_v = n_vec[k];

    for (std::size_t i = 0; i < v.length(); i++) {
      n_v[i] = NormalizeToFloat(v[i]);
    }
  }

  return n_vec;
}

template <typename D, typename S>
std::vector<D> ExtendVec(const std::vector<S>& vec, typename D::value_type value) {
  static_assert(S::length() <= D::length(), "");

  std::vector<D> n_vec(vec.size());

  for (std::size_t k = 0; k < vec.size(); k++) {
    auto& v = vec[k];

    auto& n_v = n_vec[k];

    for (std::size_t i = 0; i < v.length(); i++) {
      n_v[i] = v[i];
    }

    for (std::size_t i = v.length(); i < n_v.length(); i++) {
      n_v[i] = value;
    }
  }

  return n_vec;
}

class GLTFImporter {
 public:
  GLTFImporter(SAsset3D& sasset, const std::filesystem::path& path, bool flip_uv, bool decode_images)
      : sasset_(sasset),
        path_(path),
        filename_(utl::filesys::u8string(path.filename())),
        flip_uv_(flip_uv),
        decode_images_(decode_images) {
    Import();
  }

  GLTFImporter(SAsset3D& sasset, BufferView buffer, const std::filesystem::path& base_path, bool flip_uv,
               bool decode_images)
      : sasset_(sasset),
        buffer_(buffer),
        path_(base_path),
        filename_(utl::filesys::u8string(path_.filename())),
        flip_uv_(flip_uv),
        decode_images_(decode_images) {
    Import();
  }

 private:
  SAsset3D& sasset_;

  BufferView buffer_{};

  const std::filesystem::path path_;

  const std::string filename_;

  const bool flip_uv_;
  const bool decode_images_;

  const std::set<std::string_view> extensions_supported_{
      "KHR_texture_transform",  "KHR_lights_punctual", "KHR_materials_variants", "KHR_animation_pointer",
      "KHR_materials_specular", "KHR_materials_unlit", "KHR_mesh_quantization",  "KHR_node_visibility"};

  tinygltf::Model gltf_model_;

  tinygltf::TinyGLTF gltf_ctx_;

  std::string error_;

  std::string warn_;

  std::map<int, glm::mat4> node_inverse_;  // joint, inverse bind matrix

  std::set<int> mesh_nodes_;

  std::map<int, std::vector<std::size_t>> mesh_node_smesh_id_;

  std::set<int> root_joint_nodes_;

  std::set<int> light_nodes_;

  std::set<int> position_nodes_;

  std::set<std::size_t> used_mesh_groups_{0};

  std::map<int, std::size_t> node_mesh_group_id_;

  std::map<int, glm::mat4> node_parent_transform_;

  std::map<int, glm::mat4> node_global_transform_;

  std::map<int, std::tuple<glm::vec3, glm::quat, glm::vec3>> node_local_transform_;

  std::set<int> node_new_transform_;  // Root nodes (joints) that should inherit
                                     // their parent transform

  std::map<int, std::size_t> joint_bone_id_;  // A node id to a bone index

  std::map<std::size_t, int> bone_joint_id_;  // A bone index to a node id

  std::map<int, bool> animated_srt_nodes_;  // All nodes whose SRT props are animated

  std::map<int, bool> bone_attachment_nodes_;

  std::map<int, bool> joints_;

  std::map<int, int> image_texture_id_;

  std::map<std::size_t, glm::mat3> material_texture_transform_;

  std::vector<std::string> unsorted_variants_;

  std::set<std::string, decltype(&utl::string::CompareNat)> sorted_variants_{&utl::string::CompareNat};

  std::size_t num_bones_ = 0;

  std::size_t num_mesh_groups_ = 1;

  int dummy_root_id_ = -1;

  std::size_t default_mat_id_ = -1;

  const std::map<int, STextureFilter> kGltfToSTextureFilter{
      {TINYGLTF_TEXTURE_FILTER_NEAREST, STextureFilter::kNearest},
      {TINYGLTF_TEXTURE_FILTER_LINEAR, STextureFilter::kLinear},
      {TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST, STextureFilter::kNearestMipmapNearest},
      {TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST, STextureFilter::kLinearMipmapNearest},
      {TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR, STextureFilter::kNearestMipmapLinear},
      {TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR, STextureFilter::kLinearMipmapLinear}};

  template <typename T>
  std::vector<T> ExtractBuffer(int buffer_view_id, std::size_t count, std::size_t offset) {
    auto& buf_view = gltf_model_.bufferViews.at(buffer_view_id);

    auto& buffer = gltf_model_.buffers.at(buf_view.buffer);

    std::size_t stride = buf_view.byteStride != 0 ? buf_view.byteStride : sizeof(T);

    const unsigned char* data_ptr = buffer.data.data() + buf_view.byteOffset + offset;

    const unsigned char* data_end_ptr = buffer.data.data() + buf_view.byteOffset + offset + (stride * count);

    const unsigned char* buf_end_ptr = buffer.data.data() + buffer.data.size();

    if (data_end_ptr > buf_end_ptr) NNL_THROW(RangeError(NNL_ERMSG("buffer out of range")));

    std::vector<T> data;

    data.reserve(count);

    for (std::size_t i = 0; i < count; i++) {
      data.push_back(*reinterpret_cast<const T*>(data_ptr + stride * i));
    }

    return data;
  }

  template <typename T>
  std::vector<T> ExtractData(int accessor_id) {
    auto& accessor = gltf_model_.accessors.at(accessor_id);

    auto data = ExtractBuffer<T>(accessor.bufferView, accessor.count, accessor.byteOffset);

    if (accessor.sparse.isSparse) {
      std::vector<u32> indices_sparse;

      switch (accessor.sparse.indices.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
          indices_sparse = utl::data::CastContainer<u32>(ExtractBuffer<u8>(
              accessor.sparse.indices.bufferView, accessor.sparse.count, accessor.sparse.indices.byteOffset));
          break;

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
          indices_sparse = utl::data::CastContainer<u32>(ExtractBuffer<u16>(
              accessor.sparse.indices.bufferView, accessor.sparse.count, accessor.sparse.indices.byteOffset));
          break;

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
          indices_sparse = ExtractBuffer<u32>(accessor.sparse.indices.bufferView, accessor.sparse.count,
                                              accessor.sparse.indices.byteOffset);
          break;

        default:
          NNL_THROW(ParseError(NNL_ERMSG("invalid gltf index format")));
      }

      auto data_sparse =
          ExtractBuffer<T>(accessor.sparse.values.bufferView, accessor.sparse.count, accessor.sparse.values.byteOffset);

      for (std::size_t i = 0; i < indices_sparse.size(); i++) {
        data.at(indices_sparse.at(i)) = data_sparse.at(i);
      }
    }

    return data;
  }

  glm::mat4 GLTFMatToGLM(const tinygltf::Node& node) {
    glm::mat4 matrix(1.0f);

    const auto& mat_v = node.matrix;

    if (!mat_v.empty()) {
      matrix[0] = glm::vec4(mat_v[0], mat_v[1], mat_v[2], mat_v[3]);
      matrix[1] = glm::vec4(mat_v[4], mat_v[5], mat_v[6], mat_v[7]);
      matrix[2] = glm::vec4(mat_v[8], mat_v[9], mat_v[10], mat_v[11]);
      matrix[3] = glm::vec4(mat_v[12], mat_v[13], mat_v[14], mat_v[15]);
    } else {
      glm::mat4 scale(1.0f);

      glm::mat4 rotation(1.0f);

      glm::mat4 translation(1.0f);

      const auto& scale_v = node.scale;

      const auto& rotation_v = node.rotation;

      const auto& translation_v = node.translation;

      if (!scale_v.empty()) {
        scale[0][0] = scale_v[0];
        scale[1][1] = scale_v[1];
        scale[2][2] = scale_v[2];
      }

      if (!translation_v.empty()) {
        translation[3] = glm::vec4(translation_v[0], translation_v[1], translation_v[2], 1.0f);
      }

      if (!rotation_v.empty()) {
        rotation = glm::toMat4(glm::quat(rotation_v[3], rotation_v[0], rotation_v[1], rotation_v[2]));
      }

      matrix = translation * rotation * scale;
    }

    return matrix;
  }

  std::tuple<glm::vec3, glm::quat, glm::vec3> GLTFMatToGLMSRT(const tinygltf::Node& node) {
    if (!node.matrix.empty()) {
      glm::mat4 matrix{1.0f};

      const auto& mat_v = node.matrix;

      matrix[0] = glm::vec4(mat_v[0], mat_v[1], mat_v[2], mat_v[3]);
      matrix[1] = glm::vec4(mat_v[4], mat_v[5], mat_v[6], mat_v[7]);
      matrix[2] = glm::vec4(mat_v[8], mat_v[9], mat_v[10], mat_v[11]);
      matrix[3] = glm::vec4(mat_v[12], mat_v[13], mat_v[14], mat_v[15]);

      auto [scale, rotation, translation] = utl::math::Decompose(matrix);

      return {scale, rotation, translation};
    } else {
      glm::vec3 scale{1.0f, 1.0f, 1.0f};

      glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

      glm::vec3 translation{0.0f, 0.0f, 0.0f};

      const auto& scale_v = node.scale;

      const auto& rotation_v = node.rotation;

      const auto& translation_v = node.translation;

      if (!scale_v.empty()) {
        scale = glm::vec3(scale_v[0], scale_v[1], scale_v[2]);
      }

      if (!translation_v.empty()) {
        translation = glm::vec3(translation_v[0], translation_v[1], translation_v[2]);
      }

      if (!rotation_v.empty()) {
        rotation = glm::quat(rotation_v[3], rotation_v[0], rotation_v[1], rotation_v[2]);
      }

      return {scale, rotation, translation};
    }
  }

  SValue GLTFValueToSValue(const tinygltf::Value& o) {
    using namespace tinygltf;

    SValue val{};

    switch (o.Type()) {
      case tinygltf::OBJECT_TYPE: {
        SValue::Object value_object;
        for (auto& [key, value] : o.Get<Value::Object>() /*auto it = o.begin(); it != o.end(); it++*/) {
          SValue entry = GLTFValueToSValue(value);
          if (!entry.IsNull()) value_object.emplace(key, std::move(entry));
        }
        if (value_object.size() > 0) val = SValue(std::move(value_object));
      } break;
      case tinygltf::ARRAY_TYPE: {
        SValue::Array value_array;
        value_array.reserve(o.Size());
        for (auto& value : o.Get<Value::Array>() /*auto it = o.begin(); it != o.end(); it++*/) {
          SValue entry = GLTFValueToSValue(value);
          if (!entry.IsNull()) value_array.emplace_back(std::move(entry));
        }
        if (value_array.size() > 0) val = SValue(std::move(value_array));
      } break;
      case tinygltf::STRING_TYPE:
        val = SValue(o.Get<std::string>());
        break;
      case tinygltf::BOOL_TYPE:
        val = SValue(o.Get<bool>());
        break;
      case tinygltf::INT_TYPE:
        val = SValue(o.Get<int>());
        break;
      case tinygltf::REAL_TYPE:
        val = SValue(o.Get<double>());
        break;
      case tinygltf::NULL_TYPE:
      default:
        break;
    }

    return val;
  }

  void ProcessMesh(int mesh_node_id) {
    auto& node = gltf_model_.nodes.at(mesh_node_id);

    const tinygltf::Mesh& mesh = gltf_model_.meshes.at(node.mesh);

    glm::mat4 transform = glm::mat4(1.0f);

    std::size_t mesh_group = 0;

    // Only apply the transform if the mesh is not animated
    if (node.skin == -1 && animated_srt_nodes_.count(mesh_node_id) == 0 &&
        node_global_transform_.count(mesh_node_id) != 0) {
      transform = node_global_transform_.at(mesh_node_id);
    }

    float det = glm::determinant(transform);

    if (node.extras.Has("mesh_group")) mesh_group = node.extras.Get("mesh_group").GetNumberAsInt();

    if (mesh_group + 1 > num_mesh_groups_) num_mesh_groups_ = mesh_group + 1;

    std::string name =
        !node.name.empty() ? node.name : (!mesh.name.empty() ? mesh.name : std::to_string(sasset_.model.meshes.size()));

    if (mesh.primitives.empty()) {
      NNL_LOG_WARN("mesh \"" + name + "\" has no primitives");
      return;
    }

    for (std::size_t i = 0; i < mesh.primitives.size(); i++) {
      auto& primitive = mesh.primitives[i];

      if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
        NNL_LOG_WARN("mesh \"" + name + "\" uses unsupported primitive mode: " + std::to_string(primitive.mode));
        continue;
      }

      mesh_node_smesh_id_[mesh_node_id].push_back(sasset_.model.meshes.size());

      SMesh& smesh = sasset_.model.meshes.emplace_back();

      smesh.name = i == 0 ? name : name + ".Split" + utl::string::PrependZero(i);

      smesh.mesh_group = mesh_group;

      smesh.extras = GLTFValueToSValue(node.extras);

      smesh.extras.Erase("mesh_group");

      if (smesh.extras.IsEmpty()) smesh.extras = nullptr;

      if (primitive.material != -1)
        smesh.material_id = primitive.material;
      else {
        if (default_mat_id_ == static_cast<std::size_t>(-1)) {
          default_mat_id_ = CreateDefaultMaterial();
        }
        smesh.material_id = default_mat_id_;
      }

      if (primitive.extensions.count("KHR_materials_variants")) {
        auto& khr_materials_variants = primitive.extensions.at("KHR_materials_variants");

        auto& mappings = khr_materials_variants.Get("mappings");

        for (std::size_t j = 0; j < mappings.Size(); j++) {
          auto& mapping = mappings.Get(j);

          std::size_t mat_id = mapping.Get("material").GetNumberAsInt();

          auto& variants = mapping.Get("variants");

          for (std::size_t k = 0; k < variants.Size(); k++) {
            std::size_t variant = variants.Get(k).GetNumberAsInt();
            auto itr_name = sorted_variants_.find(unsorted_variants_.at(variant));
            std::size_t pos = std::distance(sorted_variants_.begin(), itr_name);
            smesh.material_variants[mat_id].push_back(pos);
          }
        }
      }

      glm::mat3 texture_transform = material_texture_transform_.at(smesh.material_id);

      SMaterial& smat = sasset_.model.materials.at(smesh.material_id);

      {
        std::vector<Vec3<float>> positions;

        auto& accessor = gltf_model_.accessors.at(primitive.attributes.at("POSITION"));

        if (!accessor.normalized) {
          switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
              positions =
                  utl::data::CastContainer<Vec3<float>>(ExtractData<Vec3<u8>>(primitive.attributes.at("POSITION")));
              break;
            case TINYGLTF_COMPONENT_TYPE_BYTE:
              positions =
                  utl::data::CastContainer<Vec3<float>>(ExtractData<Vec3<i8>>(primitive.attributes.at("POSITION")));
              break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
              positions =
                  utl::data::CastContainer<Vec3<float>>(ExtractData<Vec3<u16>>(primitive.attributes.at("POSITION")));
              break;
            case TINYGLTF_COMPONENT_TYPE_SHORT:
              positions =
                  utl::data::CastContainer<Vec3<float>>(ExtractData<Vec3<i16>>(primitive.attributes.at("POSITION")));
              break;
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
              positions = ExtractData<Vec3<float>>(primitive.attributes.at("POSITION"));
              break;
            default:
              NNL_THROW(ParseError(NNL_ERMSG("mesh \"" + smesh.name + "\" uses invalid position type")));
          }
        } else {
          switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
              positions = NormalizeToFloat<Vec3<float>>(ExtractData<Vec3<u8>>(primitive.attributes.at("POSITION")));
              break;
            case TINYGLTF_COMPONENT_TYPE_BYTE:
              positions = NormalizeToFloat<Vec3<float>>(ExtractData<Vec3<i8>>(primitive.attributes.at("POSITION")));
              break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
              positions = NormalizeToFloat<Vec3<float>>(ExtractData<Vec3<u16>>(primitive.attributes.at("POSITION")));
              break;
            case TINYGLTF_COMPONENT_TYPE_SHORT:
              positions = NormalizeToFloat<Vec3<float>>(ExtractData<Vec3<i16>>(primitive.attributes.at("POSITION")));
              break;
            default:
              NNL_THROW(ParseError(NNL_ERMSG("mesh \"" + smesh.name + "\" uses invalid position type")));
          }
        }

        smesh.vertices.resize(positions.size());

        for (std::size_t i = 0; i < smesh.vertices.size(); i++) {
          smesh.vertices[i].position = transform * glm::vec4(positions[i].x, positions[i].y, positions[i].z, 1.0f);
        }
      }

      if (primitive.attributes.find("NORMAL") != primitive.attributes.end() &&
          (smat.lit || smat.projection_mode != STextureProjection::kUV)) {
        std::vector<Vec3<float>> normals;

        smesh.uses_normal = true;

        auto& accessor = gltf_model_.accessors.at(primitive.attributes.at("NORMAL"));

        switch (accessor.componentType) {
          case TINYGLTF_COMPONENT_TYPE_BYTE:
            normals = NormalizeToFloat<Vec3<float>>(ExtractData<Vec3<i8>>(primitive.attributes.at("NORMAL")));
            break;
          case TINYGLTF_COMPONENT_TYPE_SHORT:
            normals = NormalizeToFloat<Vec3<float>>(ExtractData<Vec3<i16>>(primitive.attributes.at("NORMAL")));
            break;
          case TINYGLTF_COMPONENT_TYPE_FLOAT:
            normals = ExtractData<Vec3<float>>(primitive.attributes.at("NORMAL"));
            break;
          default:
            NNL_THROW(ParseError(NNL_ERMSG("mesh \"" + smesh.name + "\" uses invalid normal type")));
        }

        if (det != 0.0f) {
          for (std::size_t i = 0; i < smesh.vertices.size(); i++) {
            smesh.vertices[i].normal = glm::normalize(glm::transpose(utl::math::Inverse(glm::mat3(transform))) *
                                                      glm::vec3(normals[i].x, normals[i].y, normals[i].z));

            assert(utl::math::IsFinite(smesh.vertices[i].normal));
          }
        }
      }

      if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end() && smat.texture_id != -1 &&
          smat.projection_mode == STextureProjection::kUV) {
        std::vector<Vec2<float>> uvs;

        smesh.uses_uv = true;

        auto& accessor = gltf_model_.accessors.at(primitive.attributes.at("TEXCOORD_0"));

        if (!accessor.normalized) {
          switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
              uvs = utl::data::CastContainer<Vec2<float>>(ExtractData<Vec2<u8>>(primitive.attributes.at("TEXCOORD_0")));
              break;
            case TINYGLTF_COMPONENT_TYPE_BYTE:
              uvs = utl::data::CastContainer<Vec2<float>>(ExtractData<Vec2<i8>>(primitive.attributes.at("TEXCOORD_0")));
              break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
              uvs =
                  utl::data::CastContainer<Vec2<float>>(ExtractData<Vec2<u16>>(primitive.attributes.at("TEXCOORD_0")));
              break;
            case TINYGLTF_COMPONENT_TYPE_SHORT:
              uvs =
                  utl::data::CastContainer<Vec2<float>>(ExtractData<Vec2<i16>>(primitive.attributes.at("TEXCOORD_0")));
              break;
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
              uvs = ExtractData<Vec2<float>>(primitive.attributes.at("TEXCOORD_0"));
              break;
            default:
              NNL_THROW(ParseError(NNL_ERMSG("mesh \"" + smesh.name + "\" uses invalid uv type")));
          }
        } else {
          switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_BYTE:
              uvs = NormalizeToFloat<Vec2<float>>(ExtractData<Vec2<i8>>(primitive.attributes.at("TEXCOORD_0")));
              break;

            case TINYGLTF_COMPONENT_TYPE_SHORT:
              uvs = NormalizeToFloat<Vec2<float>>(ExtractData<Vec2<i16>>(primitive.attributes.at("TEXCOORD_0")));
              break;
            default:
              NNL_THROW(ParseError(NNL_ERMSG("mesh \"" + smesh.name + "\" uses invalid uv type")));
          }
        }

        glm::mat3 flip_v{1.0f};

        if (flip_uv_) {
          flip_v[1][1] = -1;
          flip_v[2][1] = 1;
        }

        for (std::size_t i = 0; i < smesh.vertices.size(); i++) {
          smesh.vertices[i].uv = flip_v * texture_transform * glm::vec3(uvs[i].x, uvs[i].y, 1.0f);
        }
      }

      if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
        std::vector<Vec4<float>> colors;

        smesh.uses_color = true;

        auto& accessor = gltf_model_.accessors.at(primitive.attributes.at("COLOR_0"));

        switch (accessor.type) {
          case TINYGLTF_TYPE_VEC3: {
            switch (accessor.componentType) {
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                colors = ExtendVec<Vec4<float>>(
                    NormalizeToFloat<Vec3<float>>(ExtractData<Vec3<u8>>(primitive.attributes.at("COLOR_0"))), 1.0f);
                break;

              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                colors = ExtendVec<Vec4<float>>(
                    NormalizeToFloat<Vec3<float>>(ExtractData<Vec3<u16>>(primitive.attributes.at("COLOR_0"))), 1.0f);
                break;

              case TINYGLTF_COMPONENT_TYPE_FLOAT:
                colors = ExtendVec<Vec4<float>>(ExtractData<Vec3<float>>(primitive.attributes.at("COLOR_0")), 1.0f);
                break;

              default:
                NNL_THROW(ParseError(NNL_ERMSG("mesh \"" + smesh.name + "\" uses invalid color type")));
            }
          } break;
          case TINYGLTF_TYPE_VEC4: {
            switch (accessor.componentType) {
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                colors = NormalizeToFloat<Vec4<float>>(ExtractData<Vec4<u8>>(primitive.attributes.at("COLOR_0")));
                break;

              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                colors = NormalizeToFloat<Vec4<float>>(ExtractData<Vec4<u16>>(primitive.attributes.at("COLOR_0")));
                break;

              case TINYGLTF_COMPONENT_TYPE_FLOAT:
                colors = ExtractData<Vec4<float>>(primitive.attributes.at("COLOR_0"));
                break;

              default:
                NNL_THROW(ParseError(NNL_ERMSG("mesh \"" + smesh.name + "\" uses invalid color type")));
            }
          } break;
        }

        for (std::size_t i = 0; i < smesh.vertices.size(); i++) {
          smesh.vertices[i].color =
              glm::vec4(smat.lit ? smat.diffuse : smat.ambient, smat.opacity) *
              /*utl::color::LinearToSRGB(*/ glm::vec4(colors[i].x, colors[i].y, colors[i].z, colors[i].w) /*)*/;

          smesh.vertices[i].color = glm::clamp(smesh.vertices[i].color, 0.0f, 1.0f);
        }
      }

      const auto ProcessJointsN = [&](const std::size_t joint_index) {
        std::vector<Vec4<u16>> joints_0;
        std::vector<Vec4<float>> weights_0;

        const std::string attr_joint = "JOINTS_" + std::to_string(joint_index);
        const std::string attr_weight = "WEIGHTS_" + std::to_string(joint_index);

        auto skin_id = node.skin;

        auto& accessor = gltf_model_.accessors.at(primitive.attributes.at(attr_joint));

        switch (accessor.componentType) {
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            joints_0 = utl::data::CastContainer<Vec4<u16>>(ExtractData<Vec4<u8>>(primitive.attributes.at(attr_joint)));
            break;
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            joints_0 = ExtractData<Vec4<u16>>(primitive.attributes.at(attr_joint));
            break;
          default:
            NNL_THROW(ParseError(NNL_ERMSG("mesh \"" + smesh.name + "\" uses invalid joint index type")));
        }

        auto& w_accessor = gltf_model_.accessors.at(primitive.attributes.at(attr_weight));

        switch (w_accessor.componentType) {
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            weights_0 = NormalizeToFloat<Vec4<float>>(ExtractData<Vec4<u8>>(primitive.attributes.at(attr_weight)));
            break;
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            weights_0 = NormalizeToFloat<Vec4<float>>(ExtractData<Vec4<u16>>(primitive.attributes.at(attr_weight)));
            break;
          case TINYGLTF_COMPONENT_TYPE_FLOAT:
            weights_0 = ExtractData<Vec4<float>>(primitive.attributes.at(attr_weight));
            break;
          default:
            NNL_THROW(ParseError(NNL_ERMSG("mesh \"" + smesh.name + "\" uses invalid joint weight type")));
        }

        for (std::size_t i = 0; i < smesh.vertices.size(); i++) {
          auto& joint = joints_0.at(i);
          auto& weight = weights_0.at(i);

          for (std::size_t j = 0; j < 4; j++)
            if (weight[j] > 0.0f)
              smesh.vertices[i].weights.push_back(
                  {(u16)joint_bone_id_.at(gltf_model_.skins.at(skin_id).joints.at(joint[j])), weight[j]});
        }
      };

      if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
        ProcessJointsN(0);
      } else  // Must be attached to some joint anyway
      {
        if (joint_bone_id_.count(mesh_node_id) != 0) {  // Attach to bones inserted for animated objects
          for (auto& vertex : smesh.vertices) vertex.weights.push_back({joint_bone_id_.at(mesh_node_id), 1.0f});

        } else {
          if (dummy_root_id_ == -1) CreateDummyRootBone();  // Attach to a dummy root

          for (auto& vertex : smesh.vertices) vertex.weights.push_back({joint_bone_id_.at(dummy_root_id_), 1.0f});
        }
      }

      if (primitive.attributes.find("JOINTS_1") != primitive.attributes.end() &&
          primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
        ProcessJointsN(1);
      }

      if (primitive.attributes.find("JOINTS_2") != primitive.attributes.end()) {
        NNL_LOG_WARN("mesh \"" + smesh.name + "\" has more than 8 weights per vertex");
      }

      if (primitive.indices != -1) {
        auto& ind_accessor = gltf_model_.accessors.at(primitive.indices);

        switch (ind_accessor.componentType) {
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            smesh.indices = utl::data::CastContainer<u32>(ExtractData<u8>(primitive.indices));
            break;

          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            smesh.indices = utl::data::CastContainer<u32>(ExtractData<u16>(primitive.indices));
            break;

          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            smesh.indices = ExtractData<u32>(primitive.indices);
            break;

          default:
            NNL_THROW(ParseError(NNL_ERMSG("mesh \"" + smesh.name + "\" uses invalid index type")));
        }
      } else {
        smesh.indices.resize(smesh.vertices.size());

        std::iota(smesh.indices.begin(), smesh.indices.end(), 0);
      }

      if (smesh.indices.size() % 3 != 0)
        NNL_THROW(nnl::ParseError(NNL_ERMSG("mesh \"" + smesh.name + "\" has invalid number of indices")));

      if (smesh.indices.empty() || smesh.vertices.empty()) {
        NNL_LOG_WARN("mesh \"" + smesh.name + "\" is empty and will be ignored");
        sasset_.model.meshes.pop_back();
        continue;
      }

      if (det < 0.0f) smesh.ReverseWindingOrder();

      if (primitive.attributes.find("NORMAL") == primitive.attributes.end() || det == 0.0f) {
        if (smat.lit && smat.projection_mode == STextureProjection::kUV) {
          smesh.GenerateFlatNormals();
        }

        if (smat.projection_mode != STextureProjection::kUV) {
          smesh.GenerateSmoothNormals();
        }
      }

      smesh.Optimize();
    }
  }

  std::size_t CreateDefaultMaterial() {
    std::size_t default_mat_id = gltf_model_.materials.size();
    material_texture_transform_[default_mat_id] = glm::mat3(1.0f);
    auto& smat = sasset_.model.materials.emplace_back();
    smat.lit = true;
    smat.name = "default";
    return default_mat_id;
  };

  int CreateDummyRootNode() {
    int dummy_root = static_cast<int>(gltf_model_.nodes.size());

    node_inverse_[dummy_root] = glm::mat4(1.0f);

    node_global_transform_[dummy_root] = glm::mat4(1.0f);

    node_local_transform_[dummy_root] = {glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(0.0f)};

    root_joint_nodes_.insert(dummy_root);

    auto& new_node = gltf_model_.nodes.emplace_back();

    new_node.name = "VFX0";

    return dummy_root;
  }

  std::size_t CreateDummyRootBone() {
    dummy_root_id_ = CreateDummyRootNode();

    joint_bone_id_[dummy_root_id_] = num_bones_;
    bone_joint_id_[num_bones_] = dummy_root_id_;

    SBone dummy_root;
    dummy_root.name = "0";

    sasset_.model.skeleton.roots.push_back(dummy_root);

    return num_bones_++;
  }

  void ProcessAttachment(SBone& bone, const int parent_bone, int node_id, glm::mat4 global_transform) {
    auto& node = gltf_model_.nodes.at(node_id);

    bone_attachment_nodes_[node_id] = true;

    if (node.mesh != -1) mesh_nodes_.insert(node_id);

    joint_bone_id_[node_id] = joint_bone_id_.at(parent_bone);

    glm::mat4 local_matrix = GLTFMatToGLM(node);

    glm::mat4 global_matrix = global_transform * local_matrix;

    node_parent_transform_[node_id] = global_transform;

    // Set accum transform of attachments to a "global" matrix that resets to identity at each
    // newly inserted bone
    node_global_transform_[node_id] = global_matrix;

    node_local_transform_[node_id] = GLTFMatToGLMSRT(node);

    if (node.extras.Has("id")) {
      SAttachment& sattachment = sasset_.model.attachments.emplace_back();

      sattachment.bone = joint_bone_id_.at(parent_bone);

      sattachment.SetTransform(global_matrix);

      sattachment.id = node.extras.Get("id").GetNumberAsInt();
    }

    for (auto child_node_id : node.children) {
      if (animated_srt_nodes_.count(child_node_id) == 0) {
        ProcessAttachment(bone, parent_bone, child_node_id, global_matrix);
      } else {
        // If a child is an animated node (of non animated parent), insert a new bone
        tinygltf::Node& node = gltf_model_.nodes.at(child_node_id);

        SBone& child = bone.children.emplace_back();

        joint_bone_id_[child_node_id] = num_bones_;

        bone_joint_id_[num_bones_] = child_node_id;

        num_bones_++;

        child.name = node.name;

        glm::mat4 global_matrix_child = global_matrix * GLTFMatToGLM(node);

        node_parent_transform_[child_node_id] = global_matrix;

        node_new_transform_.insert(child_node_id);

        // For animated SRT nodes, keep the local transform as identity
        if (animated_srt_nodes_.count(child_node_id) == 0) {
          std::tie(child.scale, child.rotation, child.translation) = utl::math::Decompose(global_matrix_child);
        }

        node_local_transform_[child_node_id] = {child.scale, child.rotation, child.translation};

        ProcessBone(child_node_id, child);
      }
    }
  }

  void ProcessBone(int joint_id, SBone& bone) {
    auto& joint = gltf_model_.nodes.at(joint_id);

    // Blender specific warning
    if (utl::string::StartsWith(joint.name, "neutral_bone")) {
      NNL_LOG_WARN(
          "a neutral_bone detected: some meshes might not be properly "
          "skinned");
    }

    if (joint.mesh != -1) mesh_nodes_.insert(joint_id);

    for (auto child_node_id : joint.children) {
      tinygltf::Node& node = gltf_model_.nodes.at(child_node_id);

      if (joints_.count(child_node_id) == 0 && animated_srt_nodes_.count(child_node_id) == 0) {
        // Non joint and non animated nodes are re-attached to the current bone
        ProcessAttachment(bone, joint_id, child_node_id, glm::mat4(1.0f));

        continue;
      }

      SBone& child = bone.children.emplace_back();

      joint_bone_id_[child_node_id] = num_bones_;

      bone_joint_id_[num_bones_] = child_node_id;

      num_bones_++;

      child.name = node.name;

      // Keep local transform as identity for animated nodes
      if (animated_srt_nodes_.count(child_node_id) == 0) {
        if (node_new_transform_.count(child_node_id)) {
          std::tie(child.scale, child.rotation, child.translation) =
              utl::math::Decompose(node_global_transform_.at(child_node_id));
        } else {
          std::tie(child.scale, child.rotation, child.translation) = GLTFMatToGLMSRT(node);
        }
      }

      child.inverse = node_inverse_.count(child_node_id) != 0 ? node_inverse_.at(child_node_id) : glm::mat4(1.0f);

      node_local_transform_[child_node_id] = GLTFMatToGLMSRT(node);

      ProcessBone(child_node_id, child);
    }
  }

  void ProcessNode(int node_id, glm::mat4 global_transform) {
    tinygltf::Node& node = gltf_model_.nodes.at(node_id);

    glm::mat4 local_matrix = GLTFMatToGLM(node);

    glm::mat4 global_matrix = global_transform * local_matrix;

    node_parent_transform_[node_id] = global_transform;

    node_global_transform_[node_id] = global_matrix;

    node_local_transform_[node_id] = GLTFMatToGLMSRT(node);

    if (node.extras.Has("mesh_group")) {
      std::size_t mesh_group = static_cast<std::size_t>(node.extras.Get("mesh_group").GetNumberAsInt());

      used_mesh_groups_.insert(mesh_group);
      node_mesh_group_id_[node_id] = mesh_group;
    }

    if (node.light != -1) light_nodes_.insert(node_id);

    if (utl::string::ToLower(node.name).find("positions") != std::string::npos) {
      if (node.children.empty()) NNL_LOG_WARN("node \"" + node.name + "\" has no children");
      position_nodes_.insert(node_id);
    }

    // If armature root or animated node, mark as a root bone
    if (node_inverse_.count(node_id) != 0) {
      // If translation is not 0, make the node a child of another node to properly
      // display it (otherwise translation is ignored by the games)

      if (animated_srt_nodes_.count(node_id) != 0 ||
          !utl::math::IsApproxEqual((glm::vec3)global_matrix[3], glm::vec3(0), 1e-3f)) {
        if (dummy_root_id_ == -1) dummy_root_id_ = CreateDummyRootNode();

        tinygltf::Node& dummy_root = gltf_model_.nodes.at(dummy_root_id_);

        dummy_root.children.push_back(node_id);

        root_joint_nodes_.insert(dummy_root_id_);
      } else {
        root_joint_nodes_.insert(node_id);
      }

      // Skeleton roots should inherit their global glTF parent transforms (in
      // their local transforms)

      node_new_transform_.insert(node_id);

      return;
    }

    if (node.mesh != -1) mesh_nodes_.insert(node_id);

    for (auto child_node_id : node.children) {
      ProcessNode(child_node_id, global_matrix);
    }
  }

  void ProcessLight(int light_node_id) {
    auto& light_node = gltf_model_.nodes.at(light_node_id);

    auto& light = gltf_model_.lights.at(light_node.light);

    SLight& slight = sasset_.lights.emplace_back();

    slight.name = light_node.name;

    slight.SetDirection(node_global_transform_.at(light_node_id));

    auto& color_v = light.color;

    slight.color = utl::color::LinearToSRGB(glm::vec3(color_v[0], color_v[1], color_v[2]));

    slight.extras = GLTFValueToSValue(light_node.extras);
  }

  void ProcessPosition(int position_node_id) {
    auto& positions_node = gltf_model_.nodes.at(position_node_id);

    for (auto child_node_id : positions_node.children) {
      auto& position_node = gltf_model_.nodes.at(child_node_id);

      SPosition& sposition = sasset_.positions.emplace_back();

      sposition.name = position_node.name;

      sposition.id = sasset_.positions.size() - 1;

      if (position_node.extras.Has("id")) {
        sposition.id = position_node.extras.Get("id").GetNumberAsInt();
      }

      std::tie(sposition.scale, sposition.rotation, sposition.translation) =
          utl::math::Decompose(node_global_transform_.at(child_node_id));

      sposition.extras = GLTFValueToSValue(position_node.extras);

      sposition.extras.Erase("id");

      if (sposition.extras.IsEmpty()) sposition.extras = nullptr;
    }
  }

  void ProcessMaterial(std::size_t mat_id) {
    const tinygltf::Material& mat = gltf_model_.materials.at(mat_id);

    SMaterial smat;

    smat.name = mat.name;

    smat.extras = GLTFValueToSValue(mat.extras);

    smat.extras.Erase("blend");
    smat.extras.Erase("projection");
    smat.extras.Erase("vfx_group");

    if (smat.extras.IsEmpty()) smat.extras = nullptr;

    if (mat.alphaMode == "OPAQUE") smat.alpha_mode = SBlendMode::kOpaque;

    if (mat.alphaMode == "MASK") smat.alpha_mode = SBlendMode::kClip;

    if (mat.alphaMode == "BLEND") smat.alpha_mode = SBlendMode::kAlpha;

    if (mat.extras.Has("blend")) {
      const tinygltf::Value& extra_blend = mat.extras.Get("blend");

      auto blend_mode = extra_blend.Get<std::string>();

      if (blend_mode == "add") {
        smat.alpha_mode &= (~SBlendMode::kClip);
        smat.alpha_mode |= SBlendMode::kAdd;
      }

      if (blend_mode == "sub") {
        smat.alpha_mode &= (~SBlendMode::kClip);
        smat.alpha_mode |= SBlendMode::kSub;
      }
    }

    //        if(mat.extensions.count("OMI_materials_blend"))
    //        {
    //            auto& blend_ext = mat.extensions.at("OMI_materials_blend");

    //            if(blend_ext.Has("alphaMode"))
    //            {

    //                auto alpha_mode =
    //                blend_ext.Get("alphaMode").Get<std::string>();

    //                if(alpha_mode == "BLEND" || alpha_mode == "HASH")
    //                    smat.alpha_mode = SAlphaMode::kBlend;
    //                else if(alpha_mode == "ADD")
    //                    smat.alpha_mode = SAlphaMode::kAdd;
    //                else if(alpha_mode == "SUBTRACT")
    //                    smat.alpha_mode = SAlphaMode::kSub;
    //                else
    //                {
    //                    smat.alpha_mode = SAlphaMode::kBlend;
    //                    NNL_LOG_WARN("material " + smat.name + ": blend mode
    //                    is not supported: " + alpha_mode);
    //                }
    //            }
    //        }

    if (mat.extras.Has("projection")) {
      const tinygltf::Value& extra_proj = mat.extras.Get("projection");

      auto proj_mode = extra_proj.Get<std::string>();

      if (proj_mode == "mat") smat.projection_mode = STextureProjection::kMatrix;

      if (proj_mode == "env") smat.projection_mode = STextureProjection::kEnvironment;
    }

    if (mat.extras.Has("vfx_group")) {
      smat.vfx_group_id = static_cast<u16>(mat.extras.Get("vfx_group").GetNumberAsInt());
    }

    if (mat.extensions.count("KHR_materials_unlit")) {
      smat.lit = false;

      auto& amb_v = mat.pbrMetallicRoughness.baseColorFactor;

      smat.ambient = glm::clamp(utl::color::LinearToSRGB(glm::vec3(amb_v[0], amb_v[1], amb_v[2])), 0.0f, 1.0f);

      smat.opacity = glm::clamp(amb_v[3], 0.0, 1.0);

    } else {
      smat.lit = true;

      smat.specular = glm::vec3(1.0f);

      if (mat.extensions.count("KHR_materials_specular")) {
        auto& specular_ext = mat.extensions.at("KHR_materials_specular");

        if (specular_ext.Has("specularColorFactor")) {
          auto specular = specular_ext.Get("specularColorFactor");

          smat.specular = utl::color::LinearToSRGB(glm::vec3(specular.Get(0).GetNumberAsDouble(),
                                                             specular.Get(1).GetNumberAsDouble(),
                                                             specular.Get(2).GetNumberAsDouble()));
        }

        if (specular_ext.Has("specularFactor")) {
          auto specular = specular_ext.Get("specularFactor");

          smat.specular *= (float)specular.GetNumberAsDouble();
        }

        smat.specular = glm::clamp(smat.specular, 0.0f, 1.0f);
      }

      double spec_intensity = smat.specular.x * 0.2125 + smat.specular.y * 0.7154 + smat.specular.z * 0.0721;

      if (spec_intensity > 0.0f && mat.pbrMetallicRoughness.roughnessFactor < 0.9f)
        smat.specular_power =
            ((2.0 / std::pow(std::max(mat.pbrMetallicRoughness.roughnessFactor, 0.031234752), 2.0)) - 2.0) /
            spec_intensity;
      else
        smat.specular = glm::vec3(0.0f);

      auto& dif_v = mat.pbrMetallicRoughness.baseColorFactor;

      smat.diffuse = glm::clamp(utl::color::LinearToSRGB(glm::vec3(dif_v[0], dif_v[1], dif_v[2])), 0.0f, 1.0f);

      smat.opacity = glm::clamp(dif_v[3], 0.0, 1.0);

      smat.ambient = smat.diffuse;

      if (mat.extensions.count("KHR_materials_sheen")) {
        auto& sheen_ext = mat.extensions.at("KHR_materials_sheen");

        if (sheen_ext.Has("sheenColorFactor")) {
          auto sheen = sheen_ext.Get("sheenColorFactor");

          glm::vec3 ambient = utl::color::LinearToSRGB(glm::vec3(
              sheen.Get(0).GetNumberAsDouble(), sheen.Get(1).GetNumberAsDouble(), sheen.Get(2).GetNumberAsDouble()));

          if (!(utl::math::IsApproxEqual(smat.ambient.r, smat.ambient.g, 0.05f) &&
                utl::math::IsApproxEqual(smat.ambient.g, smat.ambient.b, 0.05f))) {
            smat.ambient = ambient;
          }
        }

        smat.ambient = glm::clamp(smat.ambient, 0.0f, 1.0f);
      }

      auto& emi_v = mat.emissiveFactor;

      smat.emissive = glm::clamp(utl::color::LinearToSRGB(glm::vec3(emi_v[0], emi_v[1], emi_v[2])), 0.0f, 1.0f);
    }

    auto tex_id = mat.pbrMetallicRoughness.baseColorTexture.index;

    material_texture_transform_[mat_id] = glm::mat3(1.0f);

    glm::mat3& matrix = material_texture_transform_[mat_id];

    if (tex_id != -1) {
      auto& extensions = mat.pbrMetallicRoughness.baseColorTexture.extensions;

      if (extensions.count("KHR_texture_transform")) {
        auto& transform = extensions.at("KHR_texture_transform");

        if (transform.Has("scale")) {
          auto& scale = transform.Get("scale");

          matrix[0][0] = scale.Get(0).GetNumberAsDouble();

          matrix[1][1] = scale.Get(1).GetNumberAsDouble();
        }

        if (transform.Has("rotation")) {
          auto& rotation = transform.Get("rotation");

          float rad = rotation.GetNumberAsDouble();

          glm::mat3 rotation_mat(1.0f);
          rotation_mat[0][0] = glm::cos(rad);
          rotation_mat[0][1] = -glm::sin(rad);
          rotation_mat[0][2] = 0;

          rotation_mat[1][0] = glm::sin(rad);
          rotation_mat[1][1] = glm::cos(rad);
          rotation_mat[1][2] = 0;

          matrix = rotation_mat * matrix;
        }

        if (transform.Has("offset")) {
          auto& offset = transform.Get("offset");
          glm::mat3 offset_mat(1.0f);
          offset_mat[2][0] = offset.Get(0).GetNumberAsDouble();

          offset_mat[2][1] = offset.Get(1).GetNumberAsDouble();
          matrix = offset_mat * matrix;
        }
      }

      auto& texture = gltf_model_.textures.at(tex_id);

      auto itr_texture = image_texture_id_.find(texture.source);

      if (itr_texture != image_texture_id_.end()) {
        smat.texture_id = itr_texture->second;
      } else {
        smat.texture_id = sasset_.textures.size();

        image_texture_id_[texture.source] = sasset_.textures.size();

        STexture& stexture = sasset_.textures.emplace_back();

        stexture.extras = GLTFValueToSValue(texture.extras);

        auto& texture_data = gltf_model_.images.at(texture.source);

        if (!texture.name.empty()) stexture.name = texture.name;

        if (stexture.name.empty() && !texture_data.name.empty()) stexture.name = texture_data.name;

        if (stexture.name.empty() && !texture_data.uri.empty() && !utl::string::StartsWith(texture_data.uri, "data:"))
          stexture.name = utl::filesys::u8string(utl::filesys::u8path(texture_data.uri).filename());

        if (texture.sampler != -1) {
          auto& texture_sampler = gltf_model_.samplers.at(texture.sampler);

          if (texture_sampler.minFilter != -1) stexture.min_filter = kGltfToSTextureFilter.at(texture_sampler.minFilter);

          if (texture_sampler.magFilter != -1) stexture.mag_filter = kGltfToSTextureFilter.at(texture_sampler.magFilter);
        }

        if (!utl::math::IsPow2(texture_data.width) || !utl::math::IsPow2(texture_data.height)) {
          NNL_LOG_WARN("texture \"" + stexture.name + "\" width or height is not a power of 2: " +
                       std::to_string(texture_data.width) + "x" + std::to_string(texture_data.height));

          glm::mat3 scale(1.0f);
          scale[0][0] = ((float)texture_data.width / (float)utl::math::RoundUpPow2(texture_data.width));
          scale[1][1] = ((float)texture_data.height / (float)utl::math::RoundUpPow2(texture_data.height));
          if (flip_uv_ && !utl::math::IsPow2(texture_data.height)) scale[2][1] = -scale[1][1];
          matrix = scale * matrix;
        }

        if (!decode_images_) {
          stexture.width = 0;

          stexture.height = texture_data.image.size();

          stexture.bitmap.resize((sizeof(SPixel) + texture_data.image.size()) / sizeof(SPixel));

          std::memcpy(stexture.bitmap.data(), texture_data.image.data(), texture_data.image.size());
        } else {
          stexture.width = texture_data.width;

          stexture.height = texture_data.height;

          stexture.bitmap.resize(stexture.width * stexture.height);

          std::memcpy(stexture.bitmap.data(), texture_data.image.data(),
                      sizeof(SPixel) * stexture.width * stexture.height);

          if (flip_uv_) stexture.FlipV();

          stexture.AlignHeight();
          stexture.AlignWidth();
        }
      }
    }

    sasset_.model.materials.push_back(smat);
  }

  void MarkJointNodes() {
    for (auto& skin : gltf_model_.skins) {
      std::vector<glm::mat4> inverse;

      if (skin.inverseBindMatrices != -1)
        inverse = ExtractData<glm::mat4>(skin.inverseBindMatrices);
      else
        inverse.resize(skin.joints.size(), glm::mat4(1.0f));

      for (std::size_t i = 0; i < skin.joints.size(); i++) {
        node_inverse_[skin.joints.at(i)] = inverse.at(i);
        joints_[skin.joints.at(i)] = true;
      }
    }
    // Mark animated nodes:
    for (auto& animation : gltf_model_.animations)
      for (auto& channel : animation.channels) {
        int target_node = channel.target_node;
        std::string target_path = channel.target_path;
        if (target_path == "pointer" && channel.target_extensions.count("KHR_animation_pointer")) {
          std::string pointer_path =
              channel.target_extensions.at("KHR_animation_pointer").Get("pointer").Get<std::string>();

          if (utl::string::StartsWith(pointer_path, "/nodes/")) {
            std::size_t pos;
            target_node = std::stoul(pointer_path.substr(7), &pos);
            target_path = pointer_path.substr(8 + pos);
          }
        }

        if (target_path != "scale" && target_path != "rotation" && target_path != "translation") continue;

        if (target_node != -1 && node_inverse_.count(target_node) == 0) {
          node_inverse_[target_node] = glm::mat4(1.0f);
          animated_srt_nodes_[target_node] = true;
        }
      }
  }

  void ProcessAnimation(std::size_t animation_id) {
    const tinygltf::Animation& animation = gltf_model_.animations.at(animation_id);

    const auto InsertDefaultAnim = [&]() -> std::size_t {
      std::size_t id = sasset_.animations.size();

      SAnimation& sanimation = sasset_.animations.emplace_back();

      sanimation.name = animation.name;

      sanimation.duration = 0;

      sanimation.bone_channels.resize(num_bones_);
      return id;
    };

    float flip_v = flip_uv_ ? -1.0f : 1.0f;

    std::size_t sanimation_id = -1;

    std::size_t suv_animation_id = -1;

    // Note: a single glTF animation may be transformed into 3 different animation types
    for (auto& channel : animation.channels) {
      std::string target_path = channel.target_path;

      int target_node = channel.target_node;

      std::vector<float> key_frames;

      auto& sampler = animation.samplers.at(channel.sampler);

      key_frames = ExtractData<float>(sampler.input);

      if (target_path == "pointer" && channel.target_extensions.count("KHR_animation_pointer")) {
        std::string pointer_path =
            channel.target_extensions.at("KHR_animation_pointer").Get("pointer").Get<std::string>();

        if (!pointer_path.empty() && pointer_path.back() == '/') pointer_path.pop_back();

        if (utl::string::StartsWith(pointer_path, "/materials/") &&
            utl::string::EndsWith(pointer_path,
                                  "/pbrMetallicRoughness/baseColorTexture/extensions/"
                                  "KHR_texture_transform/offset")) {
          // A UV Animation:
          std::size_t mat_id = std::stoul(pointer_path.substr(11));

          if (suv_animation_id == static_cast<std::size_t>(-1)) {
            suv_animation_id = sasset_.model.uv_animations.size();
            auto& new_suv_anim = sasset_.model.uv_animations.emplace_back();
            new_suv_anim.name = animation.name;
          }

          SUVAnimation& suv_animation = sasset_.model.uv_animations.at(suv_animation_id);

          suv_animation.extras = GLTFValueToSValue(animation.extras);

          auto& [interpolation, schannel] = suv_animation.translation_channels[mat_id];

          interpolation = sampler.interpolation == "STEP" ? SInterpolationMode::kConstant : SInterpolationMode::kLinear;

          schannel.reserve(key_frames.size());

          auto values = ExtractData<Vec2<float>>(sampler.output);

          for (std::size_t i = 0; i < key_frames.size(); i++) {
            u16 key_frame = static_cast<u16>(std::round(key_frames[i] * 30.0f));

            if (schannel.empty() || schannel.back().time < key_frame)
              schannel.push_back({key_frame, glm::vec2(values[i].x, values[i].y * flip_v)});
          }
          continue;
        } else if (utl::string::StartsWith(pointer_path, "/nodes/")) {
          std::size_t pos;
          target_node = std::stoul(pointer_path.substr(7), &pos);
          target_path = pointer_path.substr(7 + pos + 1);

          if (target_node >= 0 && utl::string::EndsWith(pointer_path, "KHR_node_visibility/visible")) {
            // A Visibility Animation:
            target_path = "visible";
            if (node_mesh_group_id_.count(target_node) == 0) {
              // If an animated node is not associated with a particular mesh group, assign it automatically
              const std::set<std::size_t> all_groups{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

              std::set<std::size_t> unused_groups;

              std::set_difference(all_groups.begin(), all_groups.end(), used_mesh_groups_.begin(),
                                  used_mesh_groups_.end(), std::inserter(unused_groups, unused_groups.begin()));

              if (!unused_groups.empty()) {
                const std::size_t new_group = *unused_groups.begin();
                node_mesh_group_id_[target_node] = new_group;
                used_mesh_groups_.insert(new_group);

                if (mesh_node_smesh_id_.count(target_node)) {
                  auto& smesh_ids = mesh_node_smesh_id_.at(target_node);
                  for (std::size_t smesh_id : smesh_ids) {
                    sasset_.model.meshes.at(smesh_id).mesh_group = new_group;
                  }
                }
                NNL_LOG_WARN(
                    "visibility animation had no associated mesh group and was "
                    "assigned to group " +
                    std::to_string(new_group));
              } else {
                NNL_LOG_WARN(
                    "visibility animation has no associated mesh group and all "
                    "groups are already in use: " +
                    animation.name);
                continue;
              }
            }

            std::size_t mesh_group = node_mesh_group_id_.at(target_node);

            if (mesh_group > 15) {
              NNL_LOG_WARN("the mesh group is > 15 and can't be animated: " + std::to_string(mesh_group));
              continue;
            }

            if (mesh_group == 0) {
              NNL_LOG_WARN("the mesh group 0 can't be animated");
              continue;
            }

            if (sanimation_id == static_cast<std::size_t>(-1)) {
              sanimation_id = InsertDefaultAnim();
            }

            if (sasset_.visibility_animations.size() <= sanimation_id)
              sasset_.visibility_animations.resize(sanimation_id + 1);

            auto& sanimation = sasset_.visibility_animations.at(sanimation_id);

            auto& schannel = sanimation.visibility_channels.at(mesh_group);

            if (schannel.empty()) {
              schannel.reserve(key_frames.size());
              auto values = ExtractData<u8>(sampler.output);

              for (std::size_t i = 0; i < key_frames.size(); i++) {
                u16 key_frame = static_cast<u16>(std::round(key_frames[i] * 30.0f));

                if (schannel.empty() || schannel.back().time < key_frame)
                  schannel.push_back({key_frame, static_cast<bool>(values[i])});
              }
            } else {
              NNL_LOG_WARN("multiple meshes animate visibility of the mesh group " + std::to_string(mesh_group) +
                           " - join them or delete one of the animations");
            }
          }

        } else {
          // pointer path not recognized, skip
          continue;
        }
      }

      // A Skeletal SRT Animation:
      if ((target_path != "scale" && target_path != "rotation" && target_path != "translation" &&
           target_path != "visible") ||
          target_node < 0)
        continue;

      if (sanimation_id == static_cast<std::size_t>(-1)) {
        sanimation_id = InsertDefaultAnim();
      }

      SAnimation& sanimation = sasset_.animations.at(sanimation_id);

      sanimation.extras = GLTFValueToSValue(animation.extras);

      auto& k_accessor = gltf_model_.accessors.at(sampler.input);

      auto max_keyframe = static_cast<u16>(std::round(k_accessor.maxValues.at(0) * 30.0f));

      if (max_keyframe > sanimation.duration) sanimation.duration = max_keyframe;

      // Set the duration of the main SRT animation before skipping.
      if (target_path == "visible") continue;

      SBoneChannel& schannel = sanimation.bone_channels.at(joint_bone_id_.at(target_node));

      if (target_path == "translation") {
        schannel.translation_keys.reserve(key_frames.size());
        auto values = ExtractData<Vec3<float>>(sampler.output);

        for (std::size_t i = 0; i < key_frames.size(); i++) {
          u16 key_frame = static_cast<u16>(std::round(key_frames[i] * 30.0f));
          glm::vec3 value{values[i].x, values[i].y, values[i].z};

          if (schannel.translation_keys.empty() || schannel.translation_keys.back().time < key_frame)
            schannel.translation_keys.push_back({key_frame, value});
        }
      }

      if (target_path == "scale") {
        schannel.scale_keys.reserve(key_frames.size());
        auto values = ExtractData<Vec3<float>>(sampler.output);

        for (std::size_t i = 0; i < key_frames.size(); i++) {
          u16 key_frame = static_cast<u16>(std::round(key_frames[i] * 30.0f));
          glm::vec3 value{values[i].x, values[i].y, values[i].z};

          if (schannel.scale_keys.empty() || schannel.scale_keys.back().time < key_frame)
            schannel.scale_keys.push_back({key_frame, value});
        }
      }

      if (target_path == "rotation") {
        schannel.rotation_keys.reserve(key_frames.size());
        std::vector<Vec4<float>> values;

        auto& rot_accessor = gltf_model_.accessors.at(sampler.output);

        switch (rot_accessor.componentType) {
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            values = NormalizeToFloat<Vec4<float>>(ExtractData<Vec4<u8>>(sampler.output));
            break;
          case TINYGLTF_COMPONENT_TYPE_BYTE:
            values = NormalizeToFloat<Vec4<float>>(ExtractData<Vec4<i8>>(sampler.output));
            break;
          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            values = NormalizeToFloat<Vec4<float>>(ExtractData<Vec4<u16>>(sampler.output));
            break;
          case TINYGLTF_COMPONENT_TYPE_SHORT:
            values = NormalizeToFloat<Vec4<float>>(ExtractData<Vec4<i16>>(sampler.output));
            break;
          case TINYGLTF_COMPONENT_TYPE_FLOAT:
            values = ExtractData<Vec4<float>>(sampler.output);
            break;
          default:
            NNL_THROW(ParseError(NNL_ERMSG("invalid rotation type")));
        }

        for (std::size_t i = 0; i < key_frames.size(); i++) {
          u16 key_frame = static_cast<u16>(std::round(key_frames[i] * 30.0f));
          glm::quat value{values[i].w, values[i].x, values[i].y, values[i].z};

          if (schannel.rotation_keys.empty() || schannel.rotation_keys.back().time < key_frame)
            schannel.rotation_keys.push_back({key_frame, value});
        }
      }
    }

    // Insert at least one default keyframe for each channel if it has none

    if (sanimation_id != static_cast<std::size_t>(-1)) {
      SAnimation& sanimation = sasset_.animations.at(sanimation_id);

      sanimation.duration++;

      for (std::size_t i = 0; i < sanimation.bone_channels.size(); i++) {
        auto& schannel = sanimation.bone_channels.at(i);

        auto [scale, rotation, translation] = node_local_transform_.at(bone_joint_id_.at(i));

        // Get the local bone transform
        if (schannel.scale_keys.empty()) {
          schannel.scale_keys.push_back({0, scale});
        }

        if (schannel.rotation_keys.empty()) {
          schannel.rotation_keys.push_back({0, rotation});
        }

        if (schannel.translation_keys.empty()) {
          schannel.translation_keys.push_back({0, translation});
        }
      }

      sanimation.Bake();
      // Make sure animation inherits parent transforms of non animated nodes
      for (std::size_t i = 0; i < sanimation.bone_channels.size(); i++) {
        auto& schannel = sanimation.bone_channels.at(i);
        if (node_new_transform_.count(bone_joint_id_.at(i))) {
          auto parent_transform = node_parent_transform_.at(bone_joint_id_.at(i));

          if (utl::math::IsApproxEqual(parent_transform, glm::mat4(1.0f))) continue;

          for (std::size_t t = 0; t < sanimation.duration; t++) {
            auto& scale_key = schannel.scale_keys.at(t);
            auto& rotation_key = schannel.rotation_keys.at(t);
            auto& translation_key = schannel.translation_keys.at(t);

            auto global_matrix =
                parent_transform * utl::math::Compose(scale_key.value, rotation_key.value, translation_key.value);

            auto [scale, rotation, translation] = utl::math::Decompose(global_matrix);

            scale_key.value = scale;
            rotation_key.value = rotation;
            translation_key.value = translation;
          }
        }
      }
    }
  }

  void ProcessMaterialVariants() {
    if (gltf_model_.extensions.count("KHR_materials_variants")) {
      auto& khr_material_variants = gltf_model_.extensions.at("KHR_materials_variants");

      auto& variants = khr_material_variants.Get("variants");

      for (std::size_t i = 0; i < variants.Size(); i++) {
        auto& variant = variants.Get(i);

        std::string name = variant.Get("name").Get<std::string>();

        unsorted_variants_.push_back(name);

        sorted_variants_.insert(name);
      }

      sasset_.model.material_variants.resize(sorted_variants_.size());

      std::copy(sorted_variants_.begin(), sorted_variants_.end(), sasset_.model.material_variants.begin());
    }
  }

  void Import() {
    sasset_.name = filename_;

    bool success = false;

    gltf_ctx_.SetStoreOriginalJSONForExtrasAndExtensions(true);
    gltf_ctx_.SetImagesAsIs(!decode_images_);

    // Import from a file or a memory buffer.
    if (!path_.empty() && buffer_.data() == nullptr) {
      std::string file_ext = utl::string::ToLower(utl::filesys::u8string(path_.extension()));
      if (file_ext == ".gltf") {
        success = gltf_ctx_.LoadASCIIFromFile(&gltf_model_, &error_, &warn_, utl::filesys::u8string(path_));
      } else {
        success = gltf_ctx_.LoadBinaryFromFile(&gltf_model_, &error_, &warn_, utl::filesys::u8string(path_));
      }
    } else if (buffer_.data() != nullptr) {
      buffer_.Seek(0);

      constexpr u32 kMagicBytes = utl::data::FourCC("glTF");

      if (buffer_.Len() > 0x10 && buffer_.ReadLE<u32>() == kMagicBytes) {
        success = gltf_ctx_.LoadBinaryFromMemory(&gltf_model_, &error_, &warn_, buffer_.data(), buffer_.size(),
                                                 utl::filesys::u8string(path_));
      } else {
        success =
            gltf_ctx_.LoadASCIIFromString(&gltf_model_, &error_, &warn_, reinterpret_cast<const char*>(buffer_.data()),
                                          buffer_.size(), utl::filesys::u8string(path_));
      }
    }

    if (!warn_.empty()) NNL_LOG_WARN(warn_);

    if (!success) NNL_THROW(ParseError(NNL_ERMSG("importing failed: " + error_ + " " + utl::filesys::u8string(path_))));

    for (auto& extension_required : gltf_model_.extensionsRequired) {
      if (extensions_supported_.count(extension_required) == 0)
        NNL_THROW(ParseError(NNL_ERMSG("a required extension is not supported: " + extension_required + "; " +
                                       utl::filesys::u8string(path_))));
    }

    if (gltf_model_.scenes.empty()) return;

    auto& main_scene = gltf_model_.scenes.at(0);

    sasset_.extras = GLTFValueToSValue(main_scene.extras);

    ProcessMaterialVariants();

    MarkJointNodes();

    tinygltf::Node& scene_root = gltf_model_.nodes.emplace_back();

    scene_root.children = main_scene.nodes;

    ProcessNode(gltf_model_.nodes.size() - 1, glm::mat4(1.0f));

    for (auto root_id : root_joint_nodes_) {
      auto& node = gltf_model_.nodes.at(root_id);

      SBone& root = sasset_.model.skeleton.roots.emplace_back();

      joint_bone_id_[root_id] = num_bones_;

      bone_joint_id_[num_bones_] = root_id;

      num_bones_++;

      root.name = node.name;
      // Keep local transforms as identity for animated nodes.
      // For armature root nodes, ensure they inherit global transform.
      if (animated_srt_nodes_.count(root_id) == 0) {
        std::tie(root.scale, root.rotation, root.translation) = utl::math::Decompose(node_global_transform_.at(root_id));
      }

      node_local_transform_[root_id] = {root.scale, root.rotation, root.translation};

      root.inverse = node_inverse_.count(root_id) != 0 ? node_inverse_.at(root_id) : glm::mat4(1.0f);

      ProcessBone(root_id, root);
    }

    if (root_joint_nodes_.empty()) {
      CreateDummyRootBone();
    }

    for (std::size_t i = 0; i < gltf_model_.materials.size(); i++) {
      ProcessMaterial(i);
    }

    for (auto mesh_node_id : mesh_nodes_) {
      ProcessMesh(mesh_node_id);
    }

    std::sort(sasset_.model.meshes.begin(), sasset_.model.meshes.end(),
              [](const auto& a, const auto& b) { return utl::string::CompareNat(a.name, b.name); });

    // animations may be split later, pre-sort to ensure the order
    std::sort(gltf_model_.animations.begin(), gltf_model_.animations.end(),
              [](const auto& a, const auto& b) { return utl::string::CompareNat(a.name, b.name); });

    for (std::size_t i = 0; i < gltf_model_.animations.size(); i++) {
      ProcessAnimation(i);
    }

    if (!sasset_.visibility_animations.empty()) sasset_.visibility_animations.resize(sasset_.animations.size());

    for (auto light_node_id : light_nodes_) {
      ProcessLight(light_node_id);
    }

    for (auto position_node_id : position_nodes_) {
      ProcessPosition(position_node_id);
    }

    std::sort(sasset_.positions.begin(), sasset_.positions.end(),
              [](const auto& a, const auto& b) { return utl::string::CompareNat(a.name, b.name); });
  }
};

SAsset3D SAsset3D::Import(const std::filesystem::path& path, bool flip, bool decode_images) {
  SAsset3D sasset;
  GLTFImporter gltf_importer(sasset, path, flip, decode_images);

  return sasset;
}

SAsset3D SAsset3D::Import(BufferView buffer, const std::filesystem::path& base_path, bool flip, bool decode_images) {
  SAsset3D sasset;

  GLTFImporter gltf_importer(sasset, buffer, base_path, flip, decode_images);

  return sasset;
}

}  // namespace nnl
