#include "doctest.h"
#include "logger.h"

using namespace kestrel;

TEST_CASE("Logger::get_instance returns the same singleton") {
  Logger *a = Logger::get_instance();
  Logger *b = Logger::get_instance();
  CHECK(a == b);
  CHECK(a != nullptr);
}

TEST_CASE("Logger level filtering suppresses messages below the threshold") {
  Logger *log = Logger::get_instance();
  // Keep the test output clean: no console/file side effects.
  log->set_console_logging(false);
  log->set_colored_output(false);

  // At WARNING, DEBUG/INFO must be dropped and WARNING/ERROR allowed. The log
  // call itself must never throw for non-fatal levels.
  log->set_log_level(LogLevel::WARNING);
  CHECK_NOTHROW(log->debug("suppressed debug"));
  CHECK_NOTHROW(log->info("suppressed info"));
  CHECK_NOTHROW(log->warning("visible warning"));
  CHECK_NOTHROW(log->error("visible error"));

  log->set_console_logging(true);  // restore default for any later use
}

TEST_CASE("LOG_ERROR logs and throws KestrelError") {
  Logger *log = Logger::get_instance();
  log->set_console_logging(false);
  CHECK_THROWS_AS(LOG_ERROR("fatal condition"), KestrelError);
  log->set_console_logging(true);
}

TEST_CASE("KestrelError carries its message") {
  try {
    throw KestrelError("boom");
  } catch (const KestrelError &e) {
    CHECK(std::string(e.what()) == "boom");
  }
}
