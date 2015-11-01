#include <iostream>
#include <vector>
#include <CL/cl.h>
#include <CL/cl_platform.h>
#include <CL/opencl.h>
#include <fstream>
#include <string>

void LoadFileData(std::string fileName,std::vector<std::string> &data)
{
	std::string currentLine;
	std::ifstream fileStreamer(fileName);

	if(fileStreamer.is_open())
	{
		while (getline(fileStreamer, currentLine))
			data.push_back(currentLine);

		fileStreamer.close();
		std::cout << fileName + " successfully read\n";

	}else std::cout << "Unable to open file";
}

void LoadOCTData()
{
	std::cout << "Loading OCT data\n";

	std::vector<std::string> resamplingTableData;
	std::vector<std::string> referenceAScanData;
	std::vector<std::string> referenceSpectrumData;

	LoadFileData("resamplingTable.csv", resamplingTableData);
	LoadFileData("referenceAScan.csv", referenceAScanData);
	LoadFileData("referenceSpectrum.csv", referenceSpectrumData);
}

int main()
{
	LoadOCTData();
	
	int i;
	std::cin >> i;

	return 0;
}

