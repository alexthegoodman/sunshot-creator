{
	"targets": [
		{
			"target_name": "addon",
			"sources": ["addon.cpp"],
			"include_dirs": [
                "<!(node -e \"require('nan')\")",
				"<!(node -p \"require('node-addon-api').include\")",
				"C:/Users/alext/scoop/apps/ffmpeg-shared/6.0/include",
			],
			# "dependencies": [
			# 	"<!(node -p \"require('node-addon-api').gyp\")"
			# ],
			# "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"],
			# "cflags!": ["-fno-exceptions"],
			# "cflags_cc!": ["-fno-exceptions"],
			"conditions": [
				['OS=="win"', {
					# "defines": [
					# 	"_CRT_SECURE_NO_WARNINGS",
					# 	"_WIN32_WINNT=0x0601"
					# ],
					"msvs_settings": {
						"VCCLCompilerTool": {
							"ExceptionHandling": 1,
							"RuntimeLibrary": 3,	
							"AdditionalIncludeDirectories": [	
								"C:/Users/alext/scoop/apps/ffmpeg-shared/6.0/include",
							]
						},
						"VCLinkerTool": {	
							"AdditionalLibraryDirectories": [	
								"C:/Users/alext/scoop/apps/ffmpeg-shared/6.0/lib",
							],
							"AdditionalDependencies": [		
								"avutil.lib",	
								"avformat.lib",
								"avcodec.lib",
								"swscale.lib",
							]
						}
					}
				}]
			],
            "link_settings": {
                "libraries": [
					# "-LC:/Users/alext/scoop/apps/ffmpeg-shared/6.0/lib",
					"C:/Users/alext/scoop/apps/ffmpeg-shared/6.0/lib/avutil.lib",
					"C:/Users/alext/scoop/apps/ffmpeg-shared/6.0/lib/avformat.lib",
					"C:/Users/alext/scoop/apps/ffmpeg-shared/6.0/lib/avcodec.lib",
					"C:/Users/alext/scoop/apps/ffmpeg-shared/6.0/lib/swscale.lib"
                ],
			}
		}
	]
}