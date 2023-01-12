/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstring>
#include <fstream>
#include <pw_assert/assert.h>
#include <pw_tokenizer/detokenize.h>
#include <pw_tokenizer/tokenize.h>
#include <string_view>
#include <vector>
#include <zephyr/kernel.h>

static std::vector<uint8_t> ReadWholeFile(const char *path) {
  // Open the file
  std::ifstream file(path, std::ios::binary);

  // Stop eating new lines in binary mode
  file.unsetf(std::ios::skipws);

  // Get the file size
  std::streampos file_size;

  file.seekg(0, std::ios::end);
  file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  // Reserve capacity
  std::vector<uint8_t> data;
  data.reserve(file_size);

  // Read the data
  data.insert(data.begin(), std::istream_iterator<uint8_t>(file),
              std::istream_iterator<uint8_t>());

  return data;
}

pw::tokenizer::Detokenizer OpenDatabase(const char *path) {
  std::vector<uint8_t> data = ReadWholeFile(path);

  pw::tokenizer::TokenDatabase database =
      pw::tokenizer::TokenDatabase::Create(data);

  // This checks if the file contained a valid database.
  PW_ASSERT(database.ok());
  return pw::tokenizer::Detokenizer(database);
}

constexpr uint32_t kHelloWorldToken = PW_TOKENIZE_STRING("Hello tokenized world!");

int main(void) {
  char expected_string[1024];
  uint8_t buffer[1024];
  size_t size_bytes = sizeof(buffer);

  pw::tokenizer::Detokenizer detokenizer = OpenDatabase("database.bin");

  sprintf(expected_string, "token=%u\n", kHelloWorldToken);
  printk(expected_string);
  PW_TOKENIZE_TO_BUFFER(buffer, &size_bytes, "token=%u\n", kHelloWorldToken);

  printk("tokenized buffer size is %u bytes\n", size_bytes);

  auto detokenized_string = detokenizer.Detokenize(buffer, size_bytes);
  PW_ASSERT(detokenized_string.ok());
  PW_ASSERT(strcmp(detokenized_string.BestString().c_str(), expected_string) ==
            0);
  printk("detokenized message size is %u bytes\n",
         strlen(detokenized_string.BestString().c_str()));
  return 0;
}
