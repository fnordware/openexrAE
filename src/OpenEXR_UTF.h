//
//	OpenEXR UTF
//		by Brendan Bolles <brendan@fnordware.com>
//
//	UTF utility functions
//
//	Part of the fnord OpenEXR tools
//		http://www.fnordware.com/OpenEXR/
//
//


#ifndef OPENEXR_UTF_H
#define OPENEXR_UTF_H

#include <string>

typedef unsigned short utf16_char;

bool UTF8toUTF16(const std::string &str, utf16_char *buf, unsigned int max_len);
std::string UTF16toUTF8(const utf16_char *str);


#endif // OPENEXR_UTF_H