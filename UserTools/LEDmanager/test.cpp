#include <iostream>
#include <string>
#include <sstream>

int main()
{
	std::string l0 = "    0  1  2  3  4  5  6 ";
	std::string lA = "a0: -- -- -- a3 -- -- --";
	std::string lB = "b0: -- -- -- -- -- -- --";

	std::cout << l0 << std::endl << lA << std::endl << lB << std::endl;

	int pp;
	if (pp = l0.find_first_of(':') != std::string::npos)
	{
		l0.erase(0, l0.find_first_of(':')+1);
		std::cout << l0 << std::endl;
	}
	if (pp = lA.find_first_of(':') != std::string::npos)
	{
		lA.erase(0, lA.find_first_of(':')+1);
		std::cout << lA << std::endl;
	}
	if (pp = lB.find_first_of(':') != std::string::npos)
	{
		lB.erase(0, lB.find_first_of(':')+1);
		std::cout << lB << std::endl;
	}

	std::stringstream ssA(lA), ssB(lB);

	std::string key;
	while(ssA >> key)
		if (key != "--")
			std::cout << key << std::endl;
	while(ssB >> key)
		if (key != "--")
			std::cout << key << std::endl;


	char asd = 0x10;
	std::cout << "asd " << std::hex << (0xff & asd) << "\t" << (0xff & ~asd) << std::endl;

	return 0;
}
