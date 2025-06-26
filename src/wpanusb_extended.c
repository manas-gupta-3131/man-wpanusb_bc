/*
 * Copyright (c) 2025 BeagleBoard.org Foundation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "wpanusb_extended.h"

LOG_MODULE_REGISTER(wpanusb_ext, CONFIG_LOG_DEFAULT_LEVEL);

/* CC1352 specific registers and definitions */
#define CC1352_FCFG1_BASE           0x50001000
#define CC1352_IEEE_MAC_0           (CC1352_FCFG1_BASE + 0x2F0)
#define CC1352_IEEE_MAC_1           (CC1352_FCFG1_BASE + 0x2F4)

/* Channel definitions for different pages */
static const uint32_t page0_channels_2_4ghz = 0x07FFF800; /* Channels 11-26 */
static const uint32_t page2_channels_sub_ghz = 0x000007FF; /* Channels 0-10 for 915MHz */

int wpanusb_get_extended_addr(uint8_t *addr)
{
    if (!addr) {
        LOG_ERR("Invalid address pointer");
        return -EINVAL;
    }

    /* Try to read IEEE address from hardware registers */
    uint32_t mac_addr_0, mac_addr_1;
    
    /* Read from CC1352 FCFG1 registers */
    mac_addr_0 = sys_read32(CC1352_IEEE_MAC_0);
    mac_addr_1 = sys_read32(CC1352_IEEE_MAC_1);

    LOG_DBG("Read MAC registers: 0x%08x, 0x%08x", mac_addr_0, mac_addr_1);

    /* Check if valid IEEE address exists */
    if (mac_addr_0 != 0xFFFFFFFF && mac_addr_1 != 0xFFFFFFFF && 
        mac_addr_0 != 0x00000000 && mac_addr_1 != 0x00000000) {
        
        /* Extract 64-bit IEEE address from registers */
        /* CC1352 stores IEEE address in little-endian format */
        addr[0] = (mac_addr_1 >> 24) & 0xFF;
        addr[1] = (mac_addr_1 >> 16) & 0xFF;
        addr[2] = (mac_addr_1 >> 8) & 0xFF;
        addr[3] = mac_addr_1 & 0xFF;
        addr[4] = (mac_addr_0 >> 24) & 0xFF;
        addr[5] = (mac_addr_0 >> 16) & 0xFF;
        addr[6] = (mac_addr_0 >> 8) & 0xFF;
        addr[7] = mac_addr_0 & 0xFF;

        LOG_INF("Read IEEE address from hardware: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
        return 0;
    }

    LOG_WRN("No valid IEEE address in hardware registers");

    /* Fallback: try to read from device ID */
    uint8_t dev_id[16];
    ssize_t dev_id_len = hwinfo_get_device_id(dev_id, sizeof(dev_id));
    
    if (dev_id_len >= 8) {
        memcpy(addr, dev_id, 8);
        /* Set locally administered bit (bit 1 of first octet) */
        addr[0] |= 0x02;
        /* Clear group bit (bit 0 of first octet) to ensure unicast */
        addr[0] &= ~0x01;
        
        LOG_INF("Generated IEEE address from device ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
        return 0;
    }

    LOG_WRN("Device ID too short (%d bytes), using fallback", dev_id_len);

    /* Last resort: generate fallback address */
    return wpanusb_generate_fallback_addr(addr);
}

int wpanusb_get_supported_channels(uint8_t page, uint32_t *channels)
{
    if (!channels) {
        LOG_ERR("Invalid channels pointer");
        return -EINVAL;
    }

    switch (page) {
    case WPANUSB_PAGE_0_2_4GHZ:
        /* 2.4 GHz band: channels 11-26 */
        *channels = page0_channels_2_4ghz;
        LOG_DBG("Page 0 (2.4GHz) channels: 0x%08x", *channels);
        return 0;

    case WPANUSB_PAGE_2_SUB_GHZ:
        /* Sub-GHz band: check if supported */
#ifdef CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ
        *channels = page2_channels_sub_ghz;
        LOG_DBG("Page 2 (Sub-GHz) channels: 0x%08x", *channels);
        return 0;
#else
        LOG_WRN("Sub-GHz not supported in this build");
        *channels = 0;
        return -ENOTSUP;
#endif

    default:
        LOG_ERR("Unsupported channel page: %d", page);
        *channels = 0;
        return -EINVAL;
    }
}

int wpanusb_get_tx_power_levels(int8_t *min_power, int8_t *max_power, 
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

#ifdef CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ
    /* Sub-GHz can go higher */
    if (*num_steps < 8) {
        power_steps[6] = 10;
        power_steps[7] = 14;
        *num_steps = 8;
        *max_power = 14;
    }
#endif

    LOG_DBG("TX power range: %d to %d dBm, %d steps", *min_power, *max_power, *num_steps);
    return 0;
}

int wpanusb_get_device_capabilities(uint32_t *capabilities)
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
#ifdef CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ
    *capabilities |= WPANUSB_CAP_SUB_GHZ;
    *capabilities |= WPANUSB_CAP_MULTI_PAGE;
#endif

    LOG_INF("Device capabilities: 0x%08x", *capabilities);
    return 0;
}

int wpanusb_generate_fallback_addr(uint8_t *addr)
{
    if (!addr) {
        LOG_ERR("Invalid address pointer");
        return -EINVAL;
    }

    /* Generate a random IEEE address with proper OUI */
    uint32_t random_val1 = sys_rand32_get();
    uint32_t random_val2 = sys_rand32_get();
    
    /* Use a locally administered OUI based on BeagleBoard.org */
    addr[0] = 0x02;  /* Locally administered, unicast */
    addr[1] = 0xBE;  /* BeagleBoard.org inspired */
    addr[2] = 0xAC;  /* AC for "BeagleConnect" */
    
    /* Fill remaining bytes with random data */
    addr[3] = (random_val1 >> 24) & 0xFF;
    addr[4] = (random_val1 >> 16) & 0xFF;
    addr[5] = (random_val1 >> 8) & 0xFF;
    addr[6] = random_val1 & 0xFF;
    addr[7] = random_val2 & 0xFF;

    LOG_WRN("Generated fallback IEEE address: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
    
    return 0;
}
