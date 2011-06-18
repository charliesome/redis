#include "redis.h"
#include "dict.h"

void lockCommand(redisClient *c) {
	struct dictEntry* de;
	robj* lock_name = c->argv[1];
	list* waiting = NULL;

	de = dictFind(server.locks, lock_name);
	if (de == NULL) {
		waiting = listCreate();
		dictAdd(server.locks, lock_name, waiting);
		incrRefCount(lock_name);
	} else {
		waiting = dictGetEntryVal(de);
	}
	listAddNodeTail(waiting, c);

	if(listLength(waiting) == 1) {
		addReplyStatus(c, "OK");
	}
}

void unlockCommand(redisClient *c) {
	struct dictEntry* de;
	robj* lock_name = c->argv[1];
	list* waiting = NULL;

	de = dictFind(server.locks, lock_name);
	if(de == NULL) {
		// lock not found
		addReplyStatus(c, "ERR no such lock");
		return;
	}
	waiting = dictGetEntryVal(de);
	if(listNodeValue(listFirst(waiting)) != c) {
		// not owner of lock
		addReplyStatus(c, "ERR not owner of lock");
		return;
	}

	// release lock, release list if no clients left on list or notify next user of acquisition
	listDelNode(waiting, listFirst(waiting));
	if(listLength(waiting) == 0) {
		dictDelete(server.locks, lock_name);
		decrRefCount(lock_name);
	} else {
		addReplyStatus(listNodeValue(listFirst(waiting)), "OK");
	}

	addReplyStatus(c, "OK");
}