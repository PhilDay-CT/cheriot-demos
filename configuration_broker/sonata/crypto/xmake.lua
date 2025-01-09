option("CHERIOT")
  set_default(true)
  set_description("Define platform for libhydrogen")

compartment("crypto")
    add_deps("freestanding", "debug", "cxxrt")

    add_includedirs("../third_party/libhydrogen")
    add_files("crypto.cc")
    add_files("rand_32.cc")
    add_files("../third_party/libhydrogen/hydrogen.c")
    
    on_load(function(target)
        target:add('options', "CHERIOT")
        local Cheriot = get_config("CHERIOT")
        target:add("defines", "__cheriot__=" .. tostring(Cheriot))
    end)
