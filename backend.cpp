#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include "backend.hpp"
#include <omp>



bool readInputData(const std::string& inputFilePath, const std::string& wellName, std::vector<double>& times, std::vector<double>& pressures) {
    std::ifstream inputFile(inputFilePath);
    if (!inputFile) {
        std::cerr << "Failed to open the input file." << std::endl;
        return false;
    }

    std::string line;
    std::vector<std::string> headers;
    int timeColumnIndex = -1;
    int pressureColumnIndex = -1;

    // Read the entire file
    std::vector<std::vector<std::string>> fileData;
    while (std::getline(inputFile, line)) {
        fileData.push_back(splitString(line, ','));
    }

    // Find the well name in the file data
    for (size_t i = 0; i < fileData.size(); ++i) {
        for (size_t j = 0; j < fileData[i].size(); ++j) {
            if (fileData[i][j] == wellName) {
                headers = fileData[0];
                timeColumnIndex = findTimeColumnIndex(headers);
                pressureColumnIndex = static_cast<int>(j);
                break;
            }
        }
        if (pressureColumnIndex != -1) {
            break;
        }
    }

    if (pressureColumnIndex == -1) {
        std::cerr << "Well name not found in the input file." << std::endl;
        return false;
    }

    // Extract time and pressure data
    for (size_t i = 1; i < fileData.size(); ++i) {
        const auto& values = fileData[i];
        if (values.size() > static_cast<size_t>(std::max(timeColumnIndex, pressureColumnIndex)) && isNumericOrNaN(values[timeColumnIndex]) && isNumericOrNaN(values[pressureColumnIndex])) {
            double time = std::stod(values[timeColumnIndex]);
            double pressure = std::stod(values[pressureColumnIndex]);
            times.push_back(time);
            pressures.push_back(pressure);
        }
    }

    inputFile.close();
    return true;
}

bool writeOutputData(const std::string& outputFilePath, const std::vector<double>& times, const std::vector<double>& pressures,
                     const std::vector<double>& predictedPressures, const std::vector<double>& deltaP) {
    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open the output file: " << outputFilePath << std::endl;
        return false;
    }

    outputFile << "Time,Pressure,PredictedPressure,DeltaP\n";
    for (size_t i = 0; i < times.size(); ++i) {
        outputFile << times[i] << "," << pressures[i] << "," << predictedPressures[i] << "," << deltaP[i] << std::endl;
    }

    outputFile.close();
    return true;
}

bool writePlotData(const std::string& outputFilePath, const std::vector<double>& times, const std::vector<double>& pressures,
                   const std::vector<double>& predictedPressures, double testStartTime, double testEndTime, double slope) {
    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open the plot data file: " << outputFilePath << std::endl;
        return false;
    }

    outputFile << "Time,Pressure,PredictedPressure,TestStartTime,TestEndTime,Slope\n";
    for (size_t i = 0; i < times.size(); ++i) {
        if (times[i] >= testStartTime - 24.0 && times[i] <= testEndTime) {
            outputFile << times[i] << "," << pressures[i] << "," << predictedPressures[i] << ",";
            if (i == 0) {
                outputFile << testStartTime << "," << testEndTime << "," << slope;
            }
            outputFile << std::endl;
        }
    }

    outputFile.close();
    return true;
}


std::string generateOutputFileName(const std::string& wellName, const std::string& suffix) {
    static int testNumber = 1;
    std::stringstream ss;
    ss << wellName << "_test_" << testNumber << suffix;
    ++testNumber;
    return ss.str();
}

int findTimeColumnIndex(const std::vector<std::string>& headers) {
    for (unsigned int i = 0; i < headers.size(); ++i) {
        std::string header = headers[i];
        std::transform(header.begin(), header.end(), header.begin(), ::tolower);
        if (header == "time" || header == "time step" || header == "Time") {
            return i;
        }
    }
    return 0; // Assume the first column is the time column if no explicit time column found
}
 
void smoothPressureData(std::vector<double>& pressures, std::vector<double>& times, double threshold) {
    std::vector<double> smoothedPressures;
    std::vector<double> smoothedTimes;

    if (!pressures.empty() && !times.empty()) {
        smoothedPressures.push_back(pressures[0]);
        smoothedTimes.push_back(times[0]);

        for (size_t i = 1; i < pressures.size(); ++i) {
            bool shouldInclude = true;
            for (size_t j = 0; j < smoothedPressures.size(); ++j) {
                if (std::abs(pressures[i] - smoothedPressures[j]) < threshold) {
                    shouldInclude = false;
                    break;
                }
            }

            if (shouldInclude) {
                smoothedPressures.push_back(pressures[i]);
                smoothedTimes.push_back(times[i]);
            }
        }
    } else { 
		// what to do if it is empty
	}

    pressures = std::move(smoothedPressures);
    times = std::move(smoothedTimes);
}

bool isNumericOrNaN(const std::string& str) {
    if (str == "nan" || str == "NaN") return true;
    char* end = nullptr;
    std::strtod(str.c_str(), &end);
    return end != str.c_str() && *end == '\0';
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
} 
void calculateTrendAndDeltaP(const std::vector<double>& times, const std::vector<double>& pressures,
                             double slope, double testStartTime, double testEndTime,
                             std::vector<double>& trendTimes, std::vector<double>& trendPressures,
                             std::vector<double>& predictedPressures, std::vector<double>& deltaP) {
    // Find the index of the data point just before the test start time
    size_t startIndex = -1;
    while (startIndex < times.size() && times[startIndex] < testStartTime) {
        ++startIndex;
    }

	if (startIndex < 0) { 
		// return some error
	}

    // Calculate the intercept of the trend line
    double intercept = pressures[startIndex] - slope * times[startIndex];

    // Calculate trend line data and delta P
    for (size_t i = 0; i < times.size(); ++i) {
        double trendPressure = slope * times[i] + intercept;
        predictedPressures.push_back(trendPressure);
        deltaP.push_back(pressures[i] - trendPressure);

        if (times[i] >= testStartTime - 24.0 && times[i] <= testEndTime) {
            trendTimes.push_back(times[i]);
            trendPressures.push_back(trendPressure);
        }
    }
}


double calculateSlope(const std::vector<double>& times, const std::vector<double>& pressures, double testStartTime) {
    // Find the index of the data point just before the test start time
    size_t startIndex = 0;
    while (startIndex < times.size() && times[startIndex] < testStartTime - 24.0) {
        ++startIndex;
    }

    size_t endIndex = startIndex;
    while (endIndex < times.size() && times[endIndex] < testStartTime) {
        ++endIndex;
    }

    if (startIndex == endIndex) {
        return 0.0;
    }

    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;
    int n = endIndex - startIndex;

    for (size_t i = startIndex; i < endIndex; ++i) {
        sumX += times[i];
        sumY += pressures[i];
        sumXY += times[i] * pressures[i];
        sumX2 += times[i] * times[i];
    }

    double slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    return slope;
} 


int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <well_name> <slope> <test_start_time> <test_end_time>" << std::endl;
		// maybe put in a description of the inputs
        return 1;
    }

    std::string inputFilePath = argv[1];
    std::string wellName = argv[2];
    double testStartTime = std::stod(argv[4]);
    double testEndTime = std::stod(argv[5]);

    std::vector<double> times, pressures;
    if (!readInputData(inputFilePath, wellName, times, pressures)) {
        return 1; // Error message is printed inside readInputData
    }

    smoothPressureData(pressures, times, 0.5);

    double slope;
    if (std::string(argv[3]) == "auto") {
        slope = calculateSlope(times, pressures, testStartTime);
    } else {
        slope = std::stod(argv[3]);
    }

    std::vector<double> trendTimes, trendPressures, predictedPressures, deltaP;
    calculateTrendAndDeltaP(times, pressures, slope, testStartTime, testEndTime,
                            trendTimes, trendPressures, predictedPressures, deltaP);

    std::string outputFileName = generateOutputFileName(wellName, "_output.csv");
    if (!writeOutputData(outputFileName, times, pressures, predictedPressures, deltaP)) {
        std::cerr << "Failed to write output data." << std::endl;
        return 1;
    }

    std::string plotFileName = generateOutputFileName(wellName, "_plot_data.csv");
    if (!writePlotData(plotFileName, times, pressures, predictedPressures, testStartTime, testEndTime, slope)) {
        std::cerr << "Failed to write plot data." << std::endl;
        return 1;
        
    }

    return 0;
}
