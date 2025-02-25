#ifndef __PITEM_H
#define __PITEM_H

#include "Packet.h"

enum ItemMode {
	imInventory = 3003,
	imEquipment = 3004,
	imUnk2 = 3005,
	imUnk3 = 3006,
	imPetAccessory = 3007,
	imUnk4 = 3008, //Related to Pet Shop?
	imWuxingPreview = 3010 
};

class pItem : public Packet {
public:
	int mode;
	int ownerId;
	int itemId, cost;
	int look, itemSort, levelRequired;
	short life, power, attack, defence, dexterity;

	short antiPoison, antiFreeze, antiSleep, antiChaos;
	short count, stack;

	char name[16] = { 0 };
	char creator[16] = { 0 };

	pItem(char* buf, char* encBuf);
	~pItem();

	virtual void process();
	void pItem::debugPrint();
};

#endif