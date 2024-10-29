--Copyright Configured Things Ltd and CHERIoT Contributors.
--SPDX - License -Identifier : MIT

-- Configuration Broker 
debugOption("config_broker")
compartment("config_broker")
    add_rules("cheriot.component-debug")
    add_files("config_broker.cc")
