-- Copyright Configured Things Ltd and CHERIoT Contributors.
-- SPDX-License-Identifier: MIT


-- Consumer compartment
compartment("consumers")
    set_default(false)
    add_includedirs("../..")
    --add_files("consumer.cc")
    add_files("led_consumer.cc")
    add_deps("cxxrt", "stdio")
    
    -- LCD drivers
    --add_files("../third_party/display_drivers/core/lcd_base.c")
    --add_files("../third_party/display_drivers/core/lucida_console_12pt.c")
    --add_files("../third_party/display_drivers/st7735/lcd_st7735.c")
    --add_files("../third_party/display_drivers/lcd.cc")



