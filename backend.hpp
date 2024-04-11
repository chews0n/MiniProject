// Function prototypes
bool readInputData(const std::string& inputFilePath, const std::string& wellName, std::vector<double>& times, std::vector<double>& pressures);
bool writeOutputData(const std::string& outputFilePath, const std::vector<double>& times, const std::vector<double>& pressures,
                     const std::vector<double>& predictedPressures, const std::vector<double>& deltaP);
bool writePlotData(const std::string& outputFilePath, const std::vector<double>& times, const std::vector<double>& pressures,
                   const std::vector<double>& predictedPressures, double testStartTime, double testEndTime, double slope);
int findTimeColumnIndex(const std::vector<std::string>& headers);
int findPressureColumnIndex(const std::vector<std::string>& headers, const std::string& wellName);
void smoothPressureData(std::vector<double>& pressures, std::vector<double>& times, double threshold);
bool isNumericOrNaN(const std::string& str);
std::vector<std::string> splitString(const std::string& str, char delimiter);
void calculateTrendAndDeltaP(const std::vector<double>& times, const std::vector<double>& pressures,
                             double slope, double testStartTime, double testEndTime,
                             std::vector<double>& trendTimes, std::vector<double>& trendPressures,
                             std::vector<double>& predictedPressures, std::vector<double>& deltaP);
std::string generateOutputFileName(const std::string& wellName, const std::string& suffix);
double calculateSlope(const std::vector<double>& times, const std::vector<double>& pressures, double testStartTime);
