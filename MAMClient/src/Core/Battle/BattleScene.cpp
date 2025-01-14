#include "stdafx.h"
#include "BattleScene.h"
#include "Battle.h"

#include "Define.h"
#include "CustomEvents.h"
#include "Fighter.h"
#include "Player.h"
#include "Pet.h"

BattleScene::BattleScene(std::vector<Fighter*> allies, std::vector<Fighter*> enemies) {
	cast = enemies;
	cast.insert(cast.end(), allies.begin(), allies.end());
}


BattleScene::~BattleScene() {
	for (auto actor : actors) {
		delete actor;
	}
}


void BattleScene::setReactor(Fighter* target, int action) {
	reactor = target;
	reaction = action;
}


bsPerform* BattleScene::addAction(Fighter* source, bsType type, AnimType action, SDL_Point target, std::vector<std::string> floatingLabels) {
	bsPerform* performance = new bsPerform;
	performance->actor = source;
	performance->type = type;
	performance->animation = action;
	performance->targetPoint = target;
	if (type == bsMoveTo) performance->direction = getDirection(source->GetPosition(), target);
	else performance->direction = source->getDirection();
	performance->floatingLabels = floatingLabels;
	
	//Check if actor exists, and add if not
	bsActor* actor = nullptr;
	for (auto nextActor : actors) {
		if (nextActor->actor == source) {
			actor = nextActor;
			break;
		}
	}
	if (!actor) {
		actor = new bsActor;
		actor->actor = source;
		actors.push_back(actor);
	}

	//Now that we have an actor, add the performance to the queue
	actor->performances.push(performance);
	return performance;
}


bsPerform* BattleScene::addAction(Fighter* source, bsType type, AnimType action, SDL_Point target) {
	bsPerform* performance = new bsPerform;
	performance->actor = source;
	performance->type = type;
	performance->animation = action;
	performance->targetPoint = target;
	if (type == bsMoveTo) performance->direction = getDirection(source->GetPosition(), target);
	else performance->direction = source->getDirection();
	//performance->battleYell = bYell;

	//Check if actor exists, and add if not
	bsActor* actor = nullptr;
	for (auto nextActor : actors) {
		if (nextActor->actor == source) {
			actor = nextActor;
			break;
		}
	}
	if (!actor) {
		actor = new bsActor;
		actor->actor = source;
		actors.push_back(actor);
	}

	//Now that we have an actor, add the performance to the queue
	actor->performances.push(performance);
	return performance;
}


void BattleScene::start() {
	started = true;
}


bool BattleScene::isStarted() {
	return started;
}


bool BattleScene::completed() {
	return finished;
}


void BattleScene::step() {
	if (!started || finished) return;

	for (auto actor : actors) perform(actor);

	while (newActors.size() > 0) {
		bsActor* newActor = newActors.front();
		actors.push_back(newActor);
		newActors.erase(newActors.begin());
	}

	std::vector<bsActor*>::iterator itr;
	for(itr = actors.begin(); itr != actors.end();)
	{
		bsActor* actor = *itr;
		if(actor->performances.size() == 0)
		{
			itr = actors.erase(itr);
		}
		else ++itr;
	}
	if (actors.size() == 0) {
		finished = true;
		updateAudience(true);
	}
}

void BattleScene::perform(bsActor* actor) {
	
	bsPerform* perform = actor->performances.front();
	bool performanceComplete = false;

	switch (perform->type) {
	case bsMoveTo:
		performanceComplete = perform_moveTo(perform);
		break;
	case bsFadeTo:
		performanceComplete = perform_fadeTo(perform);
		break;
	case bsStandby:
		performanceComplete = perform_standBy(perform);
		break;
	case bsAction:
		performanceComplete = perform_action(perform);
		break;
	case bsReaction:
		performanceComplete = perform_reaction(perform);
		break;
	case bsFaint:
		performanceComplete = perform_faint(perform);
		break;
	case bsUseItem:
		performanceComplete = perform_useItem(perform);
		break;
		
	case bsCaptureBegin:
		performanceComplete = perform_captureBegin(perform);
		break;
	case bsCaptureDrag:
		performanceComplete = perform_captureDrag(perform);
		break;
	case bsCaptureSuccess:
		performanceComplete = perform_captureSuccess(perform);
		break;
	case bsCaptureAngry:
		performanceComplete = perform_captureAngry(perform);
		break;
	case bsCaptureFail:
		performanceComplete = perform_captureFail(perform);
		break;

	case bsVictory:
		performanceComplete = perform_victory(perform);
		break;
	}

	if (performanceComplete) {
		if (perform->boundEvent.type != 0) {
			if (perform->boundEvent.type == CUSTOMEVENT_PLAYER) {
				if (perform->boundEvent.user.code == PLAYER_LIFE) {
					if (perform->boundEvent.user.data1) {
						int life = *(int*)perform->boundEvent.user.data1;
						delete perform->boundEvent.user.data1;
						player->SetLife(player->GetCurrentLife() + life);
					}
				}
			}

			if (perform->boundEvent.type == CUSTOMEVENT_PET) {
				if (perform->boundEvent.user.code == PET_LIFE) {
					if (perform->boundEvent.user.data1) {
						int life = *(int*)perform->boundEvent.user.data1;
						delete perform->boundEvent.user.data1;
						Pet* pet = player->getActivePet();
						if (pet) pet->SetLife(pet->GetCurrentLife() + life);
					}
				}
			}

			SDL_PushEvent(&perform->boundEvent);
		}

		actor->performances.pop();
		delete perform;
	}
}

bool BattleScene::perform_moveTo(bsPerform* perform) {
	Fighter* actor = perform->actor;
	SDL_Point target = perform->targetPoint;

	if (!perform->started) {
		perform->started = true;
		perform->startTime = SDL_GetTicks();
		actor->setAnimation(Walk);
		actor->setDirection(getDirection(actor->GetPosition(), target));
		actor->loadSprite();
	}

	float dtMove = 30.0;
	float distance = sqrtf( powf((target.x - actor->GetPosition().x),2) + powf((target.y - actor->GetPosition().y), 2));
	float dtRem = distance - dtMove;

	SDL_Point battlePos = actor->GetPosition();
	if (actor->GetPosition().x != target.x || actor->GetPosition().y != target.y) {
		if (dtMove < distance) {
			battlePos.x = ((dtMove * target.x) + (dtRem * actor->GetPosition().x)) / distance;
			battlePos.y = ((dtMove * target.y) + (dtRem * actor->GetPosition().y)) / distance;
		}
		else {
			battlePos.x = target.x;
			battlePos.y = target.y;
		}
		actor->SetPosition(battlePos);
		actor->GetSprite()->setLocation(actor->GetPosition().x, actor->GetPosition().y);
	}
	else return true;

	return false;
}


bool BattleScene::perform_fadeTo(bsPerform* perform) {
	Fighter* actor = perform->actor;
	SDL_Point target = perform->targetPoint;

	if (!perform->started) {
		perform->started = true;
		perform->startTime = SDL_GetTicks();
		perform->startPoint = { actor->GetPosition().x, actor->GetPosition().y };
		actor->setDirection(getDirection(actor->GetPosition(), target));
		actor->setAnimation(Walk);
		actor->loadSprite();
	}

	float dtMove = 10.0;
	float origDistance = sqrtf(powf((target.x - perform->startPoint.x), 2) + powf((target.y - perform->startPoint.y), 2));
	float distance = sqrtf(powf((target.x - actor->GetPosition().x), 2) + powf((target.y - actor->GetPosition().y), 2));
	
	float falpha = (distance / origDistance) * 255.0;
	BYTE alpha = (BYTE)falpha;
	if (actor->findEffect(EFFECT_SPHERE)) actor->findEffect(EFFECT_SPHERE)->setAlpha(alpha);
	else actor->GetSprite()->setAlpha(alpha);
	
	float dtRem = distance - dtMove;
	SDL_Point battlePos = actor->GetPosition();
	if (battlePos.x != target.x || battlePos.y != target.y) {
		if (dtMove < distance) {
			battlePos.x = ((dtMove * target.x) + (dtRem * battlePos.x)) / distance;
			battlePos.y = ((dtMove * target.y) + (dtRem * battlePos.y)) / distance;
		}
		else {
			battlePos.x = target.x;
			battlePos.y = target.y;
		}
		actor->SetPosition(battlePos);
		actor->GetSprite()->setLocation(actor->GetPosition().x, actor->GetPosition().y);
	}
	else {
		if (actor->findEffect(EFFECT_SPHERE)) actor->findEffect(EFFECT_SPHERE)->setAlpha(0xFF);
		else actor->GetSprite()->setAlpha(0xFF);
		return true;
	}

	return false;
}


bool BattleScene::perform_standBy(bsPerform* perform) {
	Fighter* actor = perform->actor;
	actor->setAnimation(StandBy);
	if (actor->IsEnemy()) actor->setDirection(default_enemy_dir);
	else actor->setDirection(default_ally_dir);
	actor->loadSprite();
	return true;
}


bool BattleScene::perform_action(bsPerform* perform) {
	Fighter* actor = perform->actor;
	SDL_Point target = perform->targetPoint;
	if (!perform->started) {
		perform->started = true;
		perform->startTime = SDL_GetTicks();
		actor->setAnimation(perform->animation);
		if (actor->IsEnemy()) actor->setDirection(default_enemy_dir);
		else actor->setDirection(default_ally_dir);
		actor->loadSprite();
		actor->GetSprite()->setLocation(actor->GetPosition().x, actor->GetPosition().y);
		actor->GetSprite()->repeatMode = 1;
		actor->GetSprite()->start();

		while (perform->floatingLabels.size() > 0) {
			reactor->addFloatingLabel(perform->floatingLabels[0]);
			perform->floatingLabels.erase(perform->floatingLabels.begin());
		}

		startReaction();

		lookTo = target;
		updateAudience();
	}

	if (actor->GetSprite()->repeatCount >= 1) return true;

	return false;
}


bool BattleScene::perform_reaction(bsPerform* perform) {
	Fighter* actor = perform->actor;
	
	if (!perform->started) {
		perform->started = true;
		perform->startTime = SDL_GetTicks();
		actor->setAnimation(perform->animation);
		actor->loadSprite();
	}

	int elapsedTime = SDL_GetTicks() - perform->startTime;
	if (elapsedTime >= perform->playTime) return true;

	return false;
}


bool BattleScene::perform_faint(bsPerform* perform) {
	Fighter* actor = perform->actor;

	if (!perform->started) {
		perform->started = true;
		perform->startTime = SDL_GetTicks();
		actor->setAnimation(perform->animation);
		actor->loadSprite();
		actor->GetSprite()->repeatMode = 1;
		actor->GetSprite()->start();
		actor->SetAlive(false);
	}

	int elapsedTime = SDL_GetTicks() - perform->startTime;
	if (actor->GetSprite()->repeatCount >= 1) {
		actor->setAnimation(Lie);
		actor->loadSprite();
		actor->addEffect(EFFECT_FAINT);
		return true;
	}

	return false;
}


bool BattleScene::perform_captureBegin(bsPerform* perform) {
	Fighter* actor = perform->actor;

	if (!perform->started) {
		perform->started = true;
		perform->startTime = SDL_GetTicks();

		actor->setAnimation(StandBy);
		actor->setDirection(1);
		actor->loadSprite();
		actor->addEffect(EFFECT_MIRROR);

		reactor->setAnimation(Cast);
		reactor->loadSprite();
		reactor->GetSprite()->repeatMode = 2;
		reactor->GetSprite()->start();
	}

	int currentTicks = (SDL_GetTicks() - perform->startTime);
	int currentSecs = currentTicks / 1000;

	//spin in a circle with mirror above
	if (currentSecs < 1) {
		int intervalSize = 1000 / 24; //two rotations
		int newDir = default_enemy_dir + (currentTicks / intervalSize);
		while (newDir > 7) newDir -= 8;
		if (actor->getDirection() != newDir) {
			actor->setDirection(newDir);
			actor->loadSprite();
		}
	}
	else return true;

	return false;
}


bool BattleScene::perform_captureDrag(bsPerform* perform) {
	Fighter* player = reactor;
	Fighter* actor = perform->actor;

	if (!perform->started) {
		perform->started = true;
		perform->startTime = SDL_GetTicks();

		actor->setAnimation(StandBy);
		actor->setDirection(1);
		actor->loadSprite();
		actor->addEffect(EFFECT_SPHERE);
		actor->SetVisible(false);

		reactor->setAnimation(Cast);
		reactor->loadSprite();
		reactor->GetSprite()->repeatMode = 2;
		reactor->GetSprite()->speed = 500;
		reactor->GetSprite()->start();
	}

	int currentTicks = (SDL_GetTicks() - perform->startTime);
	int currentSecs = currentTicks / 1000;

	//Turn into a sphere and Drag the sphere to mid
	if (currentSecs == 0) {
		SDL_Point target;
		target.x = ((player->GetBattleBasePos().x - actor->GetBattleBasePos().x) / 2) + actor->GetBattleBasePos().x;
		target.y = ((player->GetBattleBasePos().y - actor->GetBattleBasePos().y) / 2) + actor->GetBattleBasePos().y;
		float distance = sqrtf(powf((target.x - actor->GetPosition().x), 2) + powf((target.y - actor->GetPosition().y), 2));

		float dtMove = 5.0;
		float speed = 6.0;
		float dtRem = distance - dtMove;

		SDL_Point battlePos = actor->GetPosition();
		if (battlePos.x != target.x || battlePos.y != target.y) {
			//Move the orb to the middle
			if (dtMove < distance) {
				//actor->pos.x = (int)(((dtMove * target.x) + (dtRem * actor->pos.x)) / distance + 0.5);
				//actor->pos.y = (int)(((dtMove * target.y) + (dtRem * actor->pos.y)) / distance + 0.5);

				float dx = target.x - battlePos.x;
				float dy = target.y - battlePos.y;
				float length = sqrtf(dx*dx + dy*dy);
				dx /= length;
				dy /= length;
				dx *= speed;
				dy *= speed;
				battlePos.x += (int)(dx + 0.5);
				battlePos.y += (int)(dy + 0.5);
			}
			else {
				battlePos.x = target.x;
				battlePos.y = target.y;
			}
			actor->SetPosition(battlePos);

			actor->GetSprite()->setLocation(actor->GetPosition().x, actor->GetPosition().y);
		}
	}

	//Vibrate to resist capture
	if (currentSecs == 1) {
		SDL_Point midPoint;
		midPoint.x = ((player->GetBattleBasePos().x - actor->GetBattleBasePos().x) / 2) + actor->GetBattleBasePos().x;
		midPoint.y = ((player->GetBattleBasePos().y - actor->GetBattleBasePos().y) / 2) + actor->GetBattleBasePos().y;
		float midPoint_distance = sqrtf(powf((player->GetBattleBasePos().x - midPoint.x), 2) + powf((player->GetBattleBasePos().y - midPoint.y), 2));
		float distance = sqrtf(powf((player->GetBattleBasePos().x - actor->GetPosition().x), 2) + powf((player->GetBattleBasePos().y - actor->GetPosition().y), 2));

		float maxMove = 30.0;

		float dtMove = (rand() % 3) + 1.0;
		int moveDir = rand() % 2;
		if (moveDir == 1) dtMove = dtMove;
		else dtMove = -dtMove;

		float dtRem = distance - dtMove;
		SDL_Point battlePos;
		battlePos.x = ((dtMove * player->GetBattleBasePos().x) + (dtRem * midPoint.x)) / distance;
		battlePos.y = ((dtMove * player->GetBattleBasePos().y) + (dtRem * midPoint.y)) / distance;
		actor->SetPosition(battlePos);
		actor->GetSprite()->setLocation(actor->GetPosition().x, actor->GetPosition().y);
	}

	//
	if (currentSecs >= 3) {
		return true;
	}

	return false;
}


bool BattleScene::perform_captureSuccess(bsPerform* perform) {
	Fighter* player = reactor;
	Fighter* actor = perform->actor;
	SDL_Point target = perform->targetPoint;

	if (!perform->started) {
		perform->started = true;
		perform->startTime = SDL_GetTicks();
		actor->SetAlive(false);
		EffectItr itr = actor->removeEffect(EFFECT_MIRROR);
	}	

	SDL_Point midPoint;
	midPoint.x = ((player->GetBattleBasePos().x - actor->GetBattleBasePos().x) / 2) + actor->GetBattleBasePos().x;
	midPoint.y = ((player->GetBattleBasePos().y - actor->GetBattleBasePos().y) / 2) + actor->GetBattleBasePos().y;
	float midPointDistance = sqrtf(powf((target.x - midPoint.x), 2) + powf((target.y - midPoint.y), 2));

	float dtMove = 2.5;
	float speed = 6.0;
	float distance = sqrtf(powf((target.x - actor->GetPosition().x), 2) + powf((target.y - actor->GetPosition().y), 2));
	float dtRem = distance - dtMove;

	float alphaPerc = distance / midPointDistance;
	BYTE alpha = (BYTE)((float)alphaPerc * 255.0);
	if (actor->findEffect(EFFECT_SPHERE)) actor->findEffect(EFFECT_SPHERE)->setAlpha(alpha);

	SDL_Point battlePos = actor->GetPosition();
	if (battlePos.x != target.x || battlePos.y != target.y) {
		if (dtMove < distance) {
			//actor->pos.x = (int)(((dtMove * target.x) + (dtRem * actor->pos.x)) / distance + 0.5);
			//actor->pos.y = (int)(((dtMove * target.y) + (dtRem * actor->pos.y)) / distance + 0.5);

			float dx = target.x - battlePos.x;
			float dy = target.y - battlePos.y;
			float length = sqrtf(dx*dx + dy*dy);
			dx /= length;
			dy /= length;
			dx *= speed;
			dy *= speed;
			battlePos.x += (int)(dx + 0.5);
			battlePos.y += (int)(dy + 0.5);
		}
		else {
			battlePos.x = target.x;
			battlePos.y = target.y;
		}

		actor->SetPosition(battlePos);
		actor->GetSprite()->setLocation(battlePos.x, battlePos.y);
	}
	else {
		EffectItr itr = actor->removeEffect(EFFECT_SPHERE);
		player->setAnimation(StandBy);
		player->loadSprite();
		return true;
	}

	return false;
}


bool BattleScene::perform_captureAngry(bsPerform* perform) {
	Fighter* actor = perform->actor;

	if (!perform->started) {
		perform->started = true;
		perform->startTime = SDL_GetTicks();
		actor->setAnimation(perform->animation);
		actor->loadSprite();
		actor->SetVisible(true);
		EffectItr itr = actor->removeEffect(EFFECT_SPHERE);

		reactor->setAnimation(StandBy);
		reactor->loadSprite();
	}

	int currentTicks = (SDL_GetTicks() - perform->startTime);
	int currentSecs = currentTicks / 1000;

	//Spin in reverse
	if (currentSecs == 0) {
		int intervalSize = 1000 / 24; //two rotations
		int newDir = default_enemy_dir - (currentTicks / intervalSize);
		while (newDir < 0) newDir += 8;
		if (actor->getDirection() != newDir) {
			actor->setDirection(newDir);
			actor->loadSprite();
			actor->GetSprite()->setLocation(actor->GetPosition().x, actor->GetPosition().y);
		}
	}

	if (currentSecs == 1) {
		EffectItr itr = actor->removeEffect(EFFECT_MIRROR);
		actor->SetBattleYell(perform->battleYell);
		actor->setDirection(default_enemy_dir);
		actor->loadSprite();
	}

	if (currentSecs >= 2) {
		actor->SetBattleYell("");
		reactor->setAnimation(StandBy);
		reactor->loadSprite();
		return true;
	}

	return false;
}


bool BattleScene::perform_captureFail(bsPerform* perform) {
	Fighter* actor = perform->actor;
	SDL_Point target = perform->targetPoint;

	if (!perform->started) {
		perform->started = true;
		perform->startTime = SDL_GetTicks();
		actor->setAnimation(Walk);
		actor->setDirection(getDirection(actor->GetPosition(), target));
		actor->loadSprite();
		EffectItr itr = actor->removeEffect(EFFECT_SPHERE);
		EffectItr itr2 = actor->removeEffect(EFFECT_MIRROR);
	}

	SDL_Point battlePos = actor->GetPosition();
	float dtMove = 20.0;
	float distance = sqrtf(powf((target.x - battlePos.x), 2) + powf((target.y - battlePos.y), 2));
	float dtRem = distance - dtMove;

	if (battlePos.x != target.x || battlePos.y != target.y) {
		if (dtMove < distance) {
			battlePos.x = ((dtMove * target.x) + (dtRem * battlePos.x)) / distance;
			battlePos.y = ((dtMove * target.y) + (dtRem * battlePos.y)) / distance;
		}
		else {
			battlePos.x = target.x;
			battlePos.y = target.y;
		}
		actor->SetPosition(battlePos);
	}
	else return true;

	return false;
}

bool BattleScene::perform_useItem(bsPerform* perform) {
	Fighter* actor = perform->actor;

	if (!perform->started) {
		perform->started = true;
		perform->startTime = SDL_GetTicks();

		actor->setAnimation(Cast);
		actor->loadSprite();
		actor->GetSprite()->repeatMode = 1;
		actor->GetSprite()->speed = 500; //Make cast faster, more like a 'throw'
		actor->GetSprite()->start();

		reactor->setAnimation(reaction);
		reactor->loadSprite();
		reactor->addEffect(perform->effect);
		while (perform->floatingLabels.size() > 0) {
			reactor->addFloatingLabel(perform->floatingLabels[0]);
			perform->floatingLabels.erase(perform->floatingLabels.begin());
		}
	}

	int currentTicks = (SDL_GetTicks() - perform->startTime);
	int currentSecs = currentTicks / 1000;

	if (currentSecs >= 1) {
		actor->setAnimation(StandBy);
		actor->loadSprite();

		reactor->setAnimation(StandBy);
		reactor->loadSprite();
		return true;
	}

	return false;
}


bool BattleScene::perform_victory(bsPerform* perform) {
	Fighter* player = reactor;
	Fighter* actor = perform->actor;

	if (!perform->started) {
		perform->started = true;
		perform->startTime = SDL_GetTicks();
		actor->setAnimation(perform->animation);
		actor->loadSprite();
	}

	int currentTicks = (SDL_GetTicks() - perform->startTime);
	int currentSecs = currentTicks / 1000;

	if (currentSecs < 2) {
		actor->SetBattleYell(perform->battleYell);
		actor->setDirection(default_ally_dir);
		actor->loadSprite();
	}
	else {
		actor->SetBattleYell("");
		return true;
	}

	return false;
}


void BattleScene::startReaction() {
	bsPerform* performance = getReactionPerformance();
	if (performance) {
		if (performance->type == bsReaction) {
			performance->startTime = SDL_GetTicks();
			return;
		}
	}

	bsActor* performActor = new bsActor;
	performActor->actor = reactor;

	bsPerform *performReaction = new bsPerform;

	if (reaction != AnimType::Faint) {
		performReaction->actor = reactor;
		performReaction->animation = reaction;
		performReaction->playTime = 1000;
		performReaction->type = bsReaction;
		performActor->performances.push(performReaction);

		performReaction = new bsPerform;
		performReaction->actor = reactor;
		performReaction->type = bsStandby;
		performActor->performances.push(performReaction);
	}
	else {
		performReaction->actor = reactor;
		performReaction->animation = AnimType::Wound;
		performReaction->playTime = 1000;
		performReaction->type = bsReaction;
		performActor->performances.push(performReaction);

		performReaction = new bsPerform;
		performReaction->actor = reactor;
		performReaction->animation = AnimType::Faint;
		performReaction->type = bsFaint;
		performActor->performances.push(performReaction);
	}

	newActors.push_back(performActor);
}


bsPerform* BattleScene::getReactionPerformance() {
	bsActor* reactionActor = nullptr;
	for (auto actor : actors) {
		if (actor->actor == reactor) {
			reactionActor = actor;
			break;
		}
	}
	if (!reactionActor) return nullptr;

	bsPerform* performance = reactionActor->performances.front();
	return performance;
}


void BattleScene::updateAudience(bool reset) {
	if (!reset) {
		//The audience watches the actors
		for (auto actor : cast) {
			if (actor == reactor) continue;
			if (!actor->IsAlive()) continue;

			bool isAudience = true;
			for (auto anActor : actors) {
				if (anActor->actor == actor) {
					isAudience = false;
					break;
				}
			}

			if (isAudience) {
				actor->setDirection(getDirection(actor->GetPosition(), lookTo));
				actor->loadSprite();
			}
		}
	}
	else {
		//reset to default look directions
		for (auto actor : cast) {
			if (actor->IsEnemy()) actor->setDirection(default_enemy_dir);
			else actor->setDirection(default_ally_dir);
			actor->loadSprite();
		}
	}
}