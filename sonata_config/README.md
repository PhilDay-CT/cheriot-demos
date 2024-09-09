Safe Configuration Management (Sonata)
======================================

Contributed by Configured Things Ltd

Build options:
```
 --IPv6=n --network-inject-faults=y --network-force-non-unique-mac=y --tls-rsa=y --scheduler-accounting=y
```

tty
```
picocom /dev/ttyUSB2 -b 115200 --imap lfcrlf
```

This is an example of the configurtaion broker modified to work on a Sonata board

- The rgb_led and user_led drivers use the Sonata MMIO



- The logger consfiguration item is replaced with an LCD item
- Updtaes come from an external MQTT server via the network

Ideally we would reuse:

- config_broker directly from ../configurtion_broker, but the lack of an init mechanism means that it has a dependency on parser_init() in "../parser/parser.h" which includes the logger_parser that we don't want

- the user_LED and rgb_LED parsers from ../configuration_broker, but that have a larger min update interval than we want

So reuse is currenlty limited to the JSON parser 
