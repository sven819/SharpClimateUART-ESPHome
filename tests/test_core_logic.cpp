#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>
#include <cstdarg>

// Include the core implementation
#include "core_logic.h"
#include "core_frame.h"
#include "core_messages.h"
#include "core_types.h"
#include "core_state.h"

using namespace esphome::sharp_ac;

// ============================================================================
// Mock Hardware Interface for Testing
// ============================================================================

class MockHardwareInterface : public SharpAcHardwareInterface {
public:
    std::vector<uint8_t> uart_buffer;
    std::vector<std::vector<uint8_t>> sent_frames;
    unsigned long mock_millis = 0;
    size_t read_position = 0;

    size_t read_array(uint8_t *data, size_t len) override {
        size_t bytes_read = 0;
        while (bytes_read < len && read_position < uart_buffer.size()) {
            data[bytes_read++] = uart_buffer[read_position++];
        }
        return bytes_read;
    }

    size_t available() override {
        return uart_buffer.size() - read_position;
    }

    void write_array(const uint8_t *data, size_t len) override {
        std::vector<uint8_t> frame(data, data + len);
        sent_frames.push_back(frame);
    }

    uint8_t peek() override {
        if (read_position < uart_buffer.size()) {
            return uart_buffer[read_position];
        }
        return 0;
    }

    uint8_t read() override {
        if (read_position < uart_buffer.size()) {
            return uart_buffer[read_position++];
        }
        return 0;
    }

    unsigned long get_millis() override {
        return mock_millis;
    }

    void log_debug(const char* tag, const char* format, ...) override {
        #ifdef VERBOSE_TESTS
        va_list args;
        va_start(args, format);
        printf("[%s] ", tag);
        vprintf(format, args);
        printf("\n");
        va_end(args);
        #else
        (void)tag;
        (void)format;
        #endif
    }

    std::string format_hex_pretty(const uint8_t *data, size_t len) override {
        std::string result;
        for (size_t i = 0; i < len; i++) {
            char buf[10];
            snprintf(buf, sizeof(buf), "0x%02X", data[i]);
            if (i > 0) result += " ";
            result += buf;
        }
        return result;
    }

    // Helper methods for tests
    void add_incoming_frame(const uint8_t* data, size_t len) {
        uart_buffer.insert(uart_buffer.end(), data, data + len);
    }

    void clear_sent_frames() {
        sent_frames.clear();
    }

    void reset_uart_buffer() {
        uart_buffer.clear();
        read_position = 0;
    }
};

// ============================================================================
// Mock State Callback for Tests
// ============================================================================

class MockStateCallback : public SharpAcStateCallback {
public:
    int state_update_count = 0;
    int ion_update_count = 0;
    int vane_h_update_count = 0;
    int vane_v_update_count = 0;

    void on_state_update() override {
        state_update_count++;
    }

    void on_ion_state_update(bool state) override {
        (void)state;
        ion_update_count++;
    }

    void on_vane_horizontal_update(SwingHorizontal val) override {
        (void)val;
        vane_h_update_count++;
    }

    void on_vane_vertical_update(SwingVertical val) override {
        (void)val;
        vane_v_update_count++;
    }

    void reset_counters() {
        state_update_count = 0;
        ion_update_count = 0;
        vane_h_update_count = 0;
        vane_v_update_count = 0;
    }
};

// ============================================================================
// Test Helper Functions
// ============================================================================

void print_test_header(const char* test_name) {
    std::cout << "\n=== Test: " << test_name << " ===" << std::endl;
}

void print_test_result(const char* test_name, bool passed) {
    if (passed) {
        std::cout << "✓ " << test_name << " passed" << std::endl;
    } else {
        std::cout << "✗ " << test_name << " FAILED" << std::endl;
    }
}

// ============================================================================
// Test Cases
// ============================================================================

/**
 * Test 1: Initialization
 * Verifies that SharpAcCore initializes correctly
 */
bool test_initialization() {
    print_test_header("Initialization");
    
    MockHardwareInterface hw;
    MockStateCallback callback;
    SharpAcCore core(&hw, &callback);
    
    core.setup();
    
    // State should initially be at default values
    const SharpState& state = core.getState();
    
    bool passed = true;
    passed &= (state.state == false);  // Initial aus
    passed &= (state.temperature == 25);  // Default-Temperatur
    
    print_test_result("Initialization", passed);
    return passed;
}

/**
 * Test 2: Init Sequence
 * Verifies that the initialization sequence runs correctly
 * Based on the sequence from working_example
 */
bool test_init_sequence() {
    print_test_header("Init Sequence");
    
    MockHardwareInterface hw;
    MockStateCallback callback;
    SharpAcCore core(&hw, &callback);
    
    core.setup();
    
    // Starte Init
    hw.mock_millis = 0;
    core.loop();
    
    // Should send init_msg
    bool passed = (hw.sent_frames.size() >= 1);
    
    if (passed) {
        // Check if init_msg was sent
        const auto& first_frame = hw.sent_frames[0];
        passed &= (first_frame.size() > 0);
        passed &= (first_frame[0] == init_msg[0]);  // Should start with header
    }
    
    print_test_result("Init Sequence", passed);
    return passed;
}

/**
 * Test 3: Frame Parsing - Cool Mode
 * Verifies that Cool mode frames are parsed correctly
 */
bool test_parse_cool_mode() {
    print_test_header("Parse Cool Mode Frame");
    
    // Frame from logs/Modes.txt: Cool mode
    uint8_t cool_frame[] = {0xdd, 0x0b, 0xfb, 0x60, 0xcf, 0x31, 0x32, 0x00, 0xf9, 0x80, 0x00, 0xe4, 0x81, 0x8a};
    
    SharpModeFrame frame(cool_frame);
    
    bool passed = true;
    passed &= (frame.getPowerMode() == PowerMode::cool);
    passed &= (frame.getState() == true);  // 0x31 = On
    passed &= (frame.getTemperature() == 31);  // 0xcf = 31°C
    
    print_test_result("Parse Cool Mode Frame", passed);
    return passed;
}

/**
 * Test 4: Frame Parsing - Eco Mode
 * Verifies that Eco mode frames are parsed correctly
 */
bool test_parse_eco_mode() {
    print_test_header("Parse Eco Mode Frame");
    
    // Frame from logs/Modes.txt: Cool Eco
    uint8_t eco_frame[] = {0xdd, 0x0b, 0xfb, 0x60, 0xcf, 0x61, 0x32, 0x10, 0xf9, 0x80, 0x00, 0xf4, 0xd1, 0xea};
    
    SharpModeFrame frame(eco_frame);
    
    bool passed = true;
    passed &= (frame.getPowerMode() == PowerMode::cool);
    passed &= (frame.getState() == true);  // 0x61 = On with Preset
    passed &= (frame.getPreset() == Preset::ECO);  // 0x10 = Eco
    passed &= (frame.getTemperature() == 31);
    
    print_test_result("Parse Eco Mode Frame", passed);
    return passed;
}

/**
 * Test 5: Frame Parsing - Full Power Mode
 * Verifies that Full Power frames are parsed correctly
 */
bool test_parse_full_power_mode() {
    print_test_header("Parse Full Power Mode Frame");
    
    // Frame from logs/Modes.txt: Full Power (Cool + Full Power)
    uint8_t fullpower_frame[] = {0xdd, 0x0b, 0xfb, 0x60, 0xcf, 0x61, 0x22, 0x00, 0xf8, 0x80, 0x01, 0xe4, 0xc1, 0x2a};
    
    SharpModeFrame frame(fullpower_frame);
    
    bool passed = true;
    passed &= (frame.getPowerMode() == PowerMode::cool);  // byte[6]=0x22, lower nibble 0x2 = Cool
    passed &= (frame.getState() == true);
    passed &= (frame.getPreset() == Preset::FULLPOWER);  // byte[10]=0x01 for command frames
    passed &= (frame.getTemperature() == 31);
    
    print_test_result("Parse Full Power Mode Frame", passed);
    return passed;
}

/**
 * Test 6: Frame Parsing - Fan Modes
 * Verifiziert, dass verschiedene Lüfterstufen korrekt geparst werden
 */
bool test_parse_fan_modes() {
    print_test_header("Parse Fan Modes");
    
    bool passed = true;
    
    // Test Auto Fan (aus Cool frame)
    uint8_t cool_frame[] = {0xdd, 0x0b, 0xfb, 0x60, 0xcf, 0x31, 0x32, 0x00, 0xf9, 0x80, 0x00, 0xe4, 0x81, 0x8a};
    SharpModeFrame frame1(cool_frame);
    // Byte 6 = 0x32: bits 0-3 sind Fan Mode
    // 0x32 & 0x0F = 0x02 = Mid
    passed &= (frame1.getFanMode() == FanMode::mid);
    
    print_test_result("Parse Fan Modes", passed);
    return passed;
}

/**
 * Test 7: Frame Parsing - Temperature Range
 * Verifies that temperatures in the range 16-30°C are parsed correctly
 */
bool test_parse_temperature_range() {
    print_test_header("Parse Temperature Range");
    
    bool passed = true;
    
    // Temperatur = (byte[4] & 0x0F) + 16
    // 16°C: 0xC0, 31°C: 0xCF
    
    uint8_t frame_16c[] = {0xdd, 0x0b, 0xfb, 0x60, 0xc0, 0x31, 0x32, 0x00, 0xf9, 0x80, 0x00, 0xe4, 0x81, 0x8a};
    SharpModeFrame test_16c(frame_16c);
    passed &= (test_16c.getTemperature() == 16);
    
    uint8_t frame_25c[] = {0xdd, 0x0b, 0xfb, 0x60, 0xc9, 0x31, 0x32, 0x00, 0xf9, 0x80, 0x00, 0xe4, 0x81, 0x8a};
    SharpModeFrame test_25c(frame_25c);
    passed &= (test_25c.getTemperature() == 25);
    
    uint8_t frame_30c[] = {0xdd, 0x0b, 0xfb, 0x60, 0xce, 0x31, 0x32, 0x00, 0xf9, 0x80, 0x00, 0xe4, 0x81, 0x8a};
    SharpModeFrame test_30c(frame_30c);
    passed &= (test_30c.getTemperature() == 30);
    
    print_test_result("Parse Temperature Range", passed);
    return passed;
}

/**
 * Test 8: Frame Parsing - Swing Modes
 * Verifies that swing modes are parsed correctly
 */
bool test_parse_swing_modes() {
    print_test_header("Parse Swing Modes");
    
    // Frame with swing info (byte 8 = 0xf9 for command frames)
    // byte[8] = 0xf9: SwingV = 0x9 = highest, SwingH = 0xF = swing
    uint8_t frame[] = {0xdd, 0x0b, 0xfb, 0x60, 0xcf, 0x31, 0x32, 0x00, 0xf9, 0x80, 0x00, 0xe4, 0x81, 0x8a};
    SharpModeFrame test_frame(frame);
    
    bool passed = true;
    SwingHorizontal h = test_frame.getSwingHorizontal();
    SwingVertical v = test_frame.getSwingVertical();
    
    // Validate correct swing values from frame
    passed &= (v == SwingVertical::highest);  // 0x9 from byte[8] lower nibble
    passed &= (h == SwingHorizontal::swing);  // 0xF from byte[8] upper nibble
    
    print_test_result("Parse Swing Modes", passed);
    return passed;
}

/**
 * Test 9: processUpdate Integration
 * Verifies that processUpdate correctly updates the state
 */
bool test_process_update() {
    print_test_header("Process Update Integration");
    
    MockHardwareInterface hw;
    MockStateCallback callback;
    SharpAcCore core(&hw, &callback);
    
    core.setup();
    
    // Simulate successful initialization through multiple loop() calls
    // Status must be 8 for processUpdate to be called
    for (int i = 0; i < 20; i++) {
        hw.mock_millis += 100;  // Move time forward
        core.loop();
    }
    
    hw.reset_uart_buffer();
    callback.reset_counters();
    
    // Simulate incoming 14-byte response mode frame (Cool Mode)
    // Frame from logs/Modes.txt: Response frame with Cool mode
    uint8_t response_frame[] = {0xdc, 0x0b, 0xfc, 0x73, 0x1a, 0x22, 0x18, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0xb2};
    std::cout << "  Debug: Adding " << sizeof(response_frame) << " bytes to UART buffer" << std::endl;
    hw.add_incoming_frame(response_frame, sizeof(response_frame));
    std::cout << "  Debug: UART buffer has " << hw.available() << " bytes available" << std::endl;
    
    // Trigger loop to process
    core.loop();
    
    const SharpState& state = core.getState();
    
    std::cout << "  Debug: state_update_count = " << callback.state_update_count << std::endl;
    std::cout << "  Debug: state.mode = " << static_cast<int>(state.mode) << " (Cool=2)" << std::endl;
    std::cout << "  Debug: state.temperature = " << state.temperature << std::endl;
    
    bool passed = true;
    passed &= (callback.state_update_count > 0 || state.mode == PowerMode::cool);
    
    print_test_result("Process Update Integration", passed);
    return passed;
}

/**
 * Test 10: Control Mode
 * Verifies that controlMode works correctly
 */
bool test_control_mode() {
    print_test_header("Control Mode");
    
    MockHardwareInterface hw;
    MockStateCallback callback;
    SharpAcCore core(&hw, &callback);
    
    core.setup();
    hw.clear_sent_frames();
    
    // Set mode to Cool
    core.controlMode(PowerMode::cool, true);
    
    // Should send a frame
    bool passed = (hw.sent_frames.size() > 0);
    
    if (passed) {
        const SharpState& state = core.getState();
        passed &= (state.mode == PowerMode::cool);
        passed &= (state.state == true);
    }
    
    print_test_result("Control Mode", passed);
    return passed;
}

/**
 * Test 11: Control Temperature
 * Verifies that controlTemperature works correctly
 */
bool test_control_temperature() {
    print_test_header("Control Temperature");
    
    MockHardwareInterface hw;
    MockStateCallback callback;
    SharpAcCore core(&hw, &callback);
    
    core.setup();
    hw.clear_sent_frames();
    
    // Set temperature
    core.controlTemperature(24);
    
    const SharpState& state = core.getState();
    bool passed = (state.temperature == 24);
    
    // Should send a frame
    passed &= (hw.sent_frames.size() > 0);
    
    print_test_result("Control Temperature", passed);
    return passed;
}

/**
 * Test 12: Control Fan
 * Verifies that controlFan works correctly
 */
bool test_control_fan() {
    print_test_header("Control Fan");
    
    MockHardwareInterface hw;
    MockStateCallback callback;
    SharpAcCore core(&hw, &callback);
    
    core.setup();
    hw.clear_sent_frames();
    
    core.controlFan(FanMode::high);
    
    const SharpState& state = core.getState();
    bool passed = (state.fan == FanMode::high);
    
    passed &= (hw.sent_frames.size() > 0);
    
    print_test_result("Control Fan", passed);
    return passed;
}

/**
 * Test 13: Control Preset
 * Verifies that controlPreset works correctly
 */
bool test_control_preset() {
    print_test_header("Control Preset");
    
    MockHardwareInterface hw;
    MockStateCallback callback;
    SharpAcCore core(&hw, &callback);
    
    core.setup();
    hw.clear_sent_frames();
    
    core.controlPreset(Preset::ECO);
    
    const SharpState& state = core.getState();
    bool passed = (state.preset == Preset::ECO);
    
    passed &= (hw.sent_frames.size() > 0);
    
    print_test_result("Control Preset", passed);
    return passed;
}

/**
 * Test 14: Ion Control
 * Verifies that setIon works correctly
 */
bool test_ion_control() {
    print_test_header("Ion Control");
    
    MockHardwareInterface hw;
    MockStateCallback callback;
    SharpAcCore core(&hw, &callback);
    
    core.setup();
    hw.clear_sent_frames();
    
    core.setIon(true);
    
    const SharpState& state = core.getState();
    bool passed = (state.ion == true);
    
    print_test_result("Ion Control", passed);
    return passed;
}

/**
 * Test 15: Vane Control
 * Verifies that vane control works correctly
 */
bool test_vane_control() {
    print_test_header("Vane Control");
    
    MockHardwareInterface hw;
    MockStateCallback callback;
    SharpAcCore core(&hw, &callback);
    
    core.setup();
    hw.clear_sent_frames();
    
    core.setVaneHorizontal(SwingHorizontal::swing);
    core.setVaneVertical(SwingVertical::swing);
    
    const SharpState& state = core.getState();
    bool passed = true;
    passed &= (state.swingH == SwingHorizontal::swing);
    passed &= (state.swingV == SwingVertical::swing);
    
    print_test_result("Vane Control", passed);
    return passed;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     Sharp AC Core Unit Tests (Refactored Component)       ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    
    int passed = 0;
    int total = 0;
    
    #define RUN_TEST(test_func) \
        total++; \
        if (test_func()) passed++;
    
    // Frame Parsing Tests
    RUN_TEST(test_initialization);
    RUN_TEST(test_init_sequence);
    RUN_TEST(test_parse_cool_mode);
    RUN_TEST(test_parse_eco_mode);
    RUN_TEST(test_parse_full_power_mode);
    RUN_TEST(test_parse_fan_modes);
    RUN_TEST(test_parse_temperature_range);
    RUN_TEST(test_parse_swing_modes);
    
    // Integration Tests
    RUN_TEST(test_process_update);
    
    // Control Tests
    RUN_TEST(test_control_mode);
    RUN_TEST(test_control_temperature);
    RUN_TEST(test_control_fan);
    RUN_TEST(test_control_preset);
    RUN_TEST(test_ion_control);
    RUN_TEST(test_vane_control);
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                      Test Summary                          ║" << std::endl;
    std::cout << "╠════════════════════════════════════════════════════════════╣" << std::endl;
    printf("║  Tests Passed: %2d / %2d                                      ║\n", passed, total);
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    
    if (passed == total) {
        std::cout << "\n✓✓✓ All tests passed! ✓✓✓\n" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗✗✗ Some tests failed! ✗✗✗\n" << std::endl;
        return 1;
    }
}
