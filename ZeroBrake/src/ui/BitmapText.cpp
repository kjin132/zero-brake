// UTF-8 UI text. Windows GDI rasterizes glyphs once into a reusable OpenGL atlas.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "ui/BitmapText.h"
#include <unordered_map>
#include <iostream>

#pragma comment(lib, "gdi32.lib")

namespace
{
constexpr int CELL = 64, ATLAS = 2048, COLUMNS = ATLAS / CELL;
struct Glyph
{
	int width = 0, height = 0, left = 0, top = 0, advance = 0, x = 0, y = 0;
	bool uploaded = false;
	std::vector<unsigned char> pixels;
};
struct TextVertex { float x, y, u, v; };
HDC sDC = nullptr;
HFONT sFont = nullptr;
HGDIOBJ sOldFont = nullptr;
TEXTMETRICW sMetrics = {};
std::unordered_map<wchar_t, Glyph> sGlyphs;
GLuint sProgram = 0, sAtlas = 0, sVAO = 0, sVBO = 0;
GLint sProjection = -1, sColor = -1;

bool InitFont()
{
	if (sDC) return true;
	sDC = CreateCompatibleDC(nullptr);
	if (!sDC) return false;
	sFont = CreateFontW(-48, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Malgun Gothic");
	if (!sFont) { DeleteDC(sDC); sDC = nullptr; return false; }
	sOldFont = SelectObject(sDC, sFont);
	if (!GetTextMetricsW(sDC, &sMetrics))
	{
		SelectObject(sDC, sOldFont); DeleteObject(sFont); DeleteDC(sDC);
		sFont = nullptr; sDC = nullptr; return false;
	}
	return true;
}

std::wstring Decode(const std::string& text)
{
	if (text.empty()) return {};
	int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), (int)text.size(), nullptr, 0);
	if (!count) return L"?";
	std::wstring wide(count, L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), (int)text.size(), &wide[0], count);
	return wide;
}

Glyph& GetGlyph(wchar_t ch)
{
	auto found = sGlyphs.find(ch);
	if (found != sGlyphs.end()) return found->second;
	// Reserve one fallback slot rather than growing GPU memory for changing strings.
	if (sGlyphs.size() >= COLUMNS * COLUMNS - 1 && ch != L'?') return GetGlyph(L'?');
	MAT2 identity = {}; identity.eM11.value = identity.eM22.value = 1;
	GLYPHMETRICS metrics = {};
	DWORD size = GetGlyphOutlineW(sDC, ch, GGO_GRAY8_BITMAP, &metrics, 0, nullptr, &identity);
	if ((size == GDI_ERROR || metrics.gmBlackBoxX > CELL - 2 || metrics.gmBlackBoxY > CELL - 2) && ch != L'?')
		return GetGlyph(L'?');
	Glyph glyph;
	glyph.advance = size == GDI_ERROR ? 24 : metrics.gmCellIncX;
	glyph.left = metrics.gmptGlyphOrigin.x; glyph.top = metrics.gmptGlyphOrigin.y;
	int slot = (int)sGlyphs.size();
	glyph.x = (slot % COLUMNS) * CELL + 1; glyph.y = (slot / COLUMNS) * CELL + 1;
	if (size != GDI_ERROR && size > 0)
	{
		std::vector<unsigned char> bitmap(size);
		if (GetGlyphOutlineW(sDC, ch, GGO_GRAY8_BITMAP, &metrics, size, bitmap.data(), &identity) != GDI_ERROR)
		{
			glyph.width = (int)metrics.gmBlackBoxX; glyph.height = (int)metrics.gmBlackBoxY;
			int stride = (glyph.width + 3) & ~3;
			glyph.pixels.resize(glyph.width * glyph.height);
			for (int y = 0; y < glyph.height; ++y)
				for (int x = 0; x < glyph.width; ++x)
					glyph.pixels[y * glyph.width + x] = (unsigned char)(std::min(64, (int)bitmap[y * stride + x]) * 255 / 64);
		}
	}
	return sGlyphs.emplace(ch, std::move(glyph)).first->second;
}

GLuint Compile(GLenum type, const char* source)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr); glCompileShader(shader);
	GLint ok = 0; glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		char log[1024]; glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		std::cerr << "Text shader: " << log << '\n'; glDeleteShader(shader); return 0;
	}
	return shader;
}
}

bool Text_Init()
{
	if (!InitFont()) { std::cerr << "UI font initialization failed\n"; return false; }
	if (sProgram) return true;
	const char* vertex = R"(#version 330 core
layout(location=0) in vec2 position;
layout(location=1) in vec2 uv;
uniform mat4 projection;
out vec2 texCoord;
void main() { texCoord = uv; gl_Position = projection * vec4(position, 0.0, 1.0); }
)";
	const char* fragment = R"(#version 330 core
in vec2 texCoord;
uniform sampler2D glyphAtlas;
uniform vec4 textColor;
out vec4 color;
void main() { color = vec4(textColor.rgb, textColor.a * texture(glyphAtlas, texCoord).r); }
)";
	GLuint vs = Compile(GL_VERTEX_SHADER, vertex), fs = Compile(GL_FRAGMENT_SHADER, fragment);
	if (!vs || !fs) { if (vs) glDeleteShader(vs); if (fs) glDeleteShader(fs); return false; }
	sProgram = glCreateProgram(); glAttachShader(sProgram, vs); glAttachShader(sProgram, fs);
	glLinkProgram(sProgram); glDeleteShader(vs); glDeleteShader(fs);
	GLint ok = 0; glGetProgramiv(sProgram, GL_LINK_STATUS, &ok);
	if (!ok) { glDeleteProgram(sProgram); sProgram = 0; return false; }
	sProjection = glGetUniformLocation(sProgram, "projection"); sColor = glGetUniformLocation(sProgram, "textColor");
	GLint oldVAO, oldBuffer, oldTexture;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVAO); glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldBuffer);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
	glGenVertexArrays(1, &sVAO); glBindVertexArray(sVAO);
	glGenBuffers(1, &sVBO); glBindBuffer(GL_ARRAY_BUFFER, sVBO);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), nullptr); glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)(2 * sizeof(float))); glEnableVertexAttribArray(1);
	glGenTextures(1, &sAtlas); glBindTexture(GL_TEXTURE_2D, sAtlas);
	std::vector<unsigned char> blank(ATLAS * ATLAS, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ATLAS, ATLAS, 0, GL_RED, GL_UNSIGNED_BYTE, blank.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, oldTexture); glBindVertexArray(oldVAO); glBindBuffer(GL_ARRAY_BUFFER, oldBuffer);
	return true;
}

float Text_Width(const std::string& text, float height)
{
	if (height <= 0 || !InitFont()) return 0;
	float width = 0;
	for (wchar_t ch : Decode(text)) width += GetGlyph(ch).advance;
	return width * height / std::max(1L, sMetrics.tmHeight - sMetrics.tmInternalLeading);
}

void Text_Draw(float x, float y, const std::string& text, float height, glm::vec4 color)
{
	if (text.empty() || height <= 0 || !Text_Init()) return;
	GLint oldProgram, oldVAO, oldBuffer, oldActive, oldTexture, oldAlignment;
	glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram); glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVAO);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldBuffer); glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActive);
	glActiveTexture(GL_TEXTURE0); glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldAlignment); glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, sAtlas);
	float scale = height / std::max(1L, sMetrics.tmHeight - sMetrics.tmInternalLeading);
	float baseline = y + sMetrics.tmAscent * scale;
	std::vector<TextVertex> vertices;
	for (wchar_t ch : Decode(text))
	{
		Glyph& glyph = GetGlyph(ch);
		// GetGlyph can return the cached fallback; use its atlas slot.
		if (glyph.width > 0 && glyph.height > 0)
		{
			// Uploading a small cached bitmap is bounded; the atlas itself is allocated once.
			// The uploaded flag is stored in the cache, not in a temporary glyph copy.
			if (!glyph.uploaded)
			{
				glTexSubImage2D(GL_TEXTURE_2D, 0, glyph.x, glyph.y, glyph.width, glyph.height, GL_RED, GL_UNSIGNED_BYTE, glyph.pixels.data());
				glyph.uploaded = true;
			}
			float l = x + glyph.left * scale, t = baseline - glyph.top * scale;
			float r = l + glyph.width * scale, b = t + glyph.height * scale;
			float u0 = (float)glyph.x / ATLAS, v0 = (float)glyph.y / ATLAS;
			float u1 = (float)(glyph.x + glyph.width) / ATLAS, v1 = (float)(glyph.y + glyph.height) / ATLAS;
			vertices.insert(vertices.end(), {{l,t,u0,v0},{l,b,u0,v1},{r,b,u1,v1},{l,t,u0,v0},{r,b,u1,v1},{r,t,u1,v0}});
		}
		x += glyph.advance * scale;
	}
	glUseProgram(sProgram);
	glUniformMatrix4fv(sProjection, 1, GL_FALSE, glm::value_ptr(glm::ortho(0.0f, 1280.0f, 720.0f, 0.0f, -1.0f, 1.0f)));
	glUniform4fv(sColor, 1, glm::value_ptr(color)); glUniform1i(glGetUniformLocation(sProgram, "glyphAtlas"), 0);
	glBindVertexArray(sVAO); glBindBuffer(GL_ARRAY_BUFFER, sVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(TextVertex), vertices.data(), GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());
	glBindVertexArray(oldVAO); glBindBuffer(GL_ARRAY_BUFFER, oldBuffer); glUseProgram(oldProgram);
	glPixelStorei(GL_UNPACK_ALIGNMENT, oldAlignment); glBindTexture(GL_TEXTURE_2D, oldTexture); glActiveTexture(oldActive);
}

void Text_Shutdown()
{
	if (sVBO) glDeleteBuffers(1, &sVBO);
	if (sVAO) glDeleteVertexArrays(1, &sVAO);
	if (sAtlas) glDeleteTextures(1, &sAtlas);
	if (sProgram) glDeleteProgram(sProgram);
	sVBO = sVAO = sAtlas = sProgram = 0;
	if (sDC) { SelectObject(sDC, sOldFont); DeleteDC(sDC); sDC = nullptr; }
	if (sFont) { DeleteObject(sFont); sFont = nullptr; }
	sGlyphs.clear();
}
