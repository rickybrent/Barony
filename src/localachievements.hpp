#pragma once

#include "Config.hpp"

#ifdef USE_LOCALACHIEVEMENTS
#include <vector>
#include <iostream>
#include <map>
#include <atomic>
#include "net.hpp"
#include "stat.hpp"
#include "physfs.h"

typedef struct local_achievement_t
{
	char* name; //The achievement's display name.
	char* description; //The achievement's description.
	bool hidden;

	Uint32 completionTime;
} local_achievement_t;

class LocalAchievementsFuncs
{
public:
	void loadAchievements();
	void unlockAchievement(const char* name);
	void clearAchievement(const char* name);
};

extern LocalAchievementsFuncs LocalAchievements;

#endif //USE_LOCALACHIEVEMENTS
