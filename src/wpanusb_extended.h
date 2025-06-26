/*
 * Copyright (c) 2025 BeagleBoard.org Foundation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WPANUSB_EXTENDED_H_
#define WPANUSB_EXTENDED_H_

#include <zephyr/kernel.h>
#include <zephyr/net/ieee802154.h>

/* Extended API function prototypes */
int wpanusb_get_extended_addr(uint8_t *addr);
int wpanusb_get_supported_channels(uint8_t page, uint32_t *channels);
int wpanusb_get_tx_power_levels(int8_t *min_power, int8_t *max_power, 
                               int8_t *power_steps, uint8_t *num_steps);
int wpanusb_get_device_capabilities(uint32_t *capabilities);
int wpanusb_generate_fallback_addr(uint8_t *addr);

/* Device capability flags */
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

/* Response structures for extended API */
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

#endif /* WPANUSB_EXTENDED_H_ */
