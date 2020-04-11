
#include <iostream>

#include "ClauJson.h"

#include <ctime>


int main(void)
{
	wiz::UserType global;
	int a = clock();

	wiz::LoadData::LoadDataFromFile("input.json", global, 0, 0);

	int b = clock();

	std::cout << b - a << "ms\n";

//	wiz::LoadData::SaveWizDB2(global, "output.eu4");

	return 0;
}
