#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <functional>

using download_cb = std::function<void(void* param, size_t total_len, size_t offset)>;
int download_file(const char *file_name, const char *url, download_cb cb, std::string &md5);