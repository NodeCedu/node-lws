{
    "targets": [{
        "target_name": "lws",
        "sources": [ "addon.cpp", "lws.cpp"],
        "cflags_cc!": ["-fno-exceptions"],
        "cflags_cc": ["-std=c++11", "-fexceptions"],
        "link_settings": {
            "libraries": ["-lwebsockets", "-lev"]
        }
    }]
}
