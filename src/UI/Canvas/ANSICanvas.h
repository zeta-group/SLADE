#pragma once

#include "General/ListenerAnnouncer.h"
#include "OGLCanvas.h"
#include "OpenGL/GLTexture.h"

class ANSICanvas : public OGLCanvas, Listener
{
public:
	ANSICanvas(wxWindow* parent, int id);
	~ANSICanvas() = default;

	void draw();
	void drawImage();
	void writeRGBAData(uint8_t* dest);
	void loadData(uint8_t* data) { ansidata_ = data; }
	void drawCharacter(size_t index);

private:
	size_t          width_, height_;
	vector<uint8_t> picdata_;
	const uint8_t*  fontdata_;
	uint8_t*        ansidata_;
	GLTexture       tex_image_;
	int             char_width_;
	int             char_height_;
};
