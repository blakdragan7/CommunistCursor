#ifndef CC_CONFIGURATION_MANAGER_H
#define CC_CONFIGURATION_MANAGER_H

#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <vector>

class CCConfigurationManager
{
private:
	std::string		_configFilePath;
	bool			_isLoaded;

	nlohmann::json  _jsonData;

public:
	CCConfigurationManager();
	CCConfigurationManager(std::string configFilePath);

	template<typename t>
	bool GetValue(std::vector<std::string> keys, t& outValue)const
	{
		nlohmann::json currentObj = _jsonData;

		for (auto key : keys)
		{
			if (currentObj.contains(key))
			{
				currentObj = currentObj[key];
			}
			else return false;
		}

		outValue = currentObj.get<t>();

		return true;
	}

	template<typename t>
	bool SetValue(std::vector<std::string> keys, const t& value)
	{
		std::string lastKey = *(keys.end() - 1);
		nlohmann::json* currentObj = &_jsonData;

		for (auto key : keys)
		{
			if (key == lastKey)break;
			if (currentObj->contains(key))
			{
				currentObj = &(*currentObj)[key];
			}
			else
			{
				(*currentObj)[key] = nlohmann::json();
				currentObj = &(*currentObj)[key];
			}
		}

		(*currentObj)[lastKey] = value;

		return true;
	}

	bool LoadFromFile(std::string filePath);
	bool SaveToFile(std::string filePath)const;
};

#endif