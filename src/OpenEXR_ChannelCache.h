//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#ifndef OPENEXR_CHANNEL_CACHE_H
#define OPENEXR_CHANNEL_CACHE_H

#include "ImfHybridInputFile.h"

#include "OpenEXR_PlatformIO.h"

#include "fnord_SuiteHandler.h"

#include <IlmThreadMutex.h>

#include <list>
#include <time.h>


class CancelExc : public std::exception
{
  public:
	CancelExc(A_Err err) throw() : _err(err) {}
	virtual ~CancelExc() throw() {}
	
	A_Err err() const throw() { return _err; }
	virtual const char* what() const throw() { return "User cancelled"; }

  private:
	A_Err _err;
};


class OpenEXR_ChannelCache
{
  public:
	OpenEXR_ChannelCache(const SPBasicSuite *pica_basicP, const AEIO_InterruptFuncs *inter,
							Imf::HybridInputFile &in, const IStreamPlatform &stream);
	~OpenEXR_ChannelCache();
	
	void fillFrameBuffer(const Imf::FrameBuffer &framebuffer, const Imath::Box2i &dw);
	
	const PathString & getPath() const { return _path; }
	DateTime getModTime() const { return _modtime; }
	
	double cacheAge() const;
	bool cacheIsStale(int timeout) const;
	
  private:
	AEGP_SuiteHandler suites;
	
	int _width;
	int _height;
	
	typedef struct ChannelCache {
		Imf::PixelType	pix_type;
		AEIO_Handle		bufH;
		
		ChannelCache(Imf::PixelType t=Imf::HALF, AEIO_Handle b=NULL) : pix_type(t), bufH(b) {}
	} ChannelCache;
	
	typedef std::map<std::string, ChannelCache> ChannelMap;
	ChannelMap _cache;
	
	PathString _path;
	DateTime _modtime;

	time_t _last_access;
	void updateCacheTime();
};


class OpenEXR_CachePool
{
  public:
	OpenEXR_CachePool();
	~OpenEXR_CachePool();
	
	void configurePool(int max_caches, const SPBasicSuite *pica_basicP=NULL);
	
	OpenEXR_ChannelCache *findCache(const IStreamPlatform &stream) const;
	
	OpenEXR_ChannelCache *addCache(Imf::HybridInputFile &in, const IStreamPlatform &stream, const AEIO_InterruptFuncs *inter);
	
	bool deleteStaleCaches(int timeout); // returns true if something was deleted
	
  private:
	int _max_caches;
	const SPBasicSuite *_pica_basicP;
	std::list<OpenEXR_ChannelCache *> _pool;
	IlmThread::Mutex _mutex;
};


int ScanlineBlockSize(const Imf::HybridInputFile &in);


#endif // OPENEXR_CHANNEL_CACHE_H