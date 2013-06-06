//
//	EXtractoR
//		by Brendan Bolles <brendan@fnordware.com>
//
//	extract float channels
//
//	Part of the fnord OpenEXR tools
//		http://www.fnordware.com/OpenEXR/
//
//

#pragma once

#ifndef _EXTRACTOR_DIALOG_H_
#define _EXTRACTOR_DIALOG_H_

#include "EXtractoR.h"

#include <ImfChannelList.h>

#include <string>
#include <vector>
#include <map>

typedef std::vector<std::string> ChannelVec;
typedef std::map<std::string, ChannelVec> LayerMap;

bool EXtractoR_Dialog(
	const Imf::ChannelList	&channels,
	const LayerMap		&layers,
	std::string			&red,
	std::string			&green,
	std::string			&blue,
	std::string			&alpha,
	const char			*plugHndl,
	const void			*mwnd);


#endif // _EXTRACTOR_DIALOG_H_