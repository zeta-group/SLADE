#pragma once

class MemChunk;

namespace FileUtil
{
bool           fileExists(const string& path);
bool           removeFile(const string& path);
bool           dirExists(const string& path);
bool           createDir(string_view path);
vector<string> allFilesInDir(string_view path, bool include_subdirs = false);

} // namespace FileUtil
