#ifndef __SPECIFIC_DATA_H__
#define __SPECIFIC_DATA_H__

#include "lstLib.h"
class SpecificData  {
public:
	SpecificData(char *name);
	virtual ~SpecificData();
	int setSpecificItem(char *key, char *value);
	char* getSpcificItem(char *key, char *defautValue);
private:
	char *name;

	typedef struct _Item {
		NODE node;
		char *key;
		char *value;
	}Item;


	LIST items;

};


#endif//__SPECIFIC_DATA_H__
