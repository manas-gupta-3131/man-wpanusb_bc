/*
 * Copyright (c) 2025 BeagleBoard.org Foundation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "wpanusb_extended.h"

LOG_MODULE_REGISTER(wpanusb_test, CONFIG_LOG_DEFAULT_LEVEL);

static void test_get_extended_addr(void)
{
	uint8_t ieee_addr[8];
	int ret;

	LOG_INF("Testing get_extended_addr...");
	
	ret = wpanusb_get_extended_addr(ieee_addr);
	if (ret == 0) {
		LOG_INF("✓ IEEE address: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		        ieee_addr[0], ieee_addr[1], ieee_addr[2], ieee_addr[3],
		        ieee_addr[4], ieee_addr[5], ieee_addr[6], ieee_addr[7]);
		
		/* Validate locally administered bit for fallback addresses */
		if ((ieee_addr[0] & 0x02) != 0) {
			LOG_INF("✓ Locally administered address detected");
		}
		
		/* Validate unicast address */
		if ((ieee_addr[0] & 0x01) == 0) {
			LOG_INF("✓ Unicast address confirmed");
		} else {
			LOG_WRN("✗ Group address detected (should be unicast)");
		}
	} else {
		LOG_ERR("✗ get_extended_addr failed: %d", ret);
	}
}

static void test_get_supported_channels(void)
{
	uint32_t channels;
	int ret;

	LOG_INF("Testing get_supported_channels...");
	
	/* Test 2.4GHz page */
	ret = wpanusb_get_supported_channels(WPANUSB_PAGE_0_2_4GHZ, &channels);
	if (ret == 0) {
		LOG_INF("✓ 2.4GHz channels (page 0): 0x%08x", channels);
		
		/* Validate expected 2.4GHz channels (11-26) */
		if (channels == 0x07FFF800) {
			LOG_INF("✓ Expected 2.4GHz channel mask confirmed");
		} else {
			LOG_WRN("✗ Unexpected 2.4GHz channel mask: 0x%08x", channels);
		}
	} else {
		LOG_ERR("✗ get_supported_channels (2.4GHz) failed: %d", ret);
	}

	/* Test Sub-GHz page */
	ret = wpanusb_get_supported_channels(WPANUSB_PAGE_2_SUB_GHZ, &channels);
	if (ret == 0) {
		LOG_INF("✓ Sub-GHz channels (page 2): 0x%08x", channels);
	} else if (ret == -ENOTSUP) {
		LOG_INF("✓ Sub-GHz not supported (expected if not built with Sub-GHz)");
	} else {
		LOG_ERR("✗ get_supported_channels (Sub-GHz) failed: %d", ret);
	}

	/* Test invalid page */
	ret = wpanusb_get_supported_channels(99, &channels);
	if (ret == -EINVAL) {
		LOG_INF("✓ Invalid page correctly rejected");
	} else {
		LOG_ERR("✗ Invalid page should have failed: %d", ret);
	}
}

static void test_get_tx_power_levels(void)
{
	int8_t min_power, max_power, power_steps[16];
	uint8_t num_steps;
	int ret;

	LOG_INF("Testing get_tx_power_levels...");
	
	ret = wpanusb_get_tx_power_levels(&min_power, &max_power, power_steps, &num_steps);
	if (ret == 0) {
		LOG_INF("✓ TX power range: %d to %d dBm, %d steps", 
		        min_power, max_power, num_steps);
		
		/* Validate reasonable power range */
		if (min_power >= -30 && max_power <= 20 && num_steps > 0 && num_steps <= 16) {
			LOG_INF("✓ Power range values are reasonable");
		} else {
			LOG_WRN("✗ Suspicious power range values");
		}
		
		/* Print power steps */
		for (int i = 0; i < num_steps && i < 16; i++) {
			LOG_INF("  Step %d: %d dBm", i, power_steps[i]);
		}
	} else {
		LOG_ERR("✗ get_tx_power_levels failed: %d", ret);
	}
}

static void test_get_device_capabilities(void)
{
	uint32_t capabilities;
	int ret;

	LOG_INF("Testing get_device_capabilities...");
	
	ret = wpanusb_get_device_capabilities(&capabilities);
	if (ret == 0) {
		LOG_INF("✓ Device capabilities: 0x%08x", capabilities);
		
		/* Check expected capabilities */
		if (capabilities & WPANUSB_CAP_2_4GHZ) {
			LOG_INF("✓ 2.4GHz support confirmed");
		}
		
		if (capabilities & WPANUSB_CAP_SUB_GHZ) {
			LOG_INF("✓ Sub-GHz support confirmed");
		}
		
		if (capabilities & WPANUSB_CAP_PROMISCUOUS) {
			LOG_INF("✓ Promiscuous mode support confirmed");
		}
		
		if (capabilities & WPANUSB_CAP_CSMA_CA) {
			LOG_INF("✓ CSMA-CA support confirmed");
		}
		
		if (capabilities & WPANUSB_CAP_TX_POWER_CTRL) {
			LOG_INF("✓ TX power control support confirmed");
		}
	} else {
		LOG_ERR("✗ get_device_capabilities failed: %d", ret);
	}
}

static void test_generate_fallback_addr(void)
{
	uint8_t addr1[8], addr2[8];
	int ret1, ret2;

	LOG_INF("Testing generate_fallback_addr...");
	
	ret1 = wpanusb_generate_fallback_addr(addr1);
	ret2 = wpanusb_generate_fallback_addr(addr2);
	
	if (ret1 == 0 && ret2 == 0) {
		LOG_INF("✓ Fallback address 1: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		        addr1[0], addr1[1], addr1[2], addr1[3],
		        addr1[4], addr1[5], addr1[6], addr1[7]);
		LOG_INF("✓ Fallback address 2: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		        addr2[0], addr2[1], addr2[2], addr2[3],
		        addr2[4], addr2[5], addr2[6], addr2[7]);
		
		/* Validate addresses are different (randomness) */
		if (memcmp(addr1, addr2, 8) != 0) {
			LOG_INF("✓ Generated addresses are different (good randomness)");
		} else {
			LOG_WRN("✗ Generated addresses are identical (poor randomness)");
		}
		
		/* Validate locally administered bit */
		if ((addr1[0] & 0x02) != 0 && (addr2[0] & 0x02) != 0) {
			LOG_INF("✓ Locally administered bit set correctly");
		} else {
			LOG_ERR("✗ Locally administered bit not set");
		}
		
		/* Validate unicast addresses */
		if ((addr1[0] & 0x01) == 0 && (addr2[0] & 0x01) == 0) {
			LOG_INF("✓ Unicast addresses confirmed");
		} else {
			LOG_ERR("✗ Group address detected");
		}
	} else {
		LOG_ERR("✗ generate_fallback_addr failed: %d, %d", ret1, ret2);
	}
}

void wpanusb_test_extended_api(void)
{
	LOG_INF("=== Starting Extended WPANUSB API Tests ===");
	
	test_get_extended_addr();
	k_sleep(K_MSEC(100));
	
	test_get_supported_channels();
	k_sleep(K_MSEC(100));
	
	test_get_tx_power_levels();
	k_sleep(K_MSEC(100));
	
	test_get_device_capabilities();
	k_sleep(K_MSEC(100));
	
	test_generate_fallback_addr();
	k_sleep(K_MSEC(100));
	
	LOG_INF("=== Extended WPANUSB API Tests Complete ===");
}

/* Work queue for API testing */
K_WORK_DEFINE(test_work, wpanusb_test_extended_api);

void wpanusb_schedule_api_test(void)
{
	LOG_INF("Scheduling extended API test...");
	k_work_submit(&test_work);
}

/* Auto-run test after 5 seconds */
static void test_timer_handler(struct k_timer *timer)
{
	wpanusb_schedule_api_test();
}

K_TIMER_DEFINE(test_timer, test_timer_handler, NULL);

static int wpanusb_test_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	
	LOG_INF("WPANUSB Extended API Test initialized");
	
	/* Start test timer - run tests after 5 seconds */
	k_timer_start(&test_timer, K_SECONDS(5), K_NO_WAIT);
	
	return 0;
}

/* Initialize test system */
SYS_INIT(wpanusb_test_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
