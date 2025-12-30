#include "FileNameParser.h"


std::string fNameParser(std::string s)
{
    std::string fileName = "";
    for (int i = s.length() - 1; i >= 0; i--)
    {
        if (s[i] == '/')
        {
            int j = i + 1;
            while (j != s.length())
            {
                fileName += s[j];
                j++;
            }
            int qmIdx = fileName.find('?');
            if (qmIdx != std::string::npos)
            {
                fileName.erase(qmIdx, fileName.size() - qmIdx);
            }
            int htIdx = fileName.find('#');
            if (htIdx != std::string::npos)
            {
                fileName.erase(htIdx, fileName.size() - htIdx);
            }
            break;
        }
    }
    return fileName;
}