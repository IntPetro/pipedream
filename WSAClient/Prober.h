#pragma once
#include <Windows.h>
#include <winhttp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Structs.h"


class Prober
{
private:
	std::wstring host;
	std::wstring path;
	bool https;

public:
	Prober(const std::wstring& h, const std::wstring& p, bool b);
	ThingInfo probe(int parts);
};

