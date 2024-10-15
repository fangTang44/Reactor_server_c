#include "ChannelMap.h"
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// 初始化
struct ChannelMap* channelMapInit(int size) {
	struct ChannelMap* map = (struct ChannelMap*)malloc(sizeof(struct ChannelMap));
	assert(map != NULL);
	map->size = size;
	map->list = (struct Channel**)malloc(sizeof(struct Channel*) * size);
	return map;
}

// 清空map
bool ChannelMapClear(struct ChannelMap* map) {
	if (map == NULL) {
		return false;
	}
	for (int i = 0; i < map->size; i++) {
		if (map->list[i] != NULL) {
			free(map->list);
		}
	}
	free(map->list);
	map->size = 0;
	return true;
}

// 重新分配内存空间
bool makeMapRoom(struct ChannelMap* map, int newsize, int unitSize) {
	if (map == NULL || map->size > newsize) {
		return false;
	}
	int curSize = map->size;
	// 容量每次扩大一倍
	while (curSize < newsize) {
		curSize *= 2;
	}
	// 扩容
	struct Channel** temp = realloc(map->list, curSize * unitSize);
	if (temp == NULL) {
		return false;
	}	
	map->list = temp;
	memset(map->list[map->size], 0, (curSize - map->size) * unitSize);
	map->size = curSize;
	return true;
}