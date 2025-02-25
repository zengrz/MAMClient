#pragma once

enum GAMEMODE {
	GAMEMODE_NONE = 0,
	GAMEMODE_SELECTTEAM,
	GAMEMODE_JOINTEAM,
};

enum ELEMENT {
	ELEMENT_WATER = 2,
	ELEMENT_FIRE = 3,
	ELEMENT_METAL = 4,
	ELEMENT_WOOD = 5,
	ELEMENT_EARTH = 6,
};
#define SUPERPET 1;
#define UNEVOPET 7;

enum ChatChannel {
	ccNone = 0,
	nnUnknown = 2000,
	ccPrivate = 2001,
	ccAction = 2002,
	ccTeam = 2003,
	ccGuild = 2004,
	ccSystem = 2005,
	ccSpouse = 2006,
	ccNormal = 2007,
	ccShout = 2008,
	ccFriend = 2009,
	ccBroadcast = 2010,
	ccGM = 2011,
	ccHidden = 2101,
	ccAlert = 2102,
	ccVendor = 2104,
	cWebsite = 2105
};

enum HorizontalAlignment {
	HORIZONTAL_LEFT,
	HORIZONTAL_CENTER,
	HORIZONTAL_RIGHT
};

enum VerticalAlignment {
	VERTICAL_TOP,
	VERTICAL_CENTER,
	VERTICAL_BOTTOM
};

enum Anchor {
	ANCHOR_TOPLEFT,
	ANCHOR_CENTERLEFT,
	ANCHOR_BOTTOMLEFT,
	ANCHOR_TOPMIDDLE,
	ANCHOR_CENTER,
	ANCHOR_BOTTOMMIDDLE,
	ANCHOR_TOPRIGHT,
	ANCHOR_CENTERRIGHT,
	ANCHOR_BOTTOMRIGHT
};

enum EffectType {
	EFFECT_NONE,
	EFFECT_FLASHDOWN,
	EFFECT_READY,
	EFFECT_FAINT,
	EFFECT_HEAL,
	EFFECT_POISON,
	EFFECT_MIRROR,
	EFFECT_SPHERE,
	EFFECT_THINK,
	EFFECT_SOULFLY,
	EFFECT_SOULRETURN,
	EFFECT_SOULSHINE,
	EFFECT_DESTROY,
	EFFECT_LEVEL,
};

enum EMOTE {
	EMOTE_STANDBY = 0,
	EMOTE_WALK,
	EMOTE_ATTACK1,
	EMOTE_WOUND,
	EMOTE_FAINT,
	EMOTE_LIE,
	EMOTE_WAVE,
	EMOTE_KNEEL,
	EMOTE_SAD,
	EMOTE_ANGRY,
	EMOTE_SIT,
	EMOTE_DEFEND,
	EMOTE_LAUGH,
	EMOTE_BOW,
	EMOTE_ATTACKSPECIAL,
	EMOTE_CAST,
	EMOTE_FILL,
	EMOTE_ATTACK2,
	EMOTE_ATTACK3,
};

//Battle Info
const DWORD	_IDMSK_PET	   = 0x80000000;
const DWORD _IDMSK_MONSTER = 0xC0000000;
const DWORD _IDMSK_INVITEM = 100000000;
const DWORD _IDMSK_PETITEM = 1000000000;

enum { OBJ_NONE = 1234, OBJ_MONSTER, OBJ_PET, OBJ_USER, OBJ_ITEM };

enum { OBJTYPE_MONSTER = 0, OBJTYPE_VSPET, OBJTYPE_VSPLAYER, OBJTYPE_FRIENDPET, OBJTYPE_FRIENDPLAYER };

class Texture;
using Asset = std::shared_ptr<Texture>;

struct ColorShift {
	BYTE hue;
	BYTE newHue;
	BYTE range;
	BYTE sat;
	BYTE bright;
};
using ColorShifts = std::vector<ColorShift>;

class Sprite;
struct Effect {
	int type;
	Sprite* sprite = nullptr;
};
using EffectItr = std::vector<Effect>::iterator;

//Fonts
#define ARIAL		"font\\Arial.ttf"
#define ARIALUNI	"font\\ArialUni.ttf"