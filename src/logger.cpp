/**
 * @file logger.cpp
 * @brief Implementation of Logger class
 * @author Kestrel Team
 * @date 2025
 */

#include "logger.h"

// Initialize static members
Logger *Logger::instance = nullptr;
std::mutex Logger::mutex;
