// Copyright Configured Things Ltd and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

/**
 * Update a configuration item using the JSON string
 * received on a particular topic. With a real MQTT
 * client this would be the callback registered when
 * subscribing to the topic.
 */
int updateConfig(const char *topic,
                 size_t      topicLength,
                 const void *payload,
                 size_t      payloadLength);
