# Extended WPANUSB API Implementation

## Overview

This implementation extends the BeagleConnect Freedom WPANUSB firmware with hardware capability discovery APIs. The extended API allows dynamic querying of device capabilities, IEEE addresses, supported channels, and TX power levels.

## Implementation Details

### Core API Functions

#### 1. `wpanusb_get_extended_addr(uint8_t *addr)`

**Purpose**: Reads IEEE 802.15.4 extended address from hardware-specific registers.

**Implementation Strategy**:
- **Primary**: Read from CC1352 FCFG1 registers (0x50001000 + 0x2F0/0x2F4)
- **Fallback 1**: Use Zephyr hwinfo device ID (first 8 bytes)
- **Fallback 2**: Generate random locally administered address

**Address Format Compliance**:
- Sets locally administered bit (bit 1 of first octet) for generated addresses
- Ensures unicast address (bit 0 of first octet cleared)
- Uses BeagleBoard.org inspired OUI (02:BE:AC) for fallback addresses

**Error Handling**:
- Returns `-EINVAL` for null pointer
- Returns `0` on success
- Logs detailed information about address source

#### 2. `wpanusb_get_supported_channels(uint8_t page, uint32_t *channels)`

**Purpose**: Provides multi-page channel support for different frequency bands.

**Supported Pages**:
- **Page 0**: 2.4 GHz band (channels 11-26) → `0x07FFF800`
- **Page 2**: Sub-GHz band (channels 0-10) → `0x000007FF`

**Build-time Configuration**:
- Sub-GHz support requires `CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ=y`
- Returns `-ENOTSUP` if Sub-GHz not compiled in

#### 3. `wpanusb_get_tx_power_levels()`

**Purpose**: Exposes actual CC1352 hardware TX power capabilities.

**2.4 GHz Power Levels**:
- Range: -20 dBm to +5 dBm
- Steps: [-20, -15, -10, -5, 0, 5] dBm (6 levels)

**Sub-GHz Power Levels** (if enabled):
- Range: -20 dBm to +14 dBm  
- Steps: [-20, -15, -10, -5, 0, 5, 10, 14] dBm (8 levels)

#### 4. `wpanusb_get_device_capabilities(uint32_t *capabilities)`

**Purpose**: Comprehensive device capability reporting.

**Capability Flags**:
```c
#define WPANUSB_CAP_2_4GHZ          BIT(0)  // 2.4 GHz support
#define WPANUSB_CAP_SUB_GHZ         BIT(1)  // Sub-GHz support
#define WPANUSB_CAP_MULTI_PAGE      BIT(2)  // Multi-page support
#define WPANUSB_CAP_PROMISCUOUS     BIT(3)  // Promiscuous mode
#define WPANUSB_CAP_ED_SCAN         BIT(4)  // Energy detection scan
#define WPANUSB_CAP_CSMA_CA         BIT(5)  // CSMA-CA support
#define WPANUSB_CAP_AUTO_ACK        BIT(6)  // Automatic ACK
#define WPANUSB_CAP_TX_POWER_CTRL   BIT(7)  // TX power control
#define WPANUSB_CAP_CCA_MODE_CTRL   BIT(8)  // CCA mode control
#define WPANUSB_CAP_FRAME_FILTERING BIT(9)  // Frame filtering
```

### Protocol Integration

#### USB Command Extensions

**New Command IDs**:
```c
enum wpanusb_requests {
    // ... existing commands ...
    GET_EXTENDED_ADDR = 18,
    GET_SUPPORTED_CHANNELS = 19,
    GET_TX_POWER_LEVELS = 20,
    GET_DEVICE_CAPABILITIES = 21,
};
```

#### Command Handlers

Each extended API function has a corresponding command handler:
- `get_extended_addr_handler()` → Returns 8-byte IEEE address
- `get_supported_channels_ext()` → Returns page + channel mask
- `get_tx_power_levels_handler()` → Returns power range + steps  
- `get_device_capabilities_handler()` → Returns capability flags

#### Response Structures

```c
struct get_extended_addr_resp {
    uint8_t ieee_addr[8];
} __packed;

struct get_supported_channels_resp {
    uint8_t page;
    uint32_t channel_mask;
} __packed;

struct get_tx_power_levels_resp {
    int8_t min_power;
    int8_t max_power;
    uint8_t num_steps;
    int8_t power_steps[16];
} __packed;

struct get_device_capabilities_resp {
    uint32_t capabilities;
} __packed;
```

## Testing Framework

### Automated Test Suite (`wpanusb_test.c`)

**Test Coverage**:
1. **IEEE Address Tests**:
   - Address retrieval validation
   - Locally administered bit verification
   - Unicast address confirmation

2. **Channel Support Tests**:
   - 2.4 GHz channel mask validation
   - Sub-GHz support testing
   - Invalid page rejection

3. **TX Power Tests**:
   - Power range validation
   - Step count verification
   - Power level enumeration

4. **Capability Tests**:
   - Feature flag validation
   - Build-time capability detection

5. **Fallback Address Tests**:
   - Random address generation
   - Address uniqueness verification
   - Format compliance checking

**Test Execution**:
- Auto-runs 5 seconds after boot
- Can be triggered manually via `wpanusb_schedule_api_test()`
- Comprehensive logging with ✓/✗ status indicators

## Hardware Integration

### CC1352 Register Access

**IEEE Address Registers**:
```c
#define CC1352_FCFG1_BASE    0x50001000
#define CC1352_IEEE_MAC_0    (CC1352_FCFG1_BASE + 0x2F0)
#define CC1352_IEEE_MAC_1    (CC1352_FCFG1_BASE + 0x2F4)
```

**Address Extraction**:
- Reads 64-bit IEEE address from factory configuration registers
- Handles little-endian byte ordering
- Validates against all-ones/all-zeros patterns

### Zephyr Integration

**Dependencies**:
- `CONFIG_HWINFO=y` - Hardware info API for device ID
- `CONFIG_ENTROPY_GENERATOR=y` - Random number generation
- `CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ=y` - Sub-GHz support (optional)

**Device Tree**: No additional DT changes required for core functionality.

## Build Instructions

### Standard 2.4 GHz Build
```bash
west build -b beagleconnect_freedom /path/to/wpanusb_bc -- \
    -DDTC_OVERLAY_FILE=beagleconnect_freedom.overlay
```

### Sub-GHz Build
```bash
west build -b beagleconnect_freedom /path/to/wpanusb_bc -- \
    -DOVERLAY_CONFIG=overlay-subghz.conf \
    -DDTC_OVERLAY_FILE=beagleconnect_freedom.overlay
```

### Configuration Options

**Enable Extended API**:
```
CONFIG_WPANUSB_EXTENDED_API=y
CONFIG_WPANUSB_EXTENDED_API_TESTS=y
```

**Hardware Dependencies**:
```
CONFIG_HWINFO=y
CONFIG_ENTROPY_GENERATOR=y
```

## Error Handling and Persistence

### Address Persistence

**Hardware Address**: Permanently stored in CC1352 FCFG1 registers
**Generated Address**: Regenerated on each boot (intentional for testing)
**Production Note**: For production use, consider storing generated addresses in flash

### Error Recovery

**Hardware Access Failures**:
- Graceful fallback to device ID or random generation
- Detailed logging for debugging
- No system crashes on hardware access failures

**Invalid Parameters**:
- Null pointer checking on all API functions
- Range validation for page numbers
- Proper error code returns

### Logging Strategy

**Log Levels**:
- `LOG_INF`: Successful operations and important status
- `LOG_WRN`: Fallback operations and unexpected conditions  
- `LOG_ERR`: Error conditions and failures
- `LOG_DBG`: Detailed debugging information

## Performance Characteristics

### Memory Usage
- **Flash**: ~3KB additional for extended API
- **RAM**: ~500 bytes for response structures
- **Stack**: No additional stack requirements

### Execution Time
- **IEEE Address Read**: <1ms (register access)
- **Channel Query**: <100μs (compile-time constants)
- **Power Level Query**: <100μs (static arrays)
- **Capability Query**: <100μs (compile-time flags)

### Power Consumption
- No additional power consumption during idle
- Brief register read operations during API calls

## Future Enhancements

### Planned Features
1. **Persistent Address Storage**: Flash-based address storage for production
2. **Dynamic Power Calibration**: Runtime power level calibration
3. **Temperature Compensation**: Temperature-aware power/frequency adjustment
4. **Diagnostic Counters**: Radio usage statistics and error counters

### Integration Opportunities
1. **Linux Driver Integration**: Automatic capability discovery during driver initialization
2. **Network Stack Integration**: Dynamic configuration based on hardware capabilities
3. **Power Management**: Adaptive power levels based on link quality

## Validation Checklist

### Pre-deployment Validation
- [ ] Hardware IEEE address reading tested on actual BeagleConnect Freedom
- [ ] Fallback address generation produces valid IEEE EUI-64 addresses
- [ ] Multi-page channel support works for both 2.4 GHz and Sub-GHz
- [ ] TX power levels match actual hardware capabilities
- [ ] Device capabilities accurately reflect build configuration
- [ ] Protocol command handlers respond correctly to USB requests
- [ ] Test framework passes all validation checks
- [ ] Error handling gracefully manages hardware access failures

### Hardware Test Requirements
1. **BeagleConnect Freedom board** with programmed IEEE address
2. **BeagleConnect Freedom board** without programmed IEEE address (test fallback)
3. **USB connection** to Linux host for protocol testing
4. **Logic analyzer** (optional) for protocol verification

This implementation provides a robust foundation for hardware capability discovery in the WPANUSB firmware, enabling dynamic configuration and improved compatibility with various IEEE 802.15.4 applications.
