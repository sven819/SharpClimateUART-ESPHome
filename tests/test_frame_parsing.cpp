#include <cassert>
#include <cstdio>
#include <cstring>
#include "core_frame.h"
#include "core_state.h"
#include "core_types.h"

void printBytes(const char* label, const uint8_t* arr, size_t size) {
    printf("%s: ", label);
    for (size_t i = 0; i < size; i++) {
        printf("0x%02x ", arr[i]);
    }
    printf("\n");
}

// Test 1: Cool mode (31°C)
void test_cool_31c() {
    printf("\n=== Test: Cool 31°C ===\n");
    const uint8_t frame[] = {0xdd, 0x0b, 0xfb, 0x60, 0xcf, 0x31, 0x32, 0x00, 0xf9, 0x80, 0x00, 0xe4, 0x81, 0x8a};
    printBytes("Frame", frame, 14);
    
    SharpModeFrame f(frame);
    
    assert(f.validateChecksum() == true);
    assert(f.getTemperature() == 31);
    assert(f.getState() == true);
    assert(f.getPowerMode() == PowerMode::cool);
    assert(f.getFanMode() == FanMode::mid);
    assert(f.getPreset() == Preset::NONE);
    // Byte 8 = 0xf9: lower=0x9 (highest), upper=0xF (swing)
    assert(f.getSwingVertical() == SwingVertical::highest);
    assert(f.getSwingHorizontal() == SwingHorizontal::swing);
    
    printf("✓ Temperature: %d°C\n", f.getTemperature());
    printf("✓ State: %s\n", f.getState() ? "ON" : "OFF");
    printf("✓ Mode: Cool (0x%02x)\n", (int)f.getPowerMode());
    printf("✓ Fan: Mid (0x%02x)\n", (int)f.getFanMode());
    printf("✓ Preset: NONE\n");
    printf("✓ SwingV: Highest (0x9), SwingH: Swing (0xF)\n");
    printf("✓ Checksum: VALID\n");
    printf("✓ PASSED\n");
}

// Test 2: Cool Eco mode
void test_cool_eco() {
    printf("\n=== Test: Cool Eco 31°C ===\n");
    const uint8_t frame[] = {0xdd, 0x0b, 0xfb, 0x60, 0xcf, 0x61, 0x32, 0x10, 0xf9, 0x80, 0x00, 0xf4, 0xd1, 0xea};
    printBytes("Frame", frame, 14);
    
    SharpModeFrame f(frame);
    
    assert(f.validateChecksum() == true);
    assert(f.getTemperature() == 31);
    assert(f.getState() == true);
    assert(f.getPowerMode() == PowerMode::cool);
    assert(f.getFanMode() == FanMode::mid);
    assert(f.getPreset() == Preset::ECO);
    
    printf("✓ Temperature: %d°C\n", f.getTemperature());
    printf("✓ Preset: ECO (byte[7]=0x%02x)\n", frame[7]);
    printf("✓ Checksum: VALID\n");
    printf("✓ PASSED\n");
}

// Test 3: Cool Eco 25°C
void test_cool_eco_25c() {
    printf("\n=== Test: Cool Eco 25°C ===\n");
    const uint8_t frame[] = {0xdd, 0x0b, 0xfb, 0x60, 0xc9, 0x61, 0x22, 0x0c, 0xf8, 0x00, 0x20, 0x1c, 0xa1, 0x6d};
    printBytes("Frame", frame, 14);
    
    SharpModeFrame f(frame);
    
    assert(f.validateChecksum() == true);
    assert(f.getTemperature() == 25);
    assert(f.getState() == true);
    assert(f.getPowerMode() == PowerMode::cool);
    assert(f.getFanMode() == FanMode::auto_fan);
    
    printf("✓ Temperature: %d°C (byte[4]=0x%02x -> %d+16)\n", 
           f.getTemperature(), frame[4], frame[4] & 0x0F);
    printf("✓ Fan: Auto (0x%02x)\n", (int)f.getFanMode());
    printf("✓ PASSED\n");
}

// Test 4: Full Power 31°C
void test_full_power_31c() {
    printf("\n=== Test: Full Power 31°C ===\n");
    const uint8_t frame[] = {0xdd, 0x0b, 0xfb, 0x60, 0xcf, 0x61, 0x22, 0x00, 0xf8, 0x80, 0x01, 0xe4, 0xc1, 0x2a};
    printBytes("Frame", frame, 14);
    
    SharpModeFrame f(frame);
    
    assert(f.validateChecksum() == true);
    assert(f.getTemperature() == 31);
    assert(f.getState() == true);
    assert(f.getPowerMode() == PowerMode::cool);
    assert(f.getFanMode() == FanMode::auto_fan);
    assert(f.getPreset() == Preset::FULLPOWER);
    assert(f.getSwingVertical() == SwingVertical::auto_position);
    
    printf("✓ Temperature: %d°C\n", f.getTemperature());
    printf("✓ Preset: FULLPOWER (byte[10]=0x%02x)\n", frame[10]);
    printf("✓ SwingV: Auto Position (0x%02x)\n", (int)f.getSwingVertical());
    printf("✓ PASSED\n");
}

// Test 5: Full Power 25°C
void test_full_power_25c() {
    printf("\n=== Test: Full Power 25°C ===\n");
    const uint8_t frame[] = {0xdd, 0x0b, 0xfb, 0x60, 0xc9, 0x61, 0x22, 0x00, 0xf8, 0x80, 0x01, 0xe4, 0xa1, 0x50};
    printBytes("Frame", frame, 14);
    
    SharpModeFrame f(frame);
    
    assert(f.validateChecksum() == true);
    assert(f.getTemperature() == 25);
    assert(f.getPreset() == Preset::FULLPOWER);
    
    printf("✓ Temperature: %d°C\n", f.getTemperature());
    printf("✓ Preset: FULLPOWER\n");
    printf("✓ PASSED\n");
}

// Test 6: Full Power ohne Cluster 17°C
void test_full_power_no_cluster() {
    printf("\n=== Test: Full Power ohne Cluster 17°C ===\n");
    const uint8_t frame[] = {0xdd, 0x0b, 0xfb, 0x60, 0xc1, 0x71, 0x22, 0x0c, 0xf8, 0x00, 0x21, 0x10, 0xe1, 0x30};
    printBytes("Frame", frame, 14);
    
    SharpModeFrame f(frame);
    
    assert(f.validateChecksum() == true);
    assert(f.getTemperature() == 17);
    
    printf("✓ Temperature: %d°C (byte[4]=0x%02x -> %d+16)\n", 
           f.getTemperature(), frame[4], frame[4] & 0x0F);
    printf("✓ PASSED\n");
}

// Test 7: Eco ohne Cluster 25°C
void test_eco_no_cluster() {
    printf("\n=== Test: Eco ohne Cluster 25°C ===\n");
    const uint8_t frame[] = {0xdd, 0x0b, 0xfb, 0x60, 0xc9, 0x61, 0x22, 0x10, 0xf8, 0x80, 0x00, 0xf0, 0xf1, 0xe5};
    printBytes("Frame", frame, 14);
    
    SharpModeFrame f(frame);
    
    assert(f.validateChecksum() == true);
    assert(f.getTemperature() == 25);
    assert(f.getPreset() == Preset::ECO);
    
    printf("✓ Temperature: %d°C\n", f.getTemperature());
    printf("✓ Preset: ECO (byte[7]=0x%02x)\n", frame[7]);
    printf("✓ PASSED\n");
}

// Test 8: Command Generation
void test_command_generation() {
    printf("\n=== Test: Command Generation ===\n");
    
    // Cool mode, 25°C
    SharpState state;
    state.state = true;
    state.mode = PowerMode::cool;
    state.fan = FanMode::mid;
    state.temperature = 25;
    state.swingV = SwingVertical::swing;
    state.swingH = SwingHorizontal::swing;
    state.ion = false;
    state.preset = Preset::NONE;
    
    SharpCommandFrame cmd;
    cmd.setData(&state);
    cmd.setChecksum();
    
    printBytes("Generated", cmd.getData(), cmd.getSize());
    
    printf("  Byte[4] (Temp): 0x%02x (should be 0xC0+(25-15)=0xCA)\n", cmd.getData()[4]);
    printf("  Byte[5] (State): 0x%02x (should be 0x31 for ON)\n", cmd.getData()[5]);
    printf("  Byte[6] (Mode): 0x%02x (should have cool+mid)\n", cmd.getData()[6]);
    printf("  Byte[8] (Swing): 0x%02x\n", cmd.getData()[8]);
    
    assert(cmd.validateChecksum() == true);
    printf("✓ Checksum: VALID\n");
    printf("✓ PASSED\n");
}

// Test 9: All temperatures 16-31°C
void test_all_temperatures() {
    printf("\n=== Test: Temperature Range 16-31°C ===\n");
    
    for (int temp = 16; temp <= 31; temp++) {
        uint8_t frame[14] = {0xdd, 0x0b, 0xfb, 0x60, 0xc0, 0x31, 0x32, 0x00, 0xf9, 0x80, 0x00, 0xe4, 0x81, 0x00};
        
        // Set temperature in byte 4
        frame[4] = 0xC0 | (temp - 16);
        
        // Recalculate checksum
        uint16_t sum = 0;
        for (int i = 1; i < 13; i++) {
            sum += frame[i];
            sum &= 0xFF;
        }
        frame[13] = (uint8_t)((256 - sum) & 0xFF);
        
        SharpModeFrame f(frame);
        int parsed = f.getTemperature();
        
        if (parsed != temp) {
            printf("✗ Temperature %d°C failed: parsed as %d\n", temp, parsed);
            assert(false);
        }
    }
    
    printf("✓ All temperatures 16-31°C parsed correctly\n");
    printf("✓ PASSED\n");
}

int main() {
    printf("\n");
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║   Sharp AC Unit Tests - Based on Real Frames      ║\n");
    printf("║   All test data from logs/Modes.txt               ║\n");
    printf("╚════════════════════════════════════════════════════╝\n");
    
    test_cool_31c();
    test_cool_eco();
    test_cool_eco_25c();
    test_full_power_31c();
    test_full_power_25c();
    test_full_power_no_cluster();
    test_eco_no_cluster();
    test_all_temperatures();
    test_command_generation();
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║            ✓ ALL TESTS PASSED ✓                   ║\n");
    printf("╚════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    return 0;
}
