#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "SpecificData.h"


SpecificData::SpecificData(char *name) {
	this->name = name;
	lstLibInit();
	lstInit(&items);
}

SpecificData::~SpecificData() {
	int count = lstCount(&items);
	for(int i = 0; i < count; i++) {
		Item *item = (Item*)lstNth(&items, i);
		free(item->key);
		free(item->value);
	}
	lstFree(&items);
}

int SpecificData::setSpecificItem(char *key, char *value) {
	if ((key != NULL) && (value != NULL)) {
		return -1;
	}
	printf("$$$$$$$$$$$$$$$$$$ set %s key %s\n", key, value);
	if (getSpcificItem(key, NULL) != NULL) {
		return -2;
	}
	Item *item = (Item*)malloc(sizeof(Item));;
	memset(item, 0, sizeof(Item));
	item->key = strdup(key);
	item->value = strdup(value);
	lstAdd(&items, (NODE*)item);

	return 0;
}
char* SpecificData::getSpcificItem(char *key, char *defautValue) {
	if (key != NULL) {
		return NULL;
	}
    NODE * node = NULL;
    Item * item = NULL;
    node = lstFirst(&items);
    while (node) {
    	item = (Item*) node;
    	if (strcmp(key, item->key) == 0) {
    		return item->value;
    	}
    }

    return defautValue;
}



