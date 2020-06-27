#include "CCConfigurationManager.h"
#include <iostream>
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
		throw std::exception(("Failed to load from file " + configFilePath).c_str());
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
			cout << "Error Parsing json file: " << e.what() << endl;
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
			cout << "Error writing to file " << e.what() << endl;
		}
	}
	return false;
}
