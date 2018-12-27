#ifndef IX_INTERNAL_H
#define IX_INTERNAL_H

#include "../Constants.h"
#include "../rm/RID.h"
#include "../pf/pf.h"

#define D 3
#define RID_BUCKET_SIZE 400

struct IndexInfo {
	AttrType attrType;
	int attrLength;
	PageNum rootPageNum;
};
struct LeafData {
	void *pdata;
	PageNum ridPageNum;
	void init() { ridPageNum = -1; }
};
struct LeafNode {
	int size;
	LeafData data[D << 1];
	PageNum leftPage, rightPage;
	void init() { size = 0; leftPage = rightPage = -1; }
	void insertDataIntoPos(void *pData, PageNum pageNum, int pos);
	void deleteDataAtPos(int pos);
	void split(LeafNode* splitNode, PageNum newPage, PageNum thisPage);
};
struct InternalNode {
	int keyCount;
	PageNum son[2 * D + 1];
	void init() { keyCount = 0; for (int i = 0; i <= 2 * D; ++i) son[i] = -1; }
	void* pData[2 * D + 1];
	void InsertKeyAfterPos(void *pData, PageNum pageNum, int pos);
	void ChangeKey(void *pData, int pos);
	void Split(InternalNode* splitNode);
};
struct NodePagePacket {
	int nodeType;
	/* 0 -> InternalNode 1 -> LeafNode */
	LeafNode leafNode;
	InternalNode internalNode;
};
struct RIDPagePacket {
	int size;
	RID r[RID_BUCKET_SIZE];
	RC insertRID(const RID rid);
	RC deleteRID(const RID rid);
};
struct RIDPositionInfo {
	LeafNode leafNode;
	int posInLeaf;
	RIDPagePacket ridPagePacket;
	int ridPagePos;
	RID getCurRID() { return ridPagePacket.r[ridPagePos]; }
};

#endif