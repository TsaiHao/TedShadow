#include <catch2/catch_test_macros.hpp>

#include "Utils/Utils.h"

TEST_CASE("logger output", "[logger]") {
    std::stringstream ss;
    ted::Logger logger(ss);
    logger.info("hello world");
    REQUIRE(ss.str() == "ted info: hello world\n");

    ss.str("");
    logger.error("test log {}", "log content");
    REQUIRE(ss.str() == "ted error: test log log content\n");
}