#pragma once

class MemChunk;

namespace FileUtil
{
bool           fileExists(const string& path);
bool           removeFile(const string& path);
bool           dirExists(const string& path);
bool           createDir(string_view path);
vector<string> allFilesInDir(string_view path, bool include_subdirs = false);
bool           copyFile(string_view filename, string_view target_filename);
bool           writeStrToFile(string_view str, string_view filename);
time_t         fileModificationTime(const string& path);
} // namespace FileUtil


class SFile
{
public:
	enum class Mode
	{
		ReadOnly,
		Write,
		ReadWite,
		Append
	};

	SFile() = default;
	SFile(const string& path, Mode mode = Mode::ReadOnly);
	SFile(string_view path, Mode mode = Mode::ReadOnly) : SFile(string{ path.data(), path.size() }, mode) {}
	~SFile() { close(); }

	bool     isOpen() const { return handle_ != nullptr; }
	unsigned currentPos() const;
	unsigned length() const { return handle_ ? stat_.st_size : 0; }
	unsigned size() const { return handle_ ? stat_.st_size : 0; }

	bool open(const string& path, Mode mode = Mode::ReadOnly);
	bool open(string_view path, Mode mode = Mode::ReadOnly) { return open(string{ path.data(), path.size() }, mode); }
	void close();

	bool seek(unsigned offset) const;
	bool seekFromStart(unsigned offset) const;
	bool seekFromEnd(unsigned offset) const;

	bool read(void* buffer, unsigned count) const;
	bool read(MemChunk& mc, unsigned count) const;
	bool read(string& str, unsigned count) const;

	bool write(const void* buffer, unsigned count) const;
	bool writeStr(string_view str) const;

	template<typename T> bool read(T& var) { return read(&var, sizeof(T)); }
	template<typename T> bool write(T& var) { return write(&var, sizeof(var)); }

	template<typename T> T get()
	{
		T var = T{};
		read<T>(var);
		return var;
	}

private:
	FILE*       handle_ = nullptr;
	struct stat stat_;
};
