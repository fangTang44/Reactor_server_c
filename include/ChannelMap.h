#pragma once
#include "Channel.h"

// struct Channel* list[]
struct ChannelMap {
	// struct Channel* list[]
	struct Channel** list;
	// 记录数组的总大小
	int size;
};

// 初始化
struct ChannelMap* channelMapInit(int size);
// 清空map
bool ChannelMapClear(struct ChannelMap* map);
// 重新分配内存空间
bool makeMapRoom(struct ChannelMap* map, int newsize, int unitSize);