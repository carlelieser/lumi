#include "utils.h"

std::string EscapeString(const std::string& input) {
	std::string escapedString;

	for (char c : input) {
		switch (c) {
			case '\\':
				escapedString += "\\\\";
				break;
			case '\n':
				escapedString += "\\n";
				break;
			case '\r':
				escapedString += "\\r";
				break;
			case '\t':
				escapedString += "\\t";
				break;
			case '\b':
				escapedString += "\\b";
				break;
			case '\f':
				escapedString += "\\f";
				break;
			default:
				escapedString += c;
				break;
		}
	}

	return escapedString;
}


std::vector<std::string> SplitString(const std::string &input, char delimiter) {
	std::vector<std::string> result;
	std::istringstream iss(input);

	std::string substring;
	while (std::getline(iss, substring, delimiter)) {
		result.push_back(substring);
	}

	return result;
}

Napi::Value ToNapiString(const Napi::Env env, const std::string str) {
	return str.empty() ? env.Null() : Napi::String::New(env, str);
}

std::string ConvertWideCharToMultiByte(const wchar_t *wideString) {
	int size = WideCharToMultiByte(CP_UTF8, 0, wideString, -1, nullptr, 0, nullptr, nullptr);

	std::string result;
	if (size > 0) {
		result.resize(size - 1);
		WideCharToMultiByte(CP_UTF8, 0, wideString, -1, &result[0], size, nullptr, nullptr);
	}

	return result;
}

std::string ConvertVariantToString(VARIANT variant) {
	if (variant.parray != nullptr) {
		SAFEARRAY *sa = variant.parray;
		ULONG numElements = sa->rgsabound[0].cElements;
		uint16_t *dataArray;
		SafeArrayAccessData(sa, reinterpret_cast<void **>(&dataArray));

		std::string str;
		for (ULONG i = 0; i < numElements; i++) {
			if (dataArray[i] != L'\0') {
				str += ConvertWideCharToMultiByte(&(static_cast<wchar_t>(dataArray[i])));
			}
		}

		SafeArrayUnaccessData(sa);

		return str;
	}

	return "";
}

std::wstring NarrowStringToWideString(const std::string &narrowStr) {
	int length = static_cast<int>(narrowStr.length());
	int wideStrLength = MultiByteToWideChar(CP_UTF8, 0, narrowStr.c_str(), length, nullptr, 0);
	std::wstring wideStr(wideStrLength, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, narrowStr.c_str(), length, &wideStr[0], wideStrLength);
	return wideStr;
}

std::wstring WideStringFromArray(const WCHAR *wcharArray) {
	return std::wstring(wcharArray);
}

std::string ToUTF8(const std::wstring &wide) {
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide[0], (int) wide.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wide[0], (int) wide.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

std::string GUIDToString(const GUID &guid) {
	wchar_t guidString[40];
	StringFromGUID2(guid, guidString, 40);

	std::wstring wideString(guidString);
	std::string stringResult(wideString.begin(), wideString.end());

	return stringResult;
}

bool Contains(const std::string &str, const std::string &substring) {
	return str.find(substring) != std::string::npos;
}

bool Every(const std::vector<bool> vector) {
	if (vector.empty()) return false;
	for (auto item: vector) {
		if (!item) return false;
	}
	return true;
}

int CountOccurrence(const std::vector<std::string> vector, const std::string target){
	int occurrence = 0;
	for (const auto item: vector) {
		if (item == target) ++occurrence;
	}
	return occurrence;
}

void LogToConsole(const Napi::Env env, const std::string &message) {
	Napi::Object console = env.Global().Get("console").As<Napi::Object>();
	Napi::Function log = console.Get("log").As<Napi::Function>();
	log.Call(console, {Napi::String::New(env, message)});
}

std::string StringPrintf(const char* format, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	return std::string(buffer);
}

std::string WideToUTF8(const std::wstring& wide_str) {
	if (wide_str.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), (int)wide_str.size(), nullptr, 0, nullptr, nullptr);
	std::string utf8_str(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), (int)wide_str.size(), &utf8_str[0], size_needed, nullptr, nullptr);
	return utf8_str;
}

std::wstring FixedArrayToStringView(const wchar_t* array) {
	return std::wstring(array);
}

