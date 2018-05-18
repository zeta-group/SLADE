#pragma once

#define EDF_FALSE 0
#define EDF_UNLIKELY 64
#define EDF_MAYBE 128
#define EDF_PROBABLY 192
#define EDF_TRUE 255

class EntryDataFormat
{
public:
	EntryDataFormat(string id);
	virtual ~EntryDataFormat() = default;

	const string& id() const { return id_; }

	virtual int isThisFormat(MemChunk& mc);

	static void             initBuiltinFormats();
	static EntryDataFormat* getFormat(string_view id);
	static EntryDataFormat* anyFormat();
	static EntryDataFormat* textFormat();

private:
	string   id_;
	unsigned size_min_;
};
