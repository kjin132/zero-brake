// =====================================================================================
//  [담당: 유현조] 7세그먼트 글자
//        a
//      f   b
//        g
//      e   c
//        d
// =====================================================================================
#include "ui/SevenSeg.h"
#include "ui/Hud.h"
#include <cctype>

static const char* Segments(char ch)
{
	switch (ch)
	{
	case '0': case 'O': case 'o': return "abcdef";
	case '1': case 'I': case 'i': return "bc";
	case '2': return "abged";
	case '3': return "abgcd";
	case '4': return "fgbc";
	case '5': case 'S': case 's': return "afgcd";
	case '6': case 'G': case 'g': return "afgedc";
	case '7': return "abc";
	case '8': return "abcdefg";
	case '9': return "abcfgd";
	case 'A': case 'a': return "abcefg";
	case 'B': case 'b': return "fedcg";
	case 'C': case 'c': return "afed";
	case 'D': case 'd': return "bcdeg";
	case 'E': case 'e': return "afged";
	case 'F': case 'f': return "afge";
	case 'H': case 'h': return "fbgec";
	case 'J': case 'j': return "bcde";
	case 'L': case 'l': return "fed";
	case 'N': case 'n': return "ecg";
	case 'P': case 'p': return "abfge";
	case 'R': case 'r': return "eg";
	case 'T': case 't': return "fged";
	case 'U': case 'u': case 'V': case 'v': return "fedcb";
	case 'Y': case 'y': return "fgbcd";
	case '-': return "g";
	default: return "";
	}
}

static float CharWidth(char ch, float size)
{
	if (ch == ':' || ch == '.') return size * 0.3f;
	if (ch == ' ') return size * 0.5f;
	return size * 0.72f;
}

float SevenSeg_TextWidth(const std::string& text, float size)
{
	float w = 0.0f;
	for (char ch : text) w += CharWidth(ch, size);
	return w;
}

float SevenSeg_Text(float x, float y, const std::string& text, float size, glm::vec4 color)
{
	float x0 = x;
	float t = size * 0.13f;          // 막대 두께
	float w = size * 0.55f;          // 글자 폭
	float h = size;
	for (char ch : text)
	{
		if (ch == ':' || ch == '.')
		{
			if (ch == ':') Hud_Rect(x + t * 0.5f, y + h * 0.28f, t, t, color);
			Hud_Rect(x + t * 0.5f, y + h - t, t, t, color);
		}
		else if (ch == '/')
		{
			for (int k = 0; k < 6; k++)
				Hud_Rect(x + w * (0.8f - k * 0.13f), y + h * k / 6.0f, t, h / 6.0f, color);
		}
		else
		{
			const char* segs = Segments(ch);
			for (const char* s = segs; *s; s++)
			{
				switch (*s)
				{
				case 'a': Hud_Rect(x + t, y, w - 2 * t, t, color); break;
				case 'g': Hud_Rect(x + t, y + h / 2 - t / 2, w - 2 * t, t, color); break;
				case 'd': Hud_Rect(x + t, y + h - t, w - 2 * t, t, color); break;
				case 'f': Hud_Rect(x, y + t, t, h / 2 - t * 1.5f, color); break;
				case 'b': Hud_Rect(x + w - t, y + t, t, h / 2 - t * 1.5f, color); break;
				case 'e': Hud_Rect(x, y + h / 2 + t / 2, t, h / 2 - t * 1.5f, color); break;
				case 'c': Hud_Rect(x + w - t, y + h / 2 + t / 2, t, h / 2 - t * 1.5f, color); break;
				}
			}
		}
		x += CharWidth(ch, size);
	}
	return x - x0;
}
