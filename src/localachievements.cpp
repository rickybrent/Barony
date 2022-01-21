#include "Config.hpp"

#ifdef USE_LOCALACHIEVEMENTS

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "net.hpp"
#include "menu.hpp"
#include "monster.hpp"
#include "scores.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "interface/interface.hpp"
#include <SDL_thread.h>
#include "player.hpp"
#include "mod_tools.hpp"
#include "interface/ui.hpp"


LocalAchievementsFuncs LocalAchievements;

std::string achFilePath()
{
	std::string outputPath = outputdir;
	outputPath.append("/data/local_achievements.json");

	return outputPath;
}

void saveAchievementData()
{
	rapidjson::Document d;
	d.SetObject();
	CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(1));

	rapidjson::Value achievementsObj(rapidjson::kObjectType);
	CustomHelpers::addMemberToRoot(d, "achievements", achievementsObj);
	for (auto& item : achievementNames)
	{
		rapidjson::Value achievement(rapidjson::kObjectType);
		achievement.AddMember("name", rapidjson::Value(item.second.c_str(), item.second.size()), d.GetAllocator());

		auto desc = achievementDesc.find(item.first);
		if ( desc != achievementDesc.end() )
		{
			achievement.AddMember("description", rapidjson::Value(desc->second.c_str(), desc->second.size()), d.GetAllocator());
		}

		auto unlockTime = achievementUnlockTime.find(item.first);
		if (unlockTime != achievementUnlockTime.end() )
		{
			achievement.AddMember("unlocktime", rapidjson::Value(unlockTime->second), d.GetAllocator());
		}
		bool hidden = (achievementHidden.find(item.first) != achievementHidden.end());
		achievement.AddMember("hidden", rapidjson::Value(hidden), d.GetAllocator());
		
		CustomHelpers::addMemberToSubkey(d, "achievements", item.first, achievement);
	}

	std::string outputPath = achFilePath();
	FILE* fp = fopen(outputPath.c_str(), "wb");
	if ( !fp )
	{
		return;
	}
	char buf[65536];
	rapidjson::FileWriteStream os(fp, buf, sizeof(buf));
	rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
	d.Accept(writer);

	fclose(fp);
	printlog("[JSON]: Successfully wrote json file %s", outputPath.c_str());
}

// Almost a straight copy from eos.cpp (omitting one check).
void sortAchievementsForDisplay()
{
	// sort achievements list
	achievementNamesSorted.clear();
	Comparator compFunctor =
		[](std::pair<std::string, std::string> lhs, std::pair<std::string, std::string> rhs)
	{
		bool ach1 = achievementUnlocked(lhs.first.c_str());
		bool ach2 = achievementUnlocked(rhs.first.c_str());
		bool lhsAchIsHidden = (achievementHidden.find(lhs.first) != achievementHidden.end());
		bool rhsAchIsHidden = (achievementHidden.find(rhs.first) != achievementHidden.end());
		if ( ach1 && !ach2 )
		{
			return true;
		}
		else if ( !ach1 && ach2 )
		{
			return false;
		}
		else if ( !ach1 && !ach2 && (lhsAchIsHidden || rhsAchIsHidden) )
		{
			if ( lhsAchIsHidden && rhsAchIsHidden )
			{
				return lhs.second < rhs.second;
			}
			if ( !lhsAchIsHidden )
			{
				return true;
			}
			if ( !rhsAchIsHidden )
			{
				return false;
			}
			return lhs.second < rhs.second;
		}
		else
		{
			return lhs.second < rhs.second;
		}
	};
	std::set<std::pair<std::string, std::string>, Comparator> sorted(achievementNames.begin(), achievementNames.end(), compFunctor);
	achievementNamesSorted.swap(sorted);
}

void LocalAchievementsFuncs::unlockAchievement(const char* name)
{
	if (achievementNames.find(name) == achievementNames.end())
	{
		std::string display = std::string(name).substr(11);
		std::replace(display.begin(), display.end(), '_', ' ');
		achievementNames.emplace(std::make_pair(
			std::string(name),
			display
		));
	}
	int64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	achievementUnlockTime.emplace(std::make_pair(std::string(name), now));

	UIToastNotificationManager.createAchievementNotification(name);
	saveAchievementData();
	sortAchievementsForDisplay();
}

void LocalAchievementsFuncs::clearAchievement(const char* name)
{
	achievementUnlockTime.erase(std::string(name));
	achievementUnlockedLookup.erase(std::string(name));
	saveAchievementData();
	sortAchievementsForDisplay();
}

void LocalAchievementsFuncs::loadAchievements()
{
	//achievementUnlockedLookup.insert(std::string(PlayerAchievement->AchievementId));
	std::string inputPath = achFilePath();

	FILE* fp = fopen(inputPath.c_str(), "rb");
	if ( !fp )
	{
		printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
		return;
	}
	char buf[65536];
	rapidjson::FileReadStream is(fp, buf, sizeof(buf));
	fclose(fp);

	rapidjson::Document d;
	d.ParseStream(is);
	if ( !d.HasMember("version") || !d.HasMember("achievements") )
	{
		printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
		return;
	}
	int version = d["version"].GetInt();

	for ( rapidjson::Value::ConstMemberIterator itr = d["achievements"].MemberBegin(); itr != d["achievements"].MemberEnd(); ++itr )
	{
		auto key = itr->name.GetString();
		achievementNames.emplace(std::make_pair(
			std::string(key),
			std::string(itr->value["name"].GetString())));
		if (itr->value.HasMember("description")) {
			achievementDesc.emplace(std::make_pair(
				std::string(key),
				std::string(itr->value["description"].GetString())));
		}
		if (itr->value.HasMember("unlocktime")) {
			achievementUnlockedLookup.emplace(std::string(key));
			achievementUnlockTime.emplace(std::make_pair(std::string(key), itr->value["unlocktime"].GetInt64()));
		}
		if (itr->value.HasMember("hidden") && itr->value["hidden"].GetBool()) {
			achievementHidden.emplace(std::string(key));
		}
	}
	printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
	sortAchievementsForDisplay();
}
#endif  // USE_LOCALACHIEVEMENTS