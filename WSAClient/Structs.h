#pragma once
#include <vector>
#include <Windows.h>
#include <string>
#include <stdexcept>
#include <queue>
#include <mutex>
struct ChunkStats {
	std::atomic<uint64_t> bytes{ 0 };
};
struct Chunk
{
	uint64_t start;
	uint64_t end;
	ChunkStats* stats;
};

struct ThingInfo {
	uint64_t conLen;
	bool supportsRanges;
	std::vector<Chunk> chunks;
};
class ChunkQueue {
	std::queue<Chunk> q;
	std::mutex m;
public:
	void push(const Chunk& c);
	bool pop(Chunk& out);
};
std::wstring string_to_wstring(const std::string& str);