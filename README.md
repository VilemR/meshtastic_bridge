This is a simple demonstration of a naive one-way bridge between different Meshtastic networks (e.g., LongFast to MediumFast). The code is designed for the Wireless Stick V3 board with a display, which was used for development. However, it can be easily modified and ported to any ESP32-based board, even without a display.

To prevent communication collisions between the two Meshtastic networks, each endpoint (MF or LF) operates on a dedicated frequency range. In Europe, the default frequency is 869.525 MHz, so the second Meshtastic network being bridged should use a different frequency—869.775 MHz in this case (+250 kHz, considering LoRa bandwidth).

The device follows a straightforward process: it listens on one frequency, and when a packet is received, it switches the LoRa radio to the second frequency (adjusting the corresponding spreading factor) and retransmits the packet. Immediately afterward, the radio is tuned back to the original frequency, ready to bridge the next packet.
