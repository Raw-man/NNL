#include "NNL/simple_asset/svalue.hpp"

#include <gtest/gtest.h>

using namespace nnl;

TEST(SValue, Construct) {
  SValue s("Hello, World!");

  ASSERT_TRUE(s.IsString());

  const std::string str = "Hello, World!";

  SValue s2(str);

  ASSERT_TRUE(s2.IsString());

  std::string_view str_view = str;

  SValue s3(str_view);

  ASSERT_TRUE(s3.IsString());

  SValue s4(42u);

  ASSERT_TRUE(s4.IsInt());

  SValue s5(42.0f);

  ASSERT_TRUE(s5.IsDouble());

  SValue s6(std::move(str));

  ASSERT_TRUE(s6.IsString());

  SValue s7(false);

  ASSERT_TRUE(s7.IsBool());

  int i = 42;

  const int& ir = i;

  s7 = ir;

  SValue s8(ir);

  ASSERT_TRUE(s8.IsInt());

  const int i2 = 43;

  const int& i2_ref = i2;

  SValue s9 = std::move(i2_ref);

  ASSERT_TRUE(s9.IsInt());

  const std::string& str_ref = str;

  SValue s10(str_ref);

  ASSERT_TRUE(s10.IsString());

  std::string str_m = "Move out";

  SValue s11(std::move(str_m));

  ASSERT_TRUE(s11.IsString());
  ASSERT_TRUE(str_m.empty());
}

TEST(SValue, Assign) {
  SValue s;

  ASSERT_TRUE(s.IsNull());

  s = nullptr;

  ASSERT_TRUE(s.IsNull());

  s = -3;

  ASSERT_TRUE(s.IsInt());

  s = 3.5;

  ASSERT_TRUE(s.IsDouble());

  s = 3.5f;

  ASSERT_TRUE(s.IsDouble());

  s = true;

  ASSERT_TRUE(s.IsBool());

  s = false;

  ASSERT_TRUE(s.IsBool());

  s = "Hello, World!";

  ASSERT_TRUE(s.IsString());

  std::string str = "The quick brown fox jumps over the lazy dog.";
  std::string_view str_view = str;
  s = str;

  ASSERT_TRUE(s.IsString());

  s = str_view;

  ASSERT_TRUE(s.IsString());

  s = SValue::Array();

  ASSERT_TRUE(s.IsArray());

  s = SValue::Object();

  ASSERT_TRUE(s.IsObject());

  s = {0, 1, 2};

  ASSERT_TRUE(s.IsArray());

  s = {{"key", "value"}, {"key2", "value2"}};

  ASSERT_TRUE(s.IsObject());
}

TEST(SValue, Extract) {
  SValue extras = {{"id", 42},
                   {"ctr", 0.5},
                   {"tags", {"admin", "editor"}},
                   {"settings", {{"theme", "dark"}, {"notifications", true}}}};

  ASSERT_TRUE(extras.IsObject());

  std::size_t id = extras["id"];

  float ctr = extras.At("ctr");

  ASSERT_TRUE(id == 42);

  ASSERT_TRUE(ctr > 0.499 && ctr < 0.5001);

  ASSERT_TRUE(extras.Has("tags") && extras.At("tags").IsArray());

  std::string tag = extras["tags"][0];

  ASSERT_TRUE(tag == "admin");

  ASSERT_TRUE(extras.Has("settings") && extras.At("settings").IsObject());

  std::string theme = extras["settings"]["theme"];

  ASSERT_TRUE(theme == "dark");

  bool enabled = extras["settings"]["notifications"];

  ASSERT_TRUE(enabled);
}

TEST(SValue, Mutate) {
  SValue s = nullptr;
  s.Push("Hello, World!");
  s.Push(42);

  ASSERT_TRUE(s.IsArray());
  ASSERT_TRUE(s.Size() == 2);

  ASSERT_TRUE(s[0].IsString());
  ASSERT_TRUE(s[1].IsInt());

  s = nullptr;

  ASSERT_TRUE(s.Size() == 0);
  ASSERT_TRUE(s.IsNull());

  s["key"] = 0x45564F4C;
  s["answer"] = 0x2A;

  auto [val, inserted] = s.Insert("key", "old_value_must_stay");

  ASSERT_TRUE(val.IsInt());
  ASSERT_FALSE(inserted);

  ASSERT_TRUE(s.IsObject());
  ASSERT_TRUE(s.Size() == 2);
  ASSERT_TRUE(s["key"].IsInt());
  ASSERT_TRUE(s["answer"].IsInt());

  s["key"] = 42ull;

  ASSERT_TRUE(s.At("key").IsInt());
}

TEST(SValue, Coercion) {
  SValue v;

  v = 42;

  std::string str = v;

  v = str;

  int rv = v;

  ASSERT_EQ(rv, 42);
}
