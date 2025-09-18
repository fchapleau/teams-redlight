#include <unity.h>
#include <Arduino.h>
#include "../include/logging.h"

void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
}

void test_logger_initialization() {
    Logger::begin(115200);
    TEST_ASSERT_EQUAL(LOG_LEVEL, Logger::getLevel());
}

void test_logger_level_setting() {
    Logger::setLevel(LOG_LEVEL_DEBUG);
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, Logger::getLevel());
    
    Logger::setLevel(LOG_LEVEL_ERROR);
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, Logger::getLevel());
    
    Logger::setLevel(LOG_LEVEL_NONE);
    TEST_ASSERT_EQUAL(LOG_LEVEL_NONE, Logger::getLevel());
}

void test_log_level_constants() {
    TEST_ASSERT_EQUAL(0, LOG_LEVEL_DEBUG);
    TEST_ASSERT_EQUAL(1, LOG_LEVEL_INFO);
    TEST_ASSERT_EQUAL(2, LOG_LEVEL_WARN);
    TEST_ASSERT_EQUAL(3, LOG_LEVEL_ERROR);
    TEST_ASSERT_EQUAL(4, LOG_LEVEL_NONE);
    
    // Verify ordering
    TEST_ASSERT_LESS_THAN(LOG_LEVEL_INFO, LOG_LEVEL_DEBUG);
    TEST_ASSERT_LESS_THAN(LOG_LEVEL_WARN, LOG_LEVEL_INFO);
    TEST_ASSERT_LESS_THAN(LOG_LEVEL_ERROR, LOG_LEVEL_WARN);
    TEST_ASSERT_LESS_THAN(LOG_LEVEL_NONE, LOG_LEVEL_ERROR);
}

void test_log_prefixes() {
    TEST_ASSERT_EQUAL_STRING("[DEBUG]", LOG_PREFIX_DEBUG);
    TEST_ASSERT_EQUAL_STRING("[INFO] ", LOG_PREFIX_INFO);
    TEST_ASSERT_EQUAL_STRING("[WARN] ", LOG_PREFIX_WARN);
    TEST_ASSERT_EQUAL_STRING("[ERROR]", LOG_PREFIX_ERROR);
}

void test_log_colors() {
    TEST_ASSERT_NOT_NULL(LOG_COLOR_DEBUG);
    TEST_ASSERT_NOT_NULL(LOG_COLOR_INFO);
    TEST_ASSERT_NOT_NULL(LOG_COLOR_WARN);
    TEST_ASSERT_NOT_NULL(LOG_COLOR_ERROR);
    TEST_ASSERT_NOT_NULL(LOG_COLOR_RESET);
    
    // Verify colors are different
    TEST_ASSERT_NOT_EQUAL(strcmp(LOG_COLOR_DEBUG, LOG_COLOR_INFO), 0);
    TEST_ASSERT_NOT_EQUAL(strcmp(LOG_COLOR_INFO, LOG_COLOR_WARN), 0);
    TEST_ASSERT_NOT_EQUAL(strcmp(LOG_COLOR_WARN, LOG_COLOR_ERROR), 0);
}

void test_basic_logging_functions() {
    // Test that logging functions can be called without errors
    // Note: We can't easily test output in unit tests, but we can test that
    // the functions exist and can be called
    
    Logger::setLevel(LOG_LEVEL_DEBUG);
    
    Logger::debug("Test debug message");
    Logger::info("Test info message");
    Logger::warn("Test warning message");
    Logger::error("Test error message");
    
    Logger::debug("Component", "Test debug with component");
    Logger::info("Component", "Test info with component");
    Logger::warn("Component", "Test warning with component");
    Logger::error("Component", "Test error with component");
    
    // Test that functions complete without crashing
    TEST_ASSERT_TRUE(true);
}

void test_logging_level_filtering() {
    // Test that higher level messages are filtered out
    Logger::setLevel(LOG_LEVEL_ERROR);
    
    // These should be filtered out (no way to test output in unit test)
    Logger::debug("This should be filtered");
    Logger::info("This should be filtered");
    Logger::warn("This should be filtered");
    
    // This should not be filtered
    Logger::error("This should appear");
    
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, Logger::getLevel());
}

void test_formatted_logging() {
    Logger::setLevel(LOG_LEVEL_DEBUG);
    
    // Test formatted logging functions exist and can be called
    Logger::debugf("Debug test: %d", 42);
    Logger::infof("Info test: %s", "hello");
    Logger::warnf("Warning test: %d %s", 123, "test");
    Logger::errorf("Error test: %f", 3.14);
    
    TEST_ASSERT_TRUE(true);
}

void setup() {
    delay(2000); // Wait for serial monitor
    
    UNITY_BEGIN();
    
    RUN_TEST(test_logger_initialization);
    RUN_TEST(test_logger_level_setting);
    RUN_TEST(test_log_level_constants);
    RUN_TEST(test_log_prefixes);
    RUN_TEST(test_log_colors);
    RUN_TEST(test_basic_logging_functions);
    RUN_TEST(test_logging_level_filtering);
    RUN_TEST(test_formatted_logging);
    
    UNITY_END();
}

void loop() {
    // Empty loop for testing
}