#ifndef UTILS_H
#define UTILS_H

#include <napi.h>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

std::vector<std::string> SplitString(const std::string &input, char delimiter);
Napi::Value ToNapiString(const Napi::Env env, const std::string str);
std::string ConvertVariantToString(VARIANT variant);
std::wstring NarrowStringToWideString(const std::string &narrowStr);
std::wstring WideStringFromArray(const WCHAR *wcharArray);
std::string ToUTF8(const std::wstring &wide);
std::string GUIDToString(const GUID &guid);

bool Contains(const std::string &str, const std::string &substring);
bool Every(const std::vector<bool> vector);
int CountOccurrence(const std::vector<std::string> vector, const std::string target);
void LogToConsole(const Napi::Env env, const std::string &message);

#endif// UTILS_H