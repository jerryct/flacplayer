// SPDX-License-Identifier: MIT

#include "bit_cast.h"
#include "stream.cpp"
#include <fmt/format.h>
#include <gtest/gtest.h>

namespace plac {
namespace {

TEST(StreamTest, TagCompare_WhenEqual) {
  EXPECT_EQ(0, tagcompare("abc", "abc", 3));
  EXPECT_EQ(0, tagcompare("abcd", "abce", 3));
  EXPECT_EQ(0, tagcompare("abc", "ABC", 3));
  EXPECT_EQ(0, tagcompare("abcd", "ABCe", 3));
  EXPECT_EQ(0, tagcompare("ABC", "abc", 3));
  EXPECT_EQ(0, tagcompare("ABCe", "abcd", 3));
}

TEST(StreamTest, TagCompare_WhenNotEqual) {
  EXPECT_NE(0, tagcompare("ebc", "abc", 3));
  EXPECT_NE(0, tagcompare("abe", "abc", 3));
  EXPECT_NE(0, tagcompare("ebc", "ABC", 3));
  EXPECT_NE(0, tagcompare("abe", "ABC", 3));
  EXPECT_NE(0, tagcompare("ABC", "ebc", 3));
  EXPECT_NE(0, tagcompare("ABC", "abe", 3));
}

TEST(StreamTest, VorbisComment_WhenFound) {
  FLAC__StreamMetadata_VorbisComment vc{};
  FLAC__StreamMetadata_VorbisComment_Entry entries[3];
  vc.num_comments = 3;
  vc.comments = entries;
  const char *tag0{"TAG1=foo"};
  entries[0].entry = BitCast<FLAC__byte *>(tag0);
  entries[0].length = fmt::string_view{tag0}.size();
  const char *tag1{"TAG2=bar"};
  entries[1].entry = BitCast<FLAC__byte *>(tag1);
  entries[1].length = fmt::string_view{tag1}.size();
  const char *tag2{"TAG1=xyz"};
  entries[2].entry = BitCast<FLAC__byte *>(tag2);
  entries[2].length = fmt::string_view{tag2}.size();

  EXPECT_EQ(fmt::string_view{"foo"}, fmt::string_view{vorbis_comment_query(vc, "TAG1", 0)});
  EXPECT_EQ(fmt::string_view{"xyz"}, fmt::string_view{vorbis_comment_query(vc, "TAG1", 1)});
  EXPECT_EQ(fmt::string_view{"bar"}, fmt::string_view{vorbis_comment_query(vc, "TAG2", 0)});
}

TEST(StreamTest, VorbisComment_WhenNotFound) {
  FLAC__StreamMetadata_VorbisComment vc{};
  FLAC__StreamMetadata_VorbisComment_Entry entries[1];
  vc.num_comments = 1;
  vc.comments = entries;
  const char *tag0{"TAG1=foo"};
  entries[0].entry = BitCast<FLAC__byte *>(tag0);
  entries[0].length = fmt::string_view{tag0}.size();

  EXPECT_EQ(fmt::string_view{"<none>"}, fmt::string_view{vorbis_comment_query(vc, "TAG1", 1)});
  EXPECT_EQ(fmt::string_view{"<none>"}, fmt::string_view{vorbis_comment_query(vc, "TAG2", 0)});
}

} // namespace
} // namespace plac
