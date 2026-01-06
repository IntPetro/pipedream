#include "Structs.h"

std::wstring string_to_wstring(const std::string& str)
{
    if (str.empty())
        return L"";
    int size_needed = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        str.data(),
        (int)str.size(),
        nullptr,
        0
    );
    if (size_needed == 0)
        throw std::runtime_error("MultiByteToWideChar failed");
    std::wstring wstr(size_needed, 0);
    int result = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        str.data(),
        (int)str.size(),
        &wstr[0],
        size_needed
    );

    if (result == 0)
        throw std::runtime_error("MultiByteToWideChar failed");

    return wstr;
}

void ChunkQueue::push(const Chunk& c)
{
    std::lock_guard<std::mutex> lock(m);
    q.push(c);
}

bool ChunkQueue::pop(Chunk& out)
{
    std::lock_guard<std::mutex> lock(m);
    if (q.empty())
        return false;
    out = q.front();
    q.pop();
    return true;
}