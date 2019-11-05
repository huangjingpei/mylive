
#include <stdexcept>
#include "BitStream.h"

inline unsigned char  get1(const unsigned char *data,size_t i) { return data[i]; }
inline unsigned int get2(const unsigned char *data,size_t i) { return (unsigned int)(data[i+1]) | ((unsigned int)(data[i]))<<8; }
inline unsigned int get3(const unsigned char *data,size_t i) { return (unsigned int)(data[i+2]) | ((unsigned int)(data[i+1]))<<8 | ((unsigned int)(data[i]))<<16; }
inline unsigned int get4(const unsigned char *data,size_t i) { return (unsigned int)(data[i+3]) | ((unsigned int)(data[i+2]))<<8 | ((unsigned int)(data[i+1]))<<16 | ((unsigned int)(data[i]))<<24; }
inline unsigned long int get8(const unsigned char *data,size_t i) { return ((unsigned long int)get4(data,i))<<32 | get4(data,i+4);	}

inline void set1(unsigned char *data,size_t i,unsigned char val)
{
	data[i] = val;
}
inline void set2(unsigned char *data,size_t i,unsigned int val)
{
	data[i+1] = (unsigned char)(val);
	data[i]   = (unsigned char)(val>>8);
}
inline void set3(unsigned char *data,size_t i,unsigned int val)
{
	data[i+2] = (unsigned char)(val);
	data[i+1] = (unsigned char)(val>>8);
	data[i]   = (unsigned char)(val>>16);
}
inline void set4(unsigned char *data,size_t i,unsigned int val)
{
	data[i+3] = (unsigned char)(val);
	data[i+2] = (unsigned char)(val>>8);
	data[i+1] = (unsigned char)(val>>16);
	data[i]   = (unsigned char)(val>>24);
}
inline void set8(unsigned char *data,size_t i,unsigned long int val)
{
	data[i+7] = (unsigned char)(val);
	data[i+6] = (unsigned char)(val>>8);
	data[i+5] = (unsigned char)(val>>16);
	data[i+4] = (unsigned char)(val>>24);
	data[i+3] = (unsigned char)(val>>32);
	data[i+2] = (unsigned char)(val>>40);
	data[i+1] = (unsigned char)(val>>48);
	data[i]   = (unsigned char)(val>>56);
}


BitReader::BitReader(unsigned char *data, unsigned int size) {
	//Store
	buffer = data;
	bufferLen = size;
	//nothing in the cache
	cached = 0;
	cache = 0;
	bufferPos = 0;
}
inline unsigned int BitReader::Get(unsigned int n) {
	unsigned int ret = 0;
	if (n > cached) {
		//What we have to read next
		unsigned char a = n - cached;
		//Get remaining in the cache
		ret = cache >> (32 - n);
		//Cache next
		Cache();
		//Get the remaining
		ret = ret | GetCached(a);
	} else
		ret = GetCached(n);
	//Debug("Readed %d:\n",n);
	//BitDump(ret,n);
	return ret;
}

inline bool BitReader::Check(int n, unsigned int val) {
	return Get(n) == val;
}

inline void BitReader::Skip(unsigned int n) {
	if (n > cached) {
		//Get what is left to skip
		unsigned char a = n - cached;
		//Cache next
		Cache();
		//Skip cache
		SkipCached(a);
	} else
		//Skip cache
		SkipCached(n);
}

inline unsigned long int BitReader::Left() {
	return cached + bufferLen * 8;
}

inline unsigned int BitReader::Peek(unsigned int n) {
	unsigned int ret = 0;
	if (n > cached) {
		//What we have to read next
		unsigned char a = n - cached;
		//Get remaining in the cache
		ret = cache >> (32 - n);
		//Get the remaining
		ret = ret | (PeekNextCached() >> (32 - a));
	} else
		ret = cache >> (32 - n);
	//Debug("Peeked %d:\n",n);
	//BitDump(ret,n);
	return ret;
}

inline unsigned int BitReader::GetPos() {
	return bufferPos * 8 - cached;
}

inline unsigned int BitReader::Cache() {
	//Check left buffer
	if (bufferLen > 3) {
		//Update cache
		cache = get4(buffer, 0);
		//Update bit count
		cached = 32;
		//Increase pointer
		buffer += 4;
		bufferPos += 4;
		//Decrease length
		bufferLen -= 4;
	} else if (bufferLen == 3) {
		//Update cache
		cache = get3(buffer, 0) << 8;
		//Update bit count
		cached = 24;
		//Increase pointer
		buffer += 3;
		bufferPos += 3;
		//Decrease length
		bufferLen -= 3;
	} else if (bufferLen == 2) {
		//Update cache
		cache = get2(buffer, 0) << 16;
		//Update bit count
		cached = 16;
		//Increase pointer
		buffer += 2;
		bufferPos += 2;
		//Decrease length
		bufferLen -= 2;
	} else if (bufferLen == 1) {
		//Update cache
		cache = get1(buffer, 0) << 24;
		//Update bit count
		cached = 8;
		//Increase pointer
		buffer++;
		bufferPos++;
		//Decrease length
		bufferLen--;
	} else
		throw std::runtime_error("Reading past end of stream");

	//Debug("Reading int cache");
	//BitDump(cache,cached);

	//return number of bits
	return cached;
}

inline unsigned int BitReader::PeekNextCached() {
	//Check left buffer
	if (bufferLen > 3) {
		//return  cached
		return get4(buffer, 0);
	} else if (bufferLen == 3) {
		//return  cached
		return get3(buffer, 0) << 8;
	} else if (bufferLen == 2) {
		//return  cached
		return get2(buffer, 0) << 16;
	} else if (bufferLen == 1) {
		//return  cached
		return get1(buffer, 0) << 24;
	} else
		throw std::runtime_error("Reading past end of stream");
}

inline void BitReader::SkipCached(unsigned int n) {
	//Move
	cache = cache << n;
	//Update cached unsigned chars
	cached -= n;

}

inline unsigned int BitReader::GetCached(unsigned int n) {
	//Get bits
	unsigned int ret = cache >> (32 - n);
	//Skip thos bits
	SkipCached(n);
	//Return bits
	return ret;
}

BitWriter::BitWriter(unsigned char *data, unsigned int size) {
	//Store pointers
	this->data = data;
	this->size = size;
	//And reset to init values
	Reset();
}

void BitWriter::Reset() {
	//Init
	buffer = data;
	bufferLen = 0;
	bufferSize = size;
	//nothing in the cache
	cached = 0;
	cache = 0;
}

unsigned int BitWriter::Flush() {
	Align();
	FlushCache();
	return bufferLen;
}

inline void BitWriter::FlushCache() {
	//Check if we have already finished
	if (!cached)
		//exit
		return;
	//Check size
	if (cached > bufferSize * 8)
		throw std::runtime_error("Writing past end of bit stream");
	//Debug("Flushing  cache");
	//BitDump(cache,cached);
	if (cached == 32) {
		//Set data
		set4(buffer, 0, cache);
		//Increase pointers
		bufferSize -= 4;
		buffer += 4;
		bufferLen += 4;
	} else if (cached == 24) {
		//Set data
		set3(buffer, 0, cache);
		//Increase pointers
		bufferSize -= 3;
		buffer += 3;
		bufferLen += 3;
	} else if (cached == 16) {
		set2(buffer, 0, cache);
		//Increase pointers
		bufferSize -= 2;
		buffer += 2;
		bufferLen += 2;
	} else if (cached == 8) {
		set1(buffer, 0, cache);
		//Increase pointers
		--bufferSize;
		++buffer;
		++bufferLen;
	}
	//Nothing cached
	cached = 0;
}

inline void BitWriter::Align() {
	if (cached % 8 == 0)
		return;

	if (cached > 24)
		Put(32 - cached, 0);
	else if (cached > 16)
		Put(24 - cached, 0);
	else if (cached > 8)
		Put(16 - cached, 0);
	else
		Put(8 - cached, 0);
}

unsigned int BitWriter::Put(unsigned char n, unsigned int v) {
	if (n + cached > 32) {
		unsigned char a = 32 - cached;
		unsigned char b = n - a;
		//Add to cache
		cache = (cache << a) | ((v >> b) & (0xFFFFFFFF >> cached));
		//Set cached unsigned chars
		cached = 32;
		//Flush into memory
		FlushCache();
		//Set new cache
		cache = v & (0xFFFFFFFF >> (32 - b));
		//Increase cached
		cached = b;
	} else {
		//Add to cache
		cache = (cache << n) | (v & (0xFFFFFFFF >> (32 - n)));
		//Increase cached
		cached += n;
	}
	return v;
}

unsigned int BitWriter::Put(unsigned char n, BitReader &reader) {
	return Put(n, reader.Get(n));
}

