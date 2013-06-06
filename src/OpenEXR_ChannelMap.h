//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#ifndef OPENEXR_CHANNEL_MAP_H
#define OPENEXR_CHANNEL_MAP_H


#include <string>
#include <vector>


class ChannelEntry {
	public:
		ChannelEntry() {}
		ChannelEntry(const char *channel_name, long type_code, long data_code);
		ChannelEntry(const char *channel_name, const char *type_code, const char *data_code);
		
		std::string name(void) { return chan_name; }
		long type(void) { return chan_type_code; }
		long data(void) { return chan_data_code; }
		
		int dimensions(void);
		std::string chan_part(int index);
		std::string key_name(void) { return chan_part(0); }
		
	private:
		std::vector<int> bar_positions(void);
		
		std::string chan_name;
		long chan_type_code;
		long chan_data_code;
};

class ChannelMap {
	public:
		ChannelMap(const char *file_path);
				
		bool exists(void) { return map.size() > 0; }
		
		bool findEntry(const char *channel_name, ChannelEntry &entry, bool search_all=false);
		bool findEntry(const char *channel_name, bool search_all=false);
		void addEntry(ChannelEntry entry) { map.push_back(entry); }
	
	private:
		std::vector<ChannelEntry> map;
};


#endif // OPENEXR_CHANNEL_MAP_H