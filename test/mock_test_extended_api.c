/*
 * Mock Testing Framework for WPANUSB Extended API
 * Copyright (c) 2025 BeagleBoard.org Foundation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

/* Mock Zephyr types and macros */
#define BIT(n) (1U << (n))
#define __packed __attribute__((packed))
#define LOG_INF(fmt, ...) printf("[INF] " fmt "\n", ##__VA_ARGS__)
#define LOG_WRN(fmt, ...) printf("[WRN] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) printf("[ERR] " fmt "\n", ##__VA_ARGS__)
#define LOG_DBG(fmt, ...) printf("[DBG] " fmt "\n", ##__VA_ARGS__)

/* Mock error codes */
#define EINVAL 22
#define ENOTSUP 95

/* Device capability flags from header */
#define WPANUSB_CAP_2_4GHZ          BIT(0)
#define WPANUSB_CAP_SUB_GHZ         BIT(1)
#define WPANUSB_CAP_MULTI_PAGE      BIT(2)
#define WPANUSB_CAP_PROMISCUOUS     BIT(3)
#define WPANUSB_CAP_ED_SCAN         BIT(4)
#define WPANUSB_CAP_CSMA_CA         BIT(5)
#define WPANUSB_CAP_AUTO_ACK        BIT(6)
#define WPANUSB_CAP_TX_POWER_CTRL   BIT(7)
#define WPANUSB_CAP_CCA_MODE_CTRL   BIT(8)
#define WPANUSB_CAP_FRAME_FILTERING BIT(9)

/* Multi-page channel support */
#define WPANUSB_PAGE_0_2_4GHZ       0  /* 2.4 GHz, channels 11-26 */
#define WPANUSB_PAGE_2_SUB_GHZ      2  /* Sub-GHz, 868/915 MHz */

/* Response structures */
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

/* Global test configuration */
static int mock_sub_ghz_enabled = 1;  /* Set to 0 to test without Sub-GHz */
static int mock_hardware_ieee_addr = 1;  /* Set to 0 to test fallback */

/* Mock implementations of extended API functions */

int mock_wpanusb_get_extended_addr(uint8_t *addr)
{
    if (!addr) {
        LOG_ERR("Invalid address pointer");
        return -EINVAL;
    }

    if (mock_hardware_ieee_addr) {
        /* Simulate reading from CC1352 hardware registers */
        LOG_DBG("Reading IEEE address from CC1352 FCFG1 registers");
        
        /* Mock hardware IEEE address (TI OUI: 00:12:4B) */
        addr[0] = 0x00;
        addr[1] = 0x12;
        addr[2] = 0x4B;
        addr[3] = 0xFF;
        addr[4] = 0xFE;
        addr[5] = 0x12;
        addr[6] = 0x34;
        addr[7] = 0x56;
        
        LOG_INF("Read IEEE address from hardware: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
        return 0;
    } else {
        /* Simulate fallback address generation */
        LOG_WRN("No valid IEEE address in hardware registers, generating fallback");
        
        /* Use locally administered OUI (02:BE:AC) */
        addr[0] = 0x02;  /* Locally administered, unicast */
        addr[1] = 0xBE;  /* BeagleBoard.org inspired */
        addr[2] = 0xAC;  /* AC for "BeagleConnect" */
        
        /* Fill remaining bytes with pseudo-random data */
        srand(time(NULL));
        addr[3] = rand() & 0xFF;
        addr[4] = rand() & 0xFF;
        addr[5] = rand() & 0xFF;
        addr[6] = rand() & 0xFF;
        addr[7] = rand() & 0xFF;
        
        LOG_WRN("Generated fallback IEEE address: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
        return 0;
    }
}

int mock_wpanusb_get_supported_channels(uint8_t page, uint32_t *channels)
{
    if (!channels) {
        LOG_ERR("Invalid channels pointer");
        return -EINVAL;
    }

    switch (page) {
    case WPANUSB_PAGE_0_2_4GHZ:
        /* 2.4 GHz band: channels 11-26 */
        *channels = 0x07FFF800;  /* Channels 11-26 */
        LOG_DBG("Page 0 (2.4GHz) channels: 0x%08x", *channels);
        return 0;

    case WPANUSB_PAGE_2_SUB_GHZ:
        /* Sub-GHz band: check if supported */
        if (mock_sub_ghz_enabled) {
            *channels = 0x000007FF;  /* Channels 0-10 */
            LOG_DBG("Page 2 (Sub-GHz) channels: 0x%08x", *channels);
            return 0;
        } else {
            LOG_WRN("Sub-GHz not supported in this build");
            *channels = 0;
            return -ENOTSUP;
        }

    default:
        LOG_ERR("Unsupported channel page: %d", page);
        *channels = 0;
        return -EINVAL;
    }
}

int mock_wpanusb_get_tx_power_levels(int8_t *min_power, int8_t *max_power, 
                                    int8_t *power_steps, uint8_t *num_steps)
{
    if (!min_power || !max_power || !power_steps || !num_steps) {
        LOG_ERR("Invalid power level parameters");
        return -EINVAL;
    }

    /* CC1352 TX power capabilities */
    *min_power = -20;  /* -20 dBm minimum */
    *max_power = 5;    /* +5 dBm maximum for 2.4GHz */
    
    /* Power levels available: -20, -15, -10, -5, 0, 5 dBm */
    *num_steps = 6;
    power_steps[0] = -20;
    power_steps[1] = -15;
    power_steps[2] = -10;
    power_steps[3] = -5;
    power_steps[4] = 0;
    power_steps[5] = 5;

    if (mock_sub_ghz_enabled) {
        /* Sub-GHz can go higher */
        power_steps[6] = 10;
        power_steps[7] = 14;
        *num_steps = 8;
        *max_power = 14;
    }

    LOG_DBG("TX power range: %d to %d dBm, %d steps", *min_power, *max_power, *num_steps);
    return 0;
}

int mock_wpanusb_get_device_capabilities(uint32_t *capabilities)
{
    if (!capabilities) {
        LOG_ERR("Invalid capabilities pointer");
        return -EINVAL;
    }

    *capabilities = 0;

    /* Base capabilities */
    *capabilities |= WPANUSB_CAP_2_4GHZ;
    *capabilities |= WPANUSB_CAP_PROMISCUOUS;
    *capabilities |= WPANUSB_CAP_ED_SCAN;
    *capabilities |= WPANUSB_CAP_CSMA_CA;
    *capabilities |= WPANUSB_CAP_AUTO_ACK;
    *capabilities |= WPANUSB_CAP_TX_POWER_CTRL;
    *capabilities |= WPANUSB_CAP_CCA_MODE_CTRL;
    *capabilities |= WPANUSB_CAP_FRAME_FILTERING;

    /* Check for Sub-GHz support */
    if (mock_sub_ghz_enabled) {
        *capabilities |= WPANUSB_CAP_SUB_GHZ;
        *capabilities |= WPANUSB_CAP_MULTI_PAGE;
    }

    LOG_INF("Device capabilities: 0x%08x", *capabilities);
    return 0;
}

/* Test functions */

void test_get_extended_addr(void)
{
    uint8_t ieee_addr[8];
    int ret;

    LOG_INF("=== Testing get_extended_addr ===");
    
    /* Test with hardware address */
    mock_hardware_ieee_addr = 1;
    ret = mock_wpanusb_get_extended_addr(ieee_addr);
    if (ret == 0) {
        LOG_INF("✓ Hardware IEEE address test PASSED");
        
        /* Validate it's a proper IEEE address */
        if (ieee_addr[0] == 0x00 && ieee_addr[1] == 0x12 && ieee_addr[2] == 0x4B) {
            LOG_INF("✓ Valid TI OUI detected");
        }
    } else {
        LOG_ERR("✗ Hardware IEEE address test FAILED: %d", ret);
    }
    
    /* Test fallback address generation */
    mock_hardware_ieee_addr = 0;
    ret = mock_wpanusb_get_extended_addr(ieee_addr);
    if (ret == 0) {
        LOG_INF("✓ Fallback address generation test PASSED");
        
        /* Validate locally administered bit */
        if ((ieee_addr[0] & 0x02) != 0) {
            LOG_INF("✓ Locally administered bit set correctly");
        } else {
            LOG_ERR("✗ Locally administered bit not set");
        }
        
        /* Validate unicast address */
        if ((ieee_addr[0] & 0x01) == 0) {
            LOG_INF("✓ Unicast address confirmed");
        } else {
            LOG_ERR("✗ Group address detected (should be unicast)");
        }
    } else {
        LOG_ERR("✗ Fallback address generation test FAILED: %d", ret);
    }
    
    /* Test null pointer */
    ret = mock_wpanusb_get_extended_addr(NULL);
    if (ret == -EINVAL) {
        LOG_INF("✓ NULL pointer validation test PASSED");
    } else {
        LOG_ERR("✗ NULL pointer should return -EINVAL, got: %d", ret);
    }
}

void test_get_supported_channels(void)
{
    uint32_t channels;
    int ret;

    LOG_INF("=== Testing get_supported_channels ===");
    
    /* Test 2.4GHz channels */
    ret = mock_wpanusb_get_supported_channels(WPANUSB_PAGE_0_2_4GHZ, &channels);
    if (ret == 0) {
        LOG_INF("✓ 2.4GHz channels test PASSED: 0x%08x", channels);
        
        /* Validate expected channel mask */
        if (channels == 0x07FFF800) {
            LOG_INF("✓ Expected 2.4GHz channel mask confirmed");
        } else {
            LOG_WRN("✗ Unexpected 2.4GHz channel mask: 0x%08x", channels);
        }
    } else {
        LOG_ERR("✗ 2.4GHz channels test FAILED: %d", ret);
    }
    
    /* Test Sub-GHz channels with support enabled */
    mock_sub_ghz_enabled = 1;
    ret = mock_wpanusb_get_supported_channels(WPANUSB_PAGE_2_SUB_GHZ, &channels);
    if (ret == 0) {
        LOG_INF("✓ Sub-GHz channels test PASSED: 0x%08x", channels);
    } else {
        LOG_ERR("✗ Sub-GHz channels test FAILED: %d", ret);
    }
    
    /* Test Sub-GHz channels with support disabled */
    mock_sub_ghz_enabled = 0;
    ret = mock_wpanusb_get_supported_channels(WPANUSB_PAGE_2_SUB_GHZ, &channels);
    if (ret == -ENOTSUP) {
        LOG_INF("✓ Sub-GHz disabled test PASSED (correctly returned -ENOTSUP)");
    } else {
        LOG_ERR("✗ Sub-GHz disabled test FAILED: expected -ENOTSUP, got %d", ret);
    }
    
    /* Test invalid page */
    ret = mock_wpanusb_get_supported_channels(99, &channels);
    if (ret == -EINVAL) {
        LOG_INF("✓ Invalid page validation test PASSED");
    } else {
        LOG_ERR("✗ Invalid page should return -EINVAL, got: %d", ret);
    }
    
    /* Test null pointer */
    ret = mock_wpanusb_get_supported_channels(0, NULL);
    if (ret == -EINVAL) {
        LOG_INF("✓ NULL pointer validation test PASSED");
    } else {
        LOG_ERR("✗ NULL pointer should return -EINVAL, got: %d", ret);
    }
}

void test_get_tx_power_levels(void)
{
    int8_t min_power, max_power, power_steps[16];
    uint8_t num_steps;
    int ret;

    LOG_INF("=== Testing get_tx_power_levels ===");
    
    /* Test with Sub-GHz enabled */
    mock_sub_ghz_enabled = 1;
    ret = mock_wpanusb_get_tx_power_levels(&min_power, &max_power, power_steps, &num_steps);
    if (ret == 0) {
        LOG_INF("✓ TX power levels (Sub-GHz) test PASSED");
        LOG_INF("  Range: %d to %d dBm, %d steps", min_power, max_power, num_steps);
        
        /* Validate power range */
        if (min_power == -20 && max_power == 14 && num_steps == 8) {
            LOG_INF("✓ Expected Sub-GHz power range confirmed");
        } else {
            LOG_WRN("✗ Unexpected Sub-GHz power range");
        }
        
        /* Print power steps */
        for (int i = 0; i < num_steps && i < 16; i++) {
            LOG_INF("  Step %d: %d dBm", i, power_steps[i]);
        }
    } else {
        LOG_ERR("✗ TX power levels (Sub-GHz) test FAILED: %d", ret);
    }
    
    /* Test with Sub-GHz disabled */
    mock_sub_ghz_enabled = 0;
    ret = mock_wpanusb_get_tx_power_levels(&min_power, &max_power, power_steps, &num_steps);
    if (ret == 0) {
        LOG_INF("✓ TX power levels (2.4GHz only) test PASSED");
        LOG_INF("  Range: %d to %d dBm, %d steps", min_power, max_power, num_steps);
        
        /* Validate power range */
        if (min_power == -20 && max_power == 5 && num_steps == 6) {
            LOG_INF("✓ Expected 2.4GHz power range confirmed");
        } else {
            LOG_WRN("✗ Unexpected 2.4GHz power range");
        }
    } else {
        LOG_ERR("✗ TX power levels (2.4GHz only) test FAILED: %d", ret);
    }
    
    /* Test null pointers */
    ret = mock_wpanusb_get_tx_power_levels(NULL, &max_power, power_steps, &num_steps);
    if (ret == -EINVAL) {
        LOG_INF("✓ NULL min_power validation test PASSED");
    } else {
        LOG_ERR("✗ NULL min_power should return -EINVAL, got: %d", ret);
    }
}

void test_get_device_capabilities(void)
{
    uint32_t capabilities;
    int ret;

    LOG_INF("=== Testing get_device_capabilities ===");
    
    /* Test with Sub-GHz enabled */
    mock_sub_ghz_enabled = 1;
    ret = mock_wpanusb_get_device_capabilities(&capabilities);
    if (ret == 0) {
        LOG_INF("✓ Device capabilities (Sub-GHz) test PASSED: 0x%08x", capabilities);
        
        /* Check expected capabilities */
        if (capabilities & WPANUSB_CAP_2_4GHZ) {
            LOG_INF("  ✓ 2.4GHz support confirmed");
        }
        if (capabilities & WPANUSB_CAP_SUB_GHZ) {
            LOG_INF("  ✓ Sub-GHz support confirmed");
        }
        if (capabilities & WPANUSB_CAP_MULTI_PAGE) {
            LOG_INF("  ✓ Multi-page support confirmed");
        }
        if (capabilities & WPANUSB_CAP_PROMISCUOUS) {
            LOG_INF("  ✓ Promiscuous mode support confirmed");
        }
        if (capabilities & WPANUSB_CAP_CSMA_CA) {
            LOG_INF("  ✓ CSMA-CA support confirmed");
        }
        if (capabilities & WPANUSB_CAP_TX_POWER_CTRL) {
            LOG_INF("  ✓ TX power control support confirmed");
        }
    } else {
        LOG_ERR("✗ Device capabilities (Sub-GHz) test FAILED: %d", ret);
    }
    
    /* Test with Sub-GHz disabled */
    mock_sub_ghz_enabled = 0;
    ret = mock_wpanusb_get_device_capabilities(&capabilities);
    if (ret == 0) {
        LOG_INF("✓ Device capabilities (2.4GHz only) test PASSED: 0x%08x", capabilities);
        
        /* Should have 2.4GHz but not Sub-GHz */
        if (capabilities & WPANUSB_CAP_2_4GHZ) {
            LOG_INF("  ✓ 2.4GHz support confirmed");
        }
        if (!(capabilities & WPANUSB_CAP_SUB_GHZ)) {
            LOG_INF("  ✓ Sub-GHz correctly disabled");
        }
        if (!(capabilities & WPANUSB_CAP_MULTI_PAGE)) {
            LOG_INF("  ✓ Multi-page correctly disabled");
        }
    } else {
        LOG_ERR("✗ Device capabilities (2.4GHz only) test FAILED: %d", ret);
    }
    
    /* Test null pointer */
    ret = mock_wpanusb_get_device_capabilities(NULL);
    if (ret == -EINVAL) {
        LOG_INF("✓ NULL pointer validation test PASSED");
    } else {
        LOG_ERR("✗ NULL pointer should return -EINVAL, got: %d", ret);
    }
}

int main(void)
{
    printf("\n");
    printf("==========================================================\n");
    printf("WPANUSB Extended API Mock Test Suite\n");
    printf("BeagleConnect Freedom - Non-Hardware Testing\n");
    printf("==========================================================\n");
    printf("\n");
    
    /* Run all tests */
    test_get_extended_addr();
    printf("\n");
    
    test_get_supported_channels();
    printf("\n");
    
    test_get_tx_power_levels();
    printf("\n");
    
    test_get_device_capabilities();
    printf("\n");
    
    printf("==========================================================\n");
    printf("Extended API Mock Test Suite Complete\n");
    printf("==========================================================\n");
    
    return 0;
}
