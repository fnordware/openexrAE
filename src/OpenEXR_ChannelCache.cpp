//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#include "OpenEXR_ChannelCache.h"

#include <half.h>
#include <ImfChannelList.h>
#include <IexBaseExc.h>

#include <IlmThread.h>
#include <IlmThreadPool.h>

#include <ImfVersion.h>
#include <ImfTileDescriptionAttribute.h>

#include <assert.h>

#include <vector>
#include <algorithm>


using namespace Imf;
using namespace Imath;
using namespace Iex;
using namespace IlmThread;
using namespace std;

extern AEGP_PluginID S_mem_id;


static void
FixSubsampling(const FrameBuffer &framebuffer, const Box2i &dw)
{
	for(FrameBuffer::ConstIterator i = framebuffer.begin(); i != framebuffer.end(); i++)
	{
		const Slice & slice = i.slice();
		
		if(slice.xSampling != 1 || slice.ySampling != 1)
		{
			char *row_subsampled = (char *)slice.base + (slice.yStride * (dw.min.y + ((1 + dw.max.y - dw.min.y) / slice.ySampling) - 1)) + (slice.xStride * (dw.min.x + ((1 + dw.max.x - dw.min.x) / slice.ySampling) - 1));
			char *row_expanded = (char *)slice.base + (slice.yStride * dw.max.y) + (slice.xStride * dw.max.x);
			
			for(int y = dw.max.y; y >= dw.min.y; y--)
			{
				char *pix_subsampled = row_subsampled;
				char *pix_expanded = row_expanded;
				
				for(int x = dw.max.x; x >= dw.min.x; x--)
				{
					if(slice.type == Imf::HALF)
					{
						*((half *)pix_expanded) = *((half *)pix_subsampled);
					}
					else if(slice.type == Imf::FLOAT)
					{
						*((float *)pix_expanded) = *((float *)pix_subsampled);
					}
					else if(slice.type == Imf::UINT)
					{
						*((unsigned int *)pix_expanded) = *((unsigned int *)pix_subsampled);
					}
					
					if(x % slice.xSampling == 0)
						pix_subsampled -= slice.xStride;
					
					pix_expanded -= slice.xStride;
				}
				
				if(y % slice.ySampling == 0)
					row_subsampled -= slice.yStride;
				
				row_expanded -= slice.yStride;
			}
		}
	}
}


OpenEXR_ChannelCache::OpenEXR_ChannelCache(const SPBasicSuite *pica_basicP, const AEIO_InterruptFuncs *inter,
											HybridInputFile &in, const IStreamPlatform &stream) :
	suites(pica_basicP),
	_path(stream.getPath()),
	_modtime(stream.getModTime())
{
	const Box2i &dw = in.dataWindow();
	
	_width = dw.max.x - dw.min.x + 1;
	_height = dw.max.y - dw.min.y + 1;
	
	
	FrameBuffer frameBuffer;
	
	try
	{
		const ChannelList &channels = in.channels();
		
		for(ChannelList::ConstIterator i = channels.begin(); i != channels.end(); ++i)
		{
			const Channel &channel = i.channel();
			
			const size_t pix_size =	channel.type == Imf::HALF ? sizeof(half) :
									channel.type == Imf::FLOAT ? sizeof(float) :
									channel.type == Imf::UINT ? sizeof(unsigned int) :
									sizeof(float);
									
			const size_t rowbytes = pix_size * _width;
			const size_t data_size = rowbytes * _height;
			
			
			AEIO_Handle bufH = NULL;
			
			suites.MemorySuite()->AEGP_NewMemHandle(S_mem_id, "Channel Cache",
													data_size,
													AEGP_MemFlag_CLEAR, &bufH);
			
			if(bufH == NULL)
				throw NullExc("Can't allocate a channel cache handle like I need to.");
			
			
			_cache[ i.name() ] = ChannelCache(channel.type, bufH);
			
			
			char *buf = NULL;
			
			suites.MemorySuite()->AEGP_LockMemHandle(bufH, (void**)&buf);
			
			if(buf == NULL)
				throw NullExc("Why is the locked handle NULL?");
			
			
			char * const channel_origin = buf - (pix_size * dw.min.x) - (rowbytes * dw.min.y);
			
			frameBuffer.insert(i.name(), Slice(	channel.type,
												channel_origin,
												pix_size,
												rowbytes,
												channel.xSampling,
												channel.ySampling,
												0.0) );
		}
		
		
		in.setFrameBuffer(frameBuffer);
		
	
		A_Err err = A_Err_NONE;
	
	#ifdef NDEBUG
		#define CONT()	( (inter && inter->abort0) ? !(err = inter->abort0(inter->refcon) ) : TRUE)
		#define PROG(COUNT, TOTAL)	( (inter && inter->progress0) ? !(err = inter->progress0(inter->refcon, COUNT, TOTAL) ) : TRUE)
	#else
		#define CONT()					TRUE
		#define PROG(COUNT, TOTAL)		TRUE
	#endif
		
		const int scanline_block_size = ScanlineBlockSize(in);
	
	
		int y = dw.min.y;
		
		while(y <= dw.max.y && PROG(y - dw.min.y, dw.max.y - dw.min.y) )
		{
			int high_scanline = min(y + scanline_block_size - 1, dw.max.y);
			
			in.readPixels(y, high_scanline);
			
			y = high_scanline + 1;
		}
		
		
		if(err)
			throw CancelExc(err);
	}
	catch(IoExc) {} // we catch these so that partial files are read partially without error
	catch(InputExc) {}
	catch(...)
	{
		// out of memory or worse; kill anything that got allocated
		for(ChannelMap::iterator i = _cache.begin(); i != _cache.end(); ++i)
		{
			if(i->second.bufH != NULL)
			{
				suites.MemorySuite()->AEGP_FreeMemHandle(i->second.bufH);
				
				i->second.bufH = NULL;
			}
		}
		
		throw; // re-throw the exception
	}
	

	
	FixSubsampling(frameBuffer, dw);
	
	
	for(ChannelMap::const_iterator i = _cache.begin(); i != _cache.end(); ++i)
	{
		if(i->second.bufH != NULL)
			suites.MemorySuite()->AEGP_UnlockMemHandle(i->second.bufH);
	}
	
	
	updateCacheTime();
}


OpenEXR_ChannelCache::~OpenEXR_ChannelCache()
{
	for(ChannelMap::iterator i = _cache.begin(); i != _cache.end(); ++i)
	{
		if(i->second.bufH != NULL)
		{
			suites.MemorySuite()->AEGP_FreeMemHandle(i->second.bufH);
			
			i->second.bufH = NULL;
		}
	}
}


class CopyCacheTask : public Task
{
  public:
	CopyCacheTask(TaskGroup *group,
					const char *buf, int width, Imf::PixelType pix_type,
					const Slice &slice, const Box2i &dw, int row);
	virtual ~CopyCacheTask() {}
	
	virtual void execute();
	
	template <typename INTYPE, typename OUTTYPE>
	static void CopyRow(const char *in, char *out, size_t out_stride, int width);
	
  private:
	const char *_buf;
	int _width;
	Imf::PixelType _pix_type;
	const Slice &_slice;
	const Box2i &_dw;
	int _row;
};


CopyCacheTask::CopyCacheTask(TaskGroup *group,
								const char *buf, int width, Imf::PixelType pix_type,
								const Slice &slice, const Box2i &dw, int row) :
	Task(group),
	_buf(buf),
	_width(width),
	_pix_type(pix_type),
	_slice(slice),
	_dw(dw),
	_row(row)
{

}


void
CopyCacheTask::execute()
{
	const size_t pix_size =	_pix_type == Imf::HALF ? sizeof(half) :
							_pix_type == Imf::FLOAT ? sizeof(float) :
							_pix_type == Imf::UINT ? sizeof(unsigned int) :
							sizeof(float);
							
	const size_t rowbytes = pix_size * _width;
	
	const char *in_row = _buf + (rowbytes * _row);
				
	char *slice_row = _slice.base + (_slice.yStride * (_dw.min.y + _row)) + (_slice.xStride * _dw.min.x);
	
	if(_pix_type == Imf::HALF)
	{
		assert(_slice.type == Imf::FLOAT);
		
		CopyRow<half, float>(in_row, slice_row, _slice.xStride, _width);
	}
	else if(_pix_type == Imf::FLOAT)
	{
		assert(_slice.type == Imf::FLOAT);
		
		CopyRow<float, float>(in_row, slice_row, _slice.xStride, _width);
	}
	else if(_pix_type == Imf::UINT)
	{
		assert(_slice.type == Imf::UINT);
		
		CopyRow<unsigned int, unsigned int>(in_row, slice_row, _slice.xStride, _width);
	}
}


template <typename INTYPE, typename OUTTYPE>
void
CopyCacheTask::CopyRow(const char *in, char *out, size_t out_stride, int width)
{
	INTYPE *i = (INTYPE *)in;
	OUTTYPE *o = (OUTTYPE *)out;
	
	int o_step = out_stride / sizeof(OUTTYPE);
	
	while(width--)
	{
		*o = *i++;
		
		o += o_step;
	}
}


class FillSliceTask : public Task
{
  public:
	FillSliceTask(TaskGroup *group, const Slice &slice, const Box2i &dw, int row);
	virtual ~FillSliceTask() {}
	
	virtual void execute();
	
	template <typename OUTTYPE>
	static void FillRow(char *out, size_t out_stride, int width, double val);
	
  private:
	const Slice &_slice;
	const Box2i &_dw;
	int _row;
};


FillSliceTask::FillSliceTask(TaskGroup *group, const Slice &slice, const Box2i &dw, int row) :
	Task(group),
	_slice(slice),
	_dw(dw),
	_row(row)
{

}


void
FillSliceTask::execute()
{
	char *slice_row = _slice.base + (_slice.yStride * (_dw.min.y + _row)) + (_slice.xStride * _dw.min.x);
	
	int width = _dw.max.x - _dw.min.x + 1;
	
	if(_slice.type == Imf::FLOAT)
	{
		FillRow<float>(slice_row, _slice.xStride, width, _slice.fillValue);
	}
	else if (_slice.type == Imf::UINT)
	{
		FillRow<unsigned int>(slice_row, _slice.xStride, width, _slice.fillValue);
	}
}


template <typename OUTTYPE>
void
FillSliceTask::FillRow(char *out, size_t out_stride, int width, double val)
{
	OUTTYPE *o = (OUTTYPE *)out;
	
	int o_step = out_stride / sizeof(OUTTYPE);
	
	while(width--)
	{
		*o = val;
		
		o += o_step;
	}
}


void
OpenEXR_ChannelCache::fillFrameBuffer(const FrameBuffer &framebuffer, const Box2i &dw)
{
	vector<AEIO_Handle> locked_handles;

	if(true) // making a scope for TaskGroup
	{
		TaskGroup group;
	
		for(FrameBuffer::ConstIterator i = framebuffer.begin(); i != framebuffer.end(); ++i)
		{
			const Slice &slice = i.slice();
			
			assert(slice.xSampling == 1 && slice.ySampling == 1);
			assert(_width == dw.max.x - dw.min.x + 1);
			assert(_height == dw.max.y - dw.min.y + 1);
			
			ChannelMap::const_iterator cache = _cache.find( i.name() );
		
			if( cache == _cache.end() )
			{
				// don't have this channel, fill with the fill value
				for(int y=0; y < _height; y++)
				{
					ThreadPool::addGlobalTask(new FillSliceTask(&group, slice, dw, y) );
				}
			}
			else
			{
				if(cache->second.bufH == NULL)
					throw NullExc("Why is the handle NULL?");
				
				
				char *buf = NULL;
				
				suites.MemorySuite()->AEGP_LockMemHandle(cache->second.bufH, (void**)&buf);
				
				
				if(buf == NULL)
					throw NullExc("Why is the locked handle NULL?");
				
				locked_handles.push_back(cache->second.bufH);
				
				
				for(int y=0; y < _height; y++)
				{
					ThreadPool::addGlobalTask(new CopyCacheTask(&group,
															buf, _width, cache->second.pix_type,
															slice, dw, y) );
				}
			}
		}
	}
	
	
	for(vector<AEIO_Handle>::const_iterator i = locked_handles.begin(); i != locked_handles.end(); ++i)
	{
		suites.MemorySuite()->AEGP_UnlockMemHandle(*i);
	}
	
	
	updateCacheTime();
}


void
OpenEXR_ChannelCache::updateCacheTime()
{
	_last_access = time(NULL);
}


double
OpenEXR_ChannelCache::cacheAge() const
{
	return difftime(time(NULL), _last_access);
}


bool
OpenEXR_ChannelCache::cacheIsStale(int timeout) const
{
	return (cacheAge() > timeout);
}



OpenEXR_CachePool::OpenEXR_CachePool() :
	_max_caches(0),
	_pica_basicP(NULL)
{

}


OpenEXR_CachePool::~OpenEXR_CachePool()
{
	configurePool(0, NULL);
}


static bool
compare_age(const OpenEXR_ChannelCache *first, const OpenEXR_ChannelCache *second)
{
	return first->cacheAge() > second->cacheAge();
}


void
OpenEXR_CachePool::configurePool(int max_caches, const SPBasicSuite *pica_basicP)
{
	Lock lock(_mutex);

	_max_caches = max_caches;
	
	if(pica_basicP)
		_pica_basicP = pica_basicP;
	
	_pool.sort(compare_age);

	while(_pool.size() > _max_caches)
	{
		delete _pool.front();
		
		_pool.pop_front();
	}
}


static bool
MatchDateTime(const DateTime &d1, const DateTime &d2)
{
#ifdef __APPLE__
	return (d1.fraction == d2.fraction &&
			d1.lowSeconds == d2.lowSeconds &&
			d1.highSeconds == d2.highSeconds);
#else
	return (d1.dwHighDateTime == d2.dwHighDateTime &&
			d1.dwLowDateTime == d2.dwLowDateTime);
#endif
}


OpenEXR_ChannelCache *
OpenEXR_CachePool::findCache(const IStreamPlatform &stream) const
{
	Lock lock(_mutex);

	for(list<OpenEXR_ChannelCache *>::const_iterator i = _pool.begin(); i != _pool.end(); ++i)
	{
		if( MatchDateTime(stream.getModTime(), (*i)->getModTime()) &&
			stream.getPath() == (*i)->getPath() )
		{
			return *i;
		}
	}
	
	return NULL;
}


OpenEXR_ChannelCache *
OpenEXR_CachePool::addCache(HybridInputFile &in, const IStreamPlatform &stream, const AEIO_InterruptFuncs *inter)
{
	Lock lock(_mutex);

	_pool.sort(compare_age);
	
	while(_pool.size() && _pool.size() >= _max_caches)
	{
		delete _pool.front();
		
		_pool.pop_front();
	}
	
	if(_max_caches > 0)
	{
		try
		{
			OpenEXR_ChannelCache *new_cache = new OpenEXR_ChannelCache(_pica_basicP, inter, in, stream);
			
			_pool.push_back(new_cache);
			
			return new_cache;
		}
		catch(CancelExc) { throw; }
		catch(...) {}
	}
	
	return NULL;
}


bool
OpenEXR_CachePool::deleteStaleCaches(int timeout)
{
	Lock lock(_mutex);

	if(_pool.size() > 0)
	{
		const int old_size = _pool.size();
	
		_pool.sort(compare_age);
		
		if( _pool.front()->cacheIsStale(timeout) )
		{
			delete _pool.front(); // just going to delete one cache per cycle, the oldest one
			
			_pool.pop_front();
		}
		
		return (_pool.size() < old_size); // did something actually get deleted?
	}
	
	return false;
}


int ScanlineBlockSize(const HybridInputFile &in)
{
	// When multithreaded, we can see speedups if we read in enough scanlines at a time.
	// Piz and B44(A) compression use blocks of 32 scan lines,
	// so we'll make sure each thread gets at least a full block.
	
	int scanline_block_size = 32;
	
	if(in.header(0).compression() == DWAB_COMPRESSION)
		scanline_block_size = 256; // well, DWAB uses 256
	
	
	scanline_block_size *= max(globalThreadCount(), 1);
	
	
	if( isTiled( in.version() ) )
	{
		const TileDescriptionAttribute *tiles = in.header(0).findTypedAttribute<TileDescriptionAttribute>("tiles");
		
		if(tiles)
		{
			// if the image is tiled, use a multiple of tile height
			const int num_tiles = ceil((float)scanline_block_size / (float)tiles->value().ySize);
			
			scanline_block_size = num_tiles * tiles->value().ySize;
		}
	}
	
			
	const Box2i &dataW = in.dataWindow();
	const Box2i &dispW = in.displayWindow();
	
	if(dataW != dispW)
		scanline_block_size *= 2; // might not be on tile/block boundries, so double up
	
				
	return scanline_block_size;
}

