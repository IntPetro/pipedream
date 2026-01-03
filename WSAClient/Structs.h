#pragma once
#include <vector>
#include <Windows.h>
#include <string>
#include <stdexcept>
struct Chunk
{
	uint64_t start;
	uint64_t end;
};

struct ThingInfo {
	uint64_t conLen;
	bool supportsRanges;
	std::vector<Chunk> chunks;
};
std::wstring string_to_wstring(const std::string& str);