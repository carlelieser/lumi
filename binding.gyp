{
  "win_delay_load_hook": "true",
  "targets": [
    {
      "target_name": "lumi",
      "cflags!": [
        "-fno-exceptions",
        "/ZW"
      ],
      "cflags_cc!": [
        "-fno-exceptions"
      ],
      "libraries": [
        "-lwindowsapp",
        "-lDxva2",
        "-lwmiutils",
        "-lwbemuuid",
      ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 1,
          "AdditionalOptions": [
            "-std:c++17"
          ]
        }
      },
      "sources": [
        "./src/index.cpp",
        "./src/utils.cpp",
        "./src/hash.cpp",
        "./src/display_config_helper.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "defines": [
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ]
    }
  ]
}
