/*
 * File:   bitstream.h
 * Author: Sergio
 *
 * Created on 9 de diciembre de 2010, 8:48
 */

#ifndef _BITSTREAM_H_
#define	_BITSTREAM_H_



class BitReader {
public:
	BitReader(unsigned char *data, unsigned int size);

	unsigned int Get(unsigned int n);

	bool Check(int n, unsigned int val);

	void Skip(unsigned int n);

	unsigned long int Left();

	unsigned int Peek(unsigned int n);

	unsigned int GetPos();
public:
	unsigned int Cache();
	unsigned int PeekNextCached();

	void SkipCached(unsigned int n);

	unsigned int GetCached(unsigned int n);

private:
	unsigned char *buffer;
	unsigned int bufferLen;
	unsigned int bufferPos;
	unsigned int cache;
	unsigned char cached;
};

class BitWriter {
public:
	BitWriter(unsigned char *data, unsigned int size);

	void Reset();
	unsigned int Flush();

	inline void FlushCache();

	inline void Align();

	unsigned int Put(unsigned char n, unsigned int v);
	unsigned int Put(unsigned char n, BitReader &reader);
private:
	unsigned char *data;
	unsigned int size;
	unsigned char *buffer;
	unsigned int bufferLen;
	unsigned int bufferSize;
	unsigned int cache;
	unsigned char cached;
};


#endif	/* BITSTREAM_H */
