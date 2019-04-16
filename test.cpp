#include <string.h>
#include <iostream>

using namespace std;

string getOSName();

int main()
{
  string OSName = getOSName();
  cout << OSName << endl;
	
	if(OSName == "Unix" || OSName == "Linux" || OSName == "Android")
  {
		system("./merlinAgent-Linux-x64 -url \"https://127.0.0.1:443\"");
	}
  if(OSName == ("Windows 64-bit"))
	{
		system("./merlinAgent-Windows-x64 -url \"https://127.0.0.1:443\"");
	}
	if(OSName == ("Windows 32-bit"))
	{
		system("./merlinAgent-Windows-x86 -url \"https://127.0.0.1:443\"");
	}
  if(OSName == ("Apple"))
	{
		system("./merlinAgent-MacOS -url \"https://127.0.0.1:443\"");
	}

  return 1;
}

string getOSName()
{
  #ifdef _WIN32
  return "Windows 32-bit";
  #elif _WIN64
  return "Windows 64-bit";
  #elif __unix || __unix__
  return "Unix";
  #elif __APPLE__
  return "Apple";
  #elif __linux__
  return "Linux";
  #elif __ANDROID__
  return "Android";
  #else
  return "Other";
  #endif
}



