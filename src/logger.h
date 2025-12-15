/**
 * @file logger.h
 * @brief Internal logging system for Kestrel ray tracer
 * @author Kestrel Team
 * @date 2025
 * 
 * @internal
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

/**
 * @enum LogLevel
 * @brief Severity levels for log messages
 */
enum class LogLevel {
  DEBUG,   ///< Detailed debugging information
  INFO,    ///< General informational messages
  WARNING, ///< Warning messages for potentially problematic situations
  ERROR    ///< Error messages for serious problems
};

/**
 * @class Logger
 * @brief Thread-safe logging system with configurable output and log levels
 *
 * The Logger class provides a singleton pattern for consistent logging
 * throughout the application. It supports multiple log levels, colored
 * console output, and optional file logging.
 */
class Logger {
private:
  static Logger *instance;        ///< Singleton instance
  static std::mutex mutex;        ///< Mutex for thread-safe logging
  LogLevel min_level;             ///< Minimum log level to display
  bool log_to_file;               ///< Whether to log to file
  bool log_to_console;            ///< Whether to log to console
  bool use_colors;                ///< Whether to use ANSI colors in console
  std::ofstream log_file;         ///< Output file stream
  std::string log_file_path;      ///< Path to log file

  /**
   * @brief Private constructor for singleton pattern
   */
  Logger()
      : min_level(LogLevel::INFO), log_to_file(false), log_to_console(true),
        use_colors(true) {}


  /**
   * @brief Get string representation of log level
   * @param level The log level
   * @return String representation
   */
  std::string level_to_string(LogLevel level) const {
    switch (level) {
    case LogLevel::DEBUG:
      return "DEBUG";
    case LogLevel::INFO:
      return "INFO";
    case LogLevel::WARNING:
      return "WARNING";
    case LogLevel::ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
    }
  }

  /**
   * @brief Get ANSI color code for log level
   * @param level The log level
   * @return ANSI color code string
   */
  std::string get_color_code(LogLevel level) const {
    if (!use_colors)
      return "";

    switch (level) {
    case LogLevel::DEBUG:
      return "\033[36m"; // Cyan
    case LogLevel::INFO:
      return "\033[32m"; // Green
    case LogLevel::WARNING:
      return "\033[33m"; // Yellow
    case LogLevel::ERROR:
      return "\033[31m"; // Red
    default:
      return "\033[0m"; // Reset
    }
  }

  /**
   * @brief Get ANSI reset code
   * @return ANSI reset code string
   */
  std::string get_reset_code() const {
    return use_colors ? "\033[0m" : "";
  }

public:
  /**
   * @brief Get the singleton instance of Logger
   * @return Pointer to the Logger instance
   */
  static Logger *get_instance() {
    if (instance == nullptr) {
      std::lock_guard<std::mutex> lock(mutex);
      if (instance == nullptr) {
        instance = new Logger();
      }
    }
    return instance;
  }

  /**
   * @brief Delete copy constructor
   */
  Logger(const Logger &) = delete;

  /**
   * @brief Delete assignment operator
   */
  Logger &operator=(const Logger &) = delete;

  /**
   * @brief Destructor - closes log file if open
   */
  ~Logger() {
    if (log_file.is_open()) {
      log_file.close();
    }
  }

  /**
   * @brief Set the minimum log level to display
   * @param level The minimum log level
   */
  void set_log_level(LogLevel level) { min_level = level; }

  /**
   * @brief Enable or disable console logging
   * @param enabled Whether to enable console logging
   */
  void set_console_logging(bool enabled) { log_to_console = enabled; }

  /**
   * @brief Enable or disable colored output
   * @param enabled Whether to enable colored output
   */
  void set_colored_output(bool enabled) { use_colors = enabled; }

  /**
   * @brief Enable file logging
   * @param filepath Path to the log file
   * @return True if file was opened successfully
   */
  bool enable_file_logging(const std::string &filepath) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (log_file.is_open()) {
      log_file.close();
    }

    log_file_path = filepath;
    log_file.open(filepath, std::ios::out | std::ios::app);
    
    if (log_file.is_open()) {
      log_to_file = true;
      return true;
    }
    
    log_to_file = false;
    return false;
  }

  /**
   * @brief Disable file logging
   */
  void disable_file_logging() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (log_file.is_open()) {
      log_file.close();
    }
    log_to_file = false;
  }

  /**
   * @brief Log a message with specified level
   * @param level The log level
   * @param message The message to log
   * @param file Source file name (optional)
   * @param line Source line number (optional)
   */
  void log(LogLevel level, const std::string &message,
           const char *file = nullptr, int line = -1) {
    if (level < min_level) {
      return;
    }

    std::lock_guard<std::mutex> lock(mutex);

    std::stringstream ss;
    
    if (file != nullptr && line > 0) {
      ss << " [" << file << ":" << line << "]";
    }
    
    ss << ": " << message;

    std::string log_message = ss.str();

    if (log_to_console) {
      std::cerr << get_color_code(level) << log_message << get_reset_code()
                << std::endl;
    }

    if (log_to_file && log_file.is_open()) {
      log_file << log_message << std::endl;
      log_file.flush();
    }
  }

  /**
   * @brief Log a debug message
   * @param message The message to log
   */
  void debug(const std::string &message) { log(LogLevel::DEBUG, message); }

  /**
   * @brief Log an info message
   * @param message The message to log
   */
  void info(const std::string &message) { log(LogLevel::INFO, message); }

  /**
   * @brief Log a warning message
   * @param message The message to log
   */
  void warning(const std::string &message) { log(LogLevel::WARNING, message); }

  /**
   * @brief Log an error message
   * @param message The message to log
   */
  void error(const std::string &message) { log(LogLevel::ERROR, message); }
};

// Convenience macros for logging with file and line information
#define LOG_DEBUG(msg) Logger::get_instance()->log(LogLevel::DEBUG, msg, __FILE__, __LINE__)
#define LOG_INFO(msg) Logger::get_instance()->log(LogLevel::INFO, msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) Logger::get_instance()->log(LogLevel::WARNING, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::get_instance()->log(LogLevel::ERROR, msg, __FILE__, __LINE__)

#endif // LOGGER_H
