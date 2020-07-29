#include "CCConfigurationManager.h"
#include "CCLogger.h"
#include <fstream>

using namespace std;
using namespace nlohmann;

CCConfigurationManager::CCConfigurationManager() : _isLoaded(false)
{
}

CCConfigurationManager::CCConfigurationManager(string configFilePath) : _isLoaded(false)
{
	if (LoadFromFile(configFilePath) == false)
	{
		throw std::runtime_error(("Failed to load from file " + configFilePath));
	}
}

bool CCConfigurationManager::LoadFromFile(string filePath)
{
	ifstream inFile(filePath);
	if (inFile.good())
	{
		try 
		{
			_jsonData = json::parse(inFile);

			return true;
		}
		catch (const exception& e)
		{
			LOG_ERROR << "Error Parsing json file: " << e.what() << endl;
		}
	}
	return false;
}

bool CCConfigurationManager::SaveToFile(string filePath)const
{
	ofstream outFile(filePath);
	if (outFile.good())
	{
		try
		{
			outFile.clear();
			outFile << _jsonData;
			outFile.close();

			return true;
		}
		catch (const exception& e)
		{
			LOG_ERROR << "Error writing to file " << e.what() << endl;
		}
	}
	return false;
}
