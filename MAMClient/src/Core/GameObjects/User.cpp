#include "stdafx.h"
#include "User.h"
#include "GameMap.h"

#include "Global.h"
#include "Define.h"
#include "CustomEvents.h"
#include "MainWindow.h"
#include "INI.h"

#include "Team.h"

#include "pUserInfo.h"

User::User(pUserInfo *packet):Entity(mainForm->renderer, packet->userId, packet->name, packet->look) {
	NickName = packet->nickName;
	Spouse = packet->spouse;
	SetCoord(SDL_Point{ packet->x, packet->y });
	setDirection(packet->direction);
	setAnimation(StandBy); //TEMP - Does the packet define the animation?

	Level = packet->level;
	Rank = packet->rank;
	Reborns = packet->reborns;
	RankType = packet->rankType; //Needed?
	MasterRank = packet->masterType;
	Alignment = packet->alignment;
	FullRank = (MasterRank * 10000) + (Rank * 1000) + (Reborns * 10) + Alignment;
	
	pkEnabled = packet->pkEnabled;
	SyndicateId = packet->syndicateId;
	SubGroudId = packet->subGroudId;
	SyndicateRank = packet->syndicateRank;
	Guild = packet->guild;
	SubGroup = packet->subGroup;
	GuildTitle = packet->guildTitle;
	

	for (int i = 0; i < 25; i += 5) {
		ColorShift shift;
		memcpy(&shift, packet->colorSets + i, 5);
		colorShifts.push_back(shift);
	}
	//memcpy(colorSets, packet->colorSets, 25);

	switch (packet->emotion) {
	case uieNone:
		Animation = StandBy;
		break;
	case uieWave:
		Animation = SayHello;
		break;
	case uieFaint:
		Animation = Faint;
		break;
	case uieSit:
		Animation = SitDown;
		break;
	case uieKneel:
		Animation = Genuflect;
		break;
	case uieHappy:
		Animation = Laugh;
		break;
	case uieBow:
		Animation = Politeness;
		break;
	case uieFly:
		Animation = SitDown;
		SetFlying(true);
		break;
	default:
		Animation = StandBy;
	}

	loadAura();
}

User::User(SDL_Renderer* r, int id, std::string name, int look):Entity(r, id, name, look) {
	setAnimation(StandBy);
}

User::~User() {
	if (Aura) delete Aura;
	if (TeamLeader) delete TeamLeader;
}

void User::render() {
	if (!map) return;

	//Render cloud before player
	if (Flying || Ascending || Descending) {
		if (Ascending) {
			int elapsed = SDL_GetTicks() - CloudInitTime;
			double dist = elapsed * CLOUD_HEIGHT / CLOUD_LIFT_MS;
			if (elapsed >= CLOUD_LIFT_MS) {
				dist = CLOUD_HEIGHT;
				Ascending = false;
				Flying = true;
				LoadCloud(CLOUD_FLY);
			}

			if (Cloud->completed) {
				LoadCloud(CLOUD_FLY);
			}

			SpriteOffset.y = -dist;
		}
		else if (Descending) {
			int elapsed = SDL_GetTicks() - CloudInitTime;
			double dist = CLOUD_HEIGHT - (elapsed * CLOUD_HEIGHT / CLOUD_LIFT_MS);
			if (elapsed >= CLOUD_LIFT_MS) {
				dist = 0;
				Descending = false;
				Flying = false;
				RemoveCloud();
				setAnimation(StandBy);
				loadSprite();
			}

			if (Cloud && Cloud->completed) {
				RemoveCloud();
			}

			SpriteOffset.y = -dist;
		}
		else {
			int elapsed = SDL_GetTicks() - CloudBobTime;
			if (elapsed >= BOB_MS) {
				if (BOB_UP) {
					if (abs(SpriteOffset.y) - CLOUD_HEIGHT < MAX_BOB) SpriteOffset.y -= 1;
					else {
						SpriteOffset.y = -(CLOUD_HEIGHT + MAX_BOB);
						BOB_UP = false;
					}
				}
				else {
					if (CLOUD_HEIGHT - abs(SpriteOffset.y) < MAX_BOB) SpriteOffset.y += 1;
					else {
						SpriteOffset.y = -(CLOUD_HEIGHT - MAX_BOB);
						BOB_UP = true;
					}
				}
				CloudBobTime = SDL_GetTicks();
			}
		}

		SDL_Point ShadowPos = RenderPos;
		ShadowPos.y -= SpriteOffset.y;
		if (CloudShadow.get()) CloudShadow->Render(ShadowPos);

		if (Cloud) Cloud->render(RenderPos.x, RenderPos.y);
	}

	Entity::render();

	if (Aura) {
		Aura->render(RenderPos.x, RenderPos.y);
	}

	if (team && TeamLeader) {
		TeamLeader->setLocation(RenderPos.x, RenderPos.y - 87 + 8);
		TeamLeader->speed = 500;
		TeamLeader->render();
	}

	render_nameplate();
}


void User::step() {
	if (jumping) {
		if (timeGetTime() - lastMoveTick >= JUMP_SPEED) {
			jumping = false;
			setAnimation(StandBy);
			loadSprite();
		}
	}

	if (walking) {
		MoveAlongPath();
	}

	//When in a team, follow the leader
	if (!leaveMap() && team && !IsTeamLeader()) {
		User *nextUser = team->GetNextInLine(this);
		if (nextUser) {
			SDL_Point backCoord = getBackCoord(nextUser->GetCoord(), nextUser->getDirection());
			if ((Coord.x != backCoord.x || Coord.y != backCoord.y) && map->isCoordWalkable(backCoord)) {
				auto destCoord = GetDestCoord();
				if (backCoord.x != destCoord.x || backCoord.y != destCoord.y) User::walkTo(backCoord);
			}
		}
	}
}

void User::handleEvent(SDL_Event& e) {
	Entity::handleEvent(e);

	if (!sprite) return;

	if (e.type == SDL_MOUSEBUTTONDOWN) {
		if (e.button.button == SDL_BUTTON_RIGHT && MouseOver) {
			if (GameMode == GAMEMODE_SELECTTEAM) GameMode = GAMEMODE_JOINTEAM;
			SDL_Event interact;
			SDL_zero(interact);
			interact.type = CUSTOMEVENT_USER;
			interact.user.code = USER_RIGHTCLICK;
			interact.user.data1 = this;
			interact.user.data2 = nullptr;
			SDL_PushEvent(&interact);
		}
	}
}

void User::jumpTo(SDL_Point coord) {
	if (walking) {
		walking = false;
		path.clear();
	}

	jumping = true;

	setDirectionToCoord(coord);
	SetCoord(coord);
	setAnimation(Genuflect);
	loadSprite();
	addEffect(EFFECT_FLASHDOWN);
	lastMoveTick = timeGetTime();
}

void User::walkTo(SDL_Point coord) {
	std::vector<SDL_Point> newPath = map->getPath(path.size() > 0 ? path[0] : Coord, coord, (Flying || Ascending) && !Descending);
	followPath(newPath);
}

void User::followPath(std::vector<SDL_Point> newPath)
{
	if (jumping) {
		jumping = false;
	}

	if (newPath.size() == 0) {
		if (walking) {
			if (path.size() > 1) path.erase(path.begin() + 1, path.end());
		}
		return;
	}

	//Start new path from next coordinate destination in the existing path
	if (path.size() > 0) {
		auto front = path[0];
		path = newPath;
		path.insert(path.begin(), front);
	}
	else {
		path = newPath;
	}

	//Only reset last move timestamp when we are not moving, otherwise the character will boomerang
	if (!walking) lastMoveTick = timeGetTime();
	walking = true;

	if (!Flying) setAnimation(Walk);
	setDirectionToCoord(path[0]);
	loadSprite();
}

bool User::AtDestCoord() {
	auto destCoord = GetDestCoord();
	if (Coord.x == destCoord.x && Coord.y == destCoord.y) return true;
	return false;
}

SDL_Point User::GetDestCoord() {
	if (!path.size()) return { 0, 0 };
	auto destCoord = path[0];
	//path.erase(path.begin());
	//lastMoveTick = timeGetTime();

	//If the next Coord is a map portal, end path traversal on portal
	if (map->isCoordAPortal(destCoord)) {
		clearPath();
	}
	return destCoord;
}

void User::MoveAlongPath() 
{
	auto nextCoord = path[0];
	auto basePos = map->CenterOfCoord(Coord);
	auto destPos = map->CenterOfCoord(nextCoord);
	float moveSpeed = Flying ? FLY_SPEED : WALK_SPEED;
	
	int systemTime = timeGetTime();
	int timeElapsed = systemTime - lastMoveTick;
	double movePerc = timeElapsed * 1.0 / moveSpeed;
	if (movePerc > 1.0) movePerc = 1.0;

	double shiftx = (destPos.x - basePos.x) * movePerc;
	double shifty = (destPos.y - basePos.y) * movePerc;
	Position.x = basePos.x + shiftx;
	Position.y = basePos.y + shifty;
	if (Position.x == destPos.x && Position.y == destPos.y)
	{
		lastMoveTick = systemTime;
		path.erase(path.begin());
		SetCoord(nextCoord);

		if (path.size() > 0) {			
			//Turn character towards the next coord in the path
			int dir = map->getDirectionToCoord(Coord, path[0]);
			if (dir != Direction)
			{
				setDirection(dir);
				loadSprite();
			}
		}
		else {
			walking = false;
			if (!Flying) setAnimation(StandBy);
			loadSprite();
		}		
	}
}

void User::clearPath() {
	path.clear();
}

void User::setLeaving(bool leaving) {
	isLeavingMap = leaving;
}

bool User::leaveMap() {
	if (walking || jumping) return false;
	if (!isLeavingMap) return false;
	return true;
}

bool User::getJumping() {
	return jumping;
}

bool User::getWalking() {
	return walking;
}

void User::setDirection(int direction) {
	int curDir = Direction;
	Entity::setDirection(direction);
	if (curDir == Direction) return;
	if (Flying) {
		int mode = CLOUD_FLY;
		if (Ascending) mode = CLOUD_LIFT;
		else if (Descending) mode = CLOUD_LAND;
		LoadCloud(mode);
	}
}

//Load Aura based on Rank
void User::loadAura() {
	if (Aura) delete Aura;

	if (Rank < 2) return;

	std::string rankEffect;
	if (Alignment == 1) rankEffect = "God";
	else rankEffect = "Devil";
	rankEffect += std::to_string(Rank);

	INI iniLeader("ini/common.ini", rankEffect);

	std::string buffer;
	if (iniLeader.getEntry("FrameAmount", &buffer)) {
		std::vector<std::string> vStrings;
		int length = stoi(buffer);
		for (int i = 0; i < length; i++) {
			std::string entry = "Frame" + std::to_string(i);
			std::string result = iniLeader.getEntry(entry);
			if (result.size()) {
				result = "data/" + result;
				vStrings.push_back(result);
			}
		}

		if (vStrings.size()) {
			Aura = new Sprite(renderer, vStrings, stEffect);
			Aura->setFrameInterval(55);
			Aura->repeatMode = 1;
			Aura->SetLoopTimer(7000); //repeat every 7 seconds
			Aura->RandomizeTimerDelay();
		}
	}
}

void User::loadTeamLead() {
	if (TeamLeader) return;

	INI iniLeader("ini/common.ini", "Leader");
	
	std::string buffer;
	if (iniLeader.getEntry("FrameAmount", &buffer)) {
		std::vector<std::string> vStrings;
		int length = stoi(buffer);
		for (int i = 0; i < length; i++) {
			std::string entry = "Frame" + std::to_string(i);
			std::string result = iniLeader.getEntry(entry);
			if (result.size()) {
				result = "data" + result.substr(1, std::string::npos);
				vStrings.push_back(result);
			}
		}

		if (vStrings.size()) {
			TeamLeader = new Sprite(renderer, vStrings, stEffect);
			TeamLeader->repeatMode = 0;
			TeamLeader->start();
		}
	}
}

std::string User::getNickName() {
	return NickName;
}

std::string User::getSpouse() { 
	return Spouse;
}

std::string User::GetRankText() {
	std::string rankText;

	switch (Rank) {
	case 0: //Mortal
		if (Level > 1100) rankText = "Senior Master";
		else if (Level > 900) rankText = "Junior Master";
		else if (Level > 750) rankText = "Senior Knight";
		else if (Level > 500) rankText = "Junior Knight";
		else if (Level > 300) rankText = "Basic Knight";
		else if (Level > 150) rankText = "Senior Warrior";
		else if (Level > 50) rankText = "Junior Warrior";
		else rankText = "Basic Warrior";
		break;
	case 1: 
		rankText = "Basic God";
		break;
	case 2:
		rankText = "Junior "; 
		break;
	case 3:
		rankText = "Senior ";
		break;
	case 4:
		rankText = "Super ";
		break;
	case 5:
		rankText = "Master ";
		switch (MasterRank) {
		case 1:
			rankText += "Pet Raising ";
			break;
		case 2:
			rankText += "Cultivation ";
			break;
		case 3:
			rankText += "Virtue ";
			break;
		case 4:
			rankText += "Wuxing ";
			break;
		case 5:
			rankText += "Kungfu ";
			break;
		case 6:
			rankText += "Thievery ";
			break;
		case 7:
			rankText += "Reputation ";
			break;
		}
		break;
	}

	if (Alignment > 0) {
		if (Alignment == 1) rankText += "God";
		if (Alignment == 2) rankText += "Devil";
	}

	if (Reborns > 0) {
		rankText += " [" + std::to_string(Reborns) + "]";
	}

	return rankText;
}

std::string User::getGuild() {
	return Guild;
}

std::string User::getGuildTitle() {
	return GuildTitle;
}

void User::CreateTeam() {
	team = new CTeam();
	team->AddMember(this);
	loadTeamLead();
}

void User::JoinTeam(CTeam* pTeam) {
	team = pTeam;
	team->AddMember(this);
}

void User::LeaveTeam() {
	if (!team) return;

	if (team->GetLeader() == this) {
		while (team->GetMemberCount() >= 2) {
			User* member = team->GetMember(2);
			if (member && member != this) {
				member->LeaveTeam();
			}
		}
		delete team;
		if (TeamLeader) delete TeamLeader;
		TeamLeader = nullptr;
	}
	else team->RemoveMember(this);
	team = nullptr;
}

bool User::IsTeamLeader() {
	if (!team) return false;
	if (team->GetLeader() == this) return true;
	return false;
}

void User::TakeOff() {
	Ascending = true; 
	Descending = false;
	Flying = true;
	LoadCloud(CLOUD_LIFT);
	CloudInitTime = SDL_GetTicks();
	setAnimation(SitDown);
	loadSprite();
}

void User::Land() {
	Ascending = false;
	Descending = true;
	Flying = false;
	//SpriteOffset.y = 0;
	CloudInitTime = SDL_GetTicks();
	LoadCloud(CLOUD_LAND);
}

void User::LoadCloud(int mode) {
	std::string cloudPath = "data/effect/cloud/";

	std::string finalPath;
	if (Alignment == 2) finalPath = cloudPath + "black/";
	else finalPath = cloudPath + "white/";

	if (!CloudShadow.get()) {
		std::string shadowPath = cloudPath + "shadow.tga";
		CloudShadow.reset(new Texture(renderer, shadowPath));
		int w = CloudShadow->width / 2;
		//double h = CloudShadow->height * 0.75;
		int h = CloudShadow->height / 2;
		CloudShadow->setPosition(SDL_Point{ -w,-(int)h });
	}

	std::vector<std::string> files;
	char buffer[16];
	if (mode == CLOUD_LIFT || mode == CLOUD_LAND) {
		for (int i = 0; i < 16; i++) {
			std::sprintf(buffer, "start%04d.tga", i);
			files.push_back(finalPath + buffer);
		}
		if (mode == CLOUD_LAND) std::reverse(files.begin(), files.end());
	}
	else {
		if (IsMaster()) {
			finalPath = cloudPath + "color/";
		}

		int dir = Direction + 1;
		if (dir >= 8) dir -= 8;
		int offset = (8 * dir);
		for (int i = 0; i < 8; i++) {
			std::sprintf(buffer, "cloud%04d.tga", i + offset);
			files.push_back(finalPath + buffer);
		}
	}

	if (Cloud) delete Cloud;
	Cloud = nullptr;
	Cloud = new Sprite(renderer, files, stCloud);
	Cloud->setFrameInterval(50);
	if (Ascending || Descending) Cloud->repeatMode = 1;
	else Cloud->repeatMode = 0;
	Cloud->start();
}

void User::RemoveCloud() {
	CloudShadow.reset();
	if (Cloud) delete Cloud;
	Cloud = nullptr;
}