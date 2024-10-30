-- Copyright Configured Things Ltd and CHERIoT Contributors.
-- SPDX-License-Identifier: MIT


-- Consumer compartments 
compartment("rgb_led")
    add_includedirs("../..")
    add_files("rgb_led.cc")

compartment("user_led")
    add_includedirs("../..")
    add_files("user_led.cc")

compartment("lcd")
    add_includedirs("../..")
    add_files("lcd.cc")
    add_deps("cxxrt")
    -- LCD drivers
    add_files("../third_party/display_drivers/core/lcd_base.c")
    add_files("../third_party/display_drivers/core/lucida_console_12pt.c")
    add_files("../third_party/display_drivers/st7735/lcd_st7735.c")
    add_files("../third_party/display_drivers/lcd.cc")
