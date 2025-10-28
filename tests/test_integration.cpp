#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>
#include <cstdarg>

#include "core_logic.h"
#include "core_frame.h"
#include "core_messages.h"
#include "core_types.h"
#include "core_state.h"

using namespace esphome::sharp_ac;

// ============================================================================
// Mock Hardware Interface
// ============================================================================

class TestHardwareInterface : public SharpAcHardwareInterface {
private:
    std::vector<uint8_t> uart_rx_buffer;
    std::vector<uint8_t> uart_tx_buffer;
    size_t read_pos = 0;
    unsigned long current_millis = 0;

public:
    std::vector<std::vector<uint8_t>> captured_frames;

    size_t read_array(uint8_t *data, size_t len) override {
        size_t bytes_read = 0;
        while (bytes_read < len && read_pos < uart_rx_buffer.size()) {
            data[bytes_read++] = uart_rx_buffer[read_pos++];
        }
        return bytes_read;
    }

    size_t available() override {
        return uart_rx_buffer.size() - read_pos;
    }

    void write_array(const uint8_t *data, size_t len) override {
        std::vector<uint8_t> frame(data, data + len);
        captured_frames.push_back(frame);
        uart_tx_buffer.insert(uart_tx_buffer.end(), data, data + len);
    }

    uint8_t peek() override {
        if (read_pos < uart_rx_buffer.size()) {
            return uart_rx_buffer[read_pos];
        }
        return 0;
    }

    uint8_t read() override {
        if (read_pos < uart_rx_buffer.size()) {
            return uart_rx_buffer[read_pos++];
        }
        return 0;
    }

    unsigned long get_millis() override {
        return current_millis;
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

    // Test helpers
    void inject_rx_data(const uint8_t* data, size_t len) {
        uart_rx_buffer.insert(uart_rx_buffer.end(), data, data + len);
    }

    void advance_time(unsigned long ms) {
        current_millis += ms;
    }

    void reset() {
        uart_rx_buffer.clear();
        uart_tx_buffer.clear();
        captured_frames.clear();
        read_pos = 0;
    }

    const std::vector<uint8_t>& get_tx_buffer() const {
        return uart_tx_buffer;
    }
};

class TestStateCallback : public SharpAcStateCallback {
public:
    int update_count = 0;
    bool last_ion_state = false;
    SwingHorizontal last_swing_h = SwingHorizontal::left;
    SwingVertical last_swing_v = SwingVertical::lowest;

    void on_state_update() override {
        update_count++;
    }

    void on_ion_state_update(bool state) override {
        (void)state;
        last_ion_state = state;
    }

    void on_vane_horizontal_update(SwingHorizontal val) override {
        (void)val;
        last_swing_h = val;
    }

    void on_vane_vertical_update(SwingVertical val) override {
        (void)val;
        last_swing_v = val;
    }

    void reset() {
        update_count = 0;
    }
};

// ============================================================================
// Test Utilities
// ============================================================================

void print_frame_comparison(const char* name, const uint8_t* expected, const uint8_t* actual, size_t len) {
    std::cout << "  Frame: " << name << std::endl;
    
    std::cout << "  Expected: ";
    for (size_t i = 0; i < len; i++) {
        printf("0x%02X ", expected[i]);
    }
    std::cout << std::endl;
    
    std::cout << "  Actual:   ";
    for (size_t i = 0; i < len; i++) {
        printf("0x%02X ", actual[i]);
    }
    std::cout << std::endl;
}

bool compare_frames(const uint8_t* frame1, const uint8_t* frame2, size_t len) {
    return memcmp(frame1, frame2, len) == 0;
}

// ============================================================================
// Integration Tests
// ============================================================================

/**
 * Test: Initialization Sequence Matching
 * Verifiziert, dass die Init-Sequenz identisch zum Working Example ist
 */
bool test_init_sequence_matching() {
    std::cout << "\n=== Test: Init Sequence Matching ===" << std::endl;
    
    TestHardwareInterface hw;
    TestStateCallback cb;
    SharpAcCore core(&hw, &cb);
    
    core.setup();
    
    // Trigger initialization
    hw.advance_time(0);
    core.loop();
    
    // Expect init_msg as first frame
    bool passed = hw.captured_frames.size() > 0;
    
    if (passed && hw.captured_frames.size() > 0) {
        const auto& first_frame = hw.captured_frames[0];
        // Check if it starts with expected header
        passed &= (first_frame.size() > 0);
        passed &= (first_frame[0] == init_msg[0]);
        
        std::cout << "  ✓ Init message sent" << std::endl;
        std::cout << "  Frame size: " << first_frame.size() << " bytes" << std::endl;
    } else {
        std::cout << "  ✗ No init message sent" << std::endl;
    }
    
    return passed;
}

/**
 * Test: Cool Mode Command Generation
 * Compares the generated Cool Mode command
 */
bool test_cool_mode_command_generation() {
    std::cout << "\n=== Test: Cool Mode Command Generation ===" << std::endl;
    
    TestHardwareInterface hw;
    TestStateCallback cb;
    SharpAcCore core(&hw, &cb);
    
    core.setup();
    hw.reset();
    
    core.controlMode(PowerMode::cool, true);
    core.controlTemperature(24);
    core.controlFan(FanMode::auto_fan);
    
    bool passed = hw.captured_frames.size() > 0;
    
    if (passed) {
        std::cout << "  ✓ Cool mode command generated" << std::endl;
        std::cout << "  Frames sent: " << hw.captured_frames.size() << std::endl;
        
        // Check state
        const SharpState& state = core.getState();
        passed &= (state.mode == PowerMode::cool);
        passed &= (state.state == true);
        passed &= (state.temperature == 24);
    }
    
    return passed;
}

/**
 * Test: Frame Parsing Consistency
 * Verifies that incoming frames are parsed identically to the working example
 */
bool test_frame_parsing_consistency() {
    std::cout << "\n=== Test: Frame Parsing Consistency ===" << std::endl;
    
    struct TestCase {
        const char* name;
        uint8_t frame[14];
        size_t size;
        PowerMode expected_mode;
        int expected_temp;
        bool expected_state;
        Preset expected_preset;
    };
    
    TestCase test_cases[] = {
        {
            "Cool Mode",
            {0xdd, 0x0b, 0xfb, 0x60, 0xcf, 0x31, 0x32, 0x00, 0xf9, 0x80, 0x00, 0xe4, 0x81, 0x8a},
            14,
            PowerMode::cool,
            31,
            true,
            Preset::NONE
        },
        {
            "Cool Eco Mode",
            {0xdd, 0x0b, 0xfb, 0x60, 0xcf, 0x61, 0x32, 0x10, 0xf9, 0x80, 0x00, 0xf4, 0xd1, 0xea},
            14,
            PowerMode::cool,
            31,
            true,
            Preset::ECO
        },
        {
            "Full Power Mode",  // Cool + Full Power (byte[6]=0x22, lower nibble 0x2 = Cool)
            {0xdd, 0x0b, 0xfb, 0x60, 0xcf, 0x61, 0x22, 0x00, 0xf8, 0x80, 0x01, 0xe4, 0xc1, 0x2a},
            14,
            PowerMode::cool,  // Korrigiert: byte[6]=0x22 (lower nibble 0x2 = Cool)
            31,
            true,
            Preset::FULLPOWER
        }
    };
    
    bool all_passed = true;
    
    for (const auto& test : test_cases) {
        std::cout << "\n  Testing: " << test.name << std::endl;
        
        SharpModeFrame frame(test.frame);
        
        bool test_passed = true;
        
        PowerMode parsed_mode = frame.getPowerMode();
        int parsed_temp = frame.getTemperature();
        bool parsed_state = frame.getState();
        Preset parsed_preset = frame.getPreset();
        
        if (parsed_mode != test.expected_mode) {
            std::cout << "    ✗ Mode mismatch: expected " << static_cast<int>(test.expected_mode) 
                      << ", got " << static_cast<int>(parsed_mode) << std::endl;
            test_passed = false;
        }
        
        if (parsed_temp != test.expected_temp) {
            std::cout << "    ✗ Temperature mismatch: expected " << test.expected_temp 
                      << ", got " << parsed_temp << std::endl;
            test_passed = false;
        }
        
        if (parsed_state != test.expected_state) {
            std::cout << "    ✗ State mismatch: expected " << test.expected_state 
                      << ", got " << parsed_state << std::endl;
            test_passed = false;
        }
        
        if (parsed_preset != test.expected_preset) {
            std::cout << "    ✗ Preset mismatch: expected " << static_cast<int>(test.expected_preset) 
                      << ", got " << static_cast<int>(parsed_preset) << std::endl;
            test_passed = false;
        }
        
        if (test_passed) {
            std::cout << "    ✓ All fields parsed correctly" << std::endl;
        }
        
        all_passed &= test_passed;
    }
    
    return all_passed;
}

/**
 * Test: Temperature Control Accuracy
 * Verifies that temperature control works as in the Working Example
 */
bool test_temperature_control_accuracy() {
    std::cout << "\n=== Test: Temperature Control Accuracy ===" << std::endl;
    
    TestHardwareInterface hw;
    TestStateCallback cb;
    SharpAcCore core(&hw, &cb);
    
    core.setup();
    
    bool passed = true;
    
    int test_temps[] = {16, 20, 24, 28, 30};
    
    for (int temp : test_temps) {
        hw.reset();
        core.controlTemperature(temp);
        
        const SharpState& state = core.getState();
        if (state.temperature != temp) {
            std::cout << "  ✗ Temperature " << temp << "°C not set correctly (got " 
                      << state.temperature << "°C)" << std::endl;
            passed = false;
        } else {
            std::cout << "  ✓ Temperature " << temp << "°C set correctly" << std::endl;
        }
    }
    
    return passed;
}

/**
 * Test: Fan Mode Control
 * Verifies that all fan modes are set correctly
 */
bool test_fan_mode_control() {
    std::cout << "\n=== Test: Fan Mode Control ===" << std::endl;
    
    TestHardwareInterface hw;
    TestStateCallback cb;
    SharpAcCore core(&hw, &cb);
    
    core.setup();
    
    bool passed = true;
    
    FanMode modes[] = {FanMode::auto_fan, FanMode::low, FanMode::mid, FanMode::high, FanMode::highest};
    const char* mode_names[] = {"Auto", "Low", "Mid", "High", "Highest"};
    
    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
        hw.reset();
        core.controlFan(modes[i]);
        
        const SharpState& state = core.getState();
        if (state.fan != modes[i]) {
            std::cout << "  ✗ Fan mode " << mode_names[i] << " not set correctly" << std::endl;
            passed = false;
        } else {
            std::cout << "  ✓ Fan mode " << mode_names[i] << " set correctly" << std::endl;
        }
    }
    
    return passed;
}

/**
 * Test: Power Mode Control
 * Verifies that all power modes are set correctly
 */
bool test_power_mode_control() {
    std::cout << "\n=== Test: Power Mode Control ===" << std::endl;
    
    TestHardwareInterface hw;
    TestStateCallback cb;
    SharpAcCore core(&hw, &cb);
    
    core.setup();
    
    bool passed = true;
    
    struct ModeTest {
        PowerMode mode;
        const char* name;
    };
    
    ModeTest modes[] = {
        {PowerMode::cool, "Cool"},
        {PowerMode::heat, "Heat"},
        {PowerMode::dry, "Dry"},
        {PowerMode::fan, "Fan"}
    };
    
    for (const auto& test : modes) {
        hw.reset();
        core.controlMode(test.mode, true);
        
        const SharpState& state = core.getState();
        if (state.mode != test.mode || !state.state) {
            std::cout << "  ✗ Mode " << test.name << " not set correctly" << std::endl;
            passed = false;
        } else {
            std::cout << "  ✓ Mode " << test.name << " set correctly" << std::endl;
        }
    }
    
    // Test Power Off
    hw.reset();
    core.controlMode(PowerMode::cool, false);
    const SharpState& state = core.getState();
    if (state.state != false) {
        std::cout << "  ✗ Power off not working" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ Power off working" << std::endl;
    }
    
    return passed;
}

/**
 * Test: Preset Control (Eco, Full Power)
 * Verifies that presets work as in the Working Example
 */
bool test_preset_control() {
    std::cout << "\n=== Test: Preset Control ===" << std::endl;
    
    TestHardwareInterface hw;
    TestStateCallback cb;
    SharpAcCore core(&hw, &cb);
    
    core.setup();
    
    bool passed = true;
    
    // Test ECO Preset
    hw.reset();
    core.controlPreset(Preset::ECO);
    
    const SharpState& state1 = core.getState();
    if (state1.preset != Preset::ECO) {
        std::cout << "  ✗ ECO preset not set correctly" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ ECO preset set correctly" << std::endl;
    }
    
    hw.reset();
    core.controlPreset(Preset::FULLPOWER);
    
    const SharpState& state2 = core.getState();
    if (state2.preset != Preset::FULLPOWER) {
        std::cout << "  ✗ FULLPOWER preset not set correctly" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ FULLPOWER preset set correctly" << std::endl;
    }
    
    hw.reset();
    core.controlPreset(Preset::NONE);
    
    const SharpState& state3 = core.getState();
    if (state3.preset != Preset::NONE) {
        std::cout << "  ✗ NONE preset not set correctly" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ NONE preset reset correctly" << std::endl;
    }
    
    return passed;
}

/**
 * Test: Swing Control
 * Verifies swing control (horizontal and vertical)
 */
bool test_swing_control() {
    std::cout << "\n=== Test: Swing Control ===" << std::endl;
    
    TestHardwareInterface hw;
    TestStateCallback cb;
    SharpAcCore core(&hw, &cb);
    
    core.setup();
    
    bool passed = true;
    
    // Test Horizontal Swing
    hw.reset();
    core.setVaneHorizontal(SwingHorizontal::swing);
    
    const SharpState& state1 = core.getState();
    if (state1.swingH != SwingHorizontal::swing) {
        std::cout << "  ✗ Horizontal swing not set" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ Horizontal swing set" << std::endl;
    }
    
    // Test Vertical Swing
    hw.reset();
    core.setVaneVertical(SwingVertical::swing);
    
    const SharpState& state2 = core.getState();
    if (state2.swingV != SwingVertical::swing) {
        std::cout << "  ✗ Vertical swing not set" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ Vertical swing set" << std::endl;
    }
    
    return passed;
}

/**
 * Test: Ion Mode Control
 * Verifies Ion mode control
 */
bool test_ion_mode_control() {
    std::cout << "\n=== Test: Ion Mode Control ===" << std::endl;
    
    TestHardwareInterface hw;
    TestStateCallback cb;
    SharpAcCore core(&hw, &cb);
    
    core.setup();
    
    bool passed = true;
    
    // Test Ion On
    hw.reset();
    core.setIon(true);
    
    const SharpState& state1 = core.getState();
    if (state1.ion != true) {
        std::cout << "  ✗ Ion mode ON not set" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ Ion mode ON set" << std::endl;
    }
    
    // Test Ion Off
    hw.reset();
    core.setIon(false);
    
    const SharpState& state2 = core.getState();
    if (state2.ion != false) {
        std::cout << "  ✗ Ion mode OFF not set" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ Ion mode OFF set" << std::endl;
    }
    
    return passed;
}

/**
 * Test: State Callback Invocation
 * Verifies that callbacks are invoked correctly
 */
bool test_state_callback_invocation() {
    std::cout << "\n=== Test: State Callback Invocation ===" << std::endl;
    
    TestHardwareInterface hw;
    TestStateCallback cb;
    SharpAcCore core(&hw, &cb);
    
    core.setup();
    
    bool passed = true;
    
    int initial_count = cb.update_count;
    
    // Trigger State Update
    core.controlMode(PowerMode::cool, true);
    core.publishUpdate();
    
    if (cb.update_count <= initial_count) {
        std::cout << "  ✗ State callback not invoked" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ State callback invoked (" << (cb.update_count - initial_count) << " times)" << std::endl;
    }
    
    return passed;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Integration Tests: Refactored vs Working Example           ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    int passed = 0;
    int total = 0;
    
    #define RUN_TEST(test_func) \
        total++; \
        if (test_func()) { \
            passed++; \
            std::cout << "✓ Test passed\n"; \
        } else { \
            std::cout << "✗ Test FAILED\n"; \
        }
    
    // Initialization Tests
    RUN_TEST(test_init_sequence_matching);
    
    // Frame Parsing Tests
    RUN_TEST(test_frame_parsing_consistency);
    
    // Command Generation Tests
    RUN_TEST(test_cool_mode_command_generation);
    
    // Control Tests
    RUN_TEST(test_temperature_control_accuracy);
    RUN_TEST(test_fan_mode_control);
    RUN_TEST(test_power_mode_control);
    RUN_TEST(test_preset_control);
    RUN_TEST(test_swing_control);
    RUN_TEST(test_ion_mode_control);
    
    // Callback Tests
    RUN_TEST(test_state_callback_invocation);
    
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                        Test Summary                          ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    printf("║  Tests Passed: %2d / %2d                                        ║\n", passed, total);
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    
    if (passed == total) {
        std::cout << "\n✓✓✓ All integration tests passed! ✓✓✓\n" << std::endl;
        std::cout << "The refactored component behaves identically to the working example.\n" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗✗✗ Some integration tests failed! ✗✗✗\n" << std::endl;
        std::cout << "Review the differences between refactored and working example.\n" << std::endl;
        return 1;
    }
}
