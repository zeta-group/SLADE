#pragma once

namespace Audio
{
wxString getID3Tag(MemChunk& mc);
wxString getOggComments(MemChunk& mc);
wxString getFlacComments(MemChunk& mc);
wxString getITComments(MemChunk& mc);
wxString getModComments(MemChunk& mc);
wxString getS3MComments(MemChunk& mc);
wxString getXMComments(MemChunk& mc);
wxString getWavInfo(MemChunk& mc);
wxString getVocInfo(MemChunk& mc);
wxString getSunInfo(MemChunk& mc);
wxString getRmidInfo(MemChunk& mc);
wxString getAiffInfo(MemChunk& mc);
size_t checkForTags(MemChunk& mc);

// WAV format values
// A more complete list can be found in mmreg.h,
// under the "WAVE form wFormatTag IDs" comment.
// There are dozens upon dozens of them, most of
// which are not usually seen in practice.
enum WavFormat : int
{
	Unknown = 0x0000,
	PCM = 0x0001,
	ADPCM = 0x0002,
	ALAW = 0x0006,
	MULAW = 0x0007,
	MP3 = 0x0055,
	XTNSBL = 0xFFFE,
};
int detectRiffWavFormat(MemChunk& mc);

} // namespace Audio
