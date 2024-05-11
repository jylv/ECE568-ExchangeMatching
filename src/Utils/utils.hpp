#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <ctime>

bool isValidNumber(const std::string &line);
bool isValidPrice(const std::string &line, bool mustPositive, double *result);
bool isValidInteger(const std::string &line, bool mustPositive, int *result);
bool isValidAccountNum(const std::string &line);
bool isValidSymbol(const std::string &line);
time_t getTimeStamp();

#endif