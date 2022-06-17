# PSn00bSDK example CMake script
# (C) 2021 spicyjpeg - MPL licensed

cmake_minimum_required(VERSION 3.20)

project(
	PS1MiniPadTest
	LANGUAGES    C ASM
	VERSION      0.3.1
	DESCRIPTION  "PS1 Mini Pad Tester intended to use with FreePSXBoot"
	HOMEPAGE_URL "https://github.com/luisibalaz/PS1MiniPadTest"
)

file(GLOB _sources *.c *.s)
psn00bsdk_add_executable(PS1MiniPadTest STATIC ${_sources})
#psn00bsdk_add_cd_image(pads_iso pads iso.xml DEPENDS pads)

psn00bsdk_target_incbin(PS1MiniPadTest PRIVATE buttons_tim all.tim)

install(FILES ${PROJECT_BINARY_DIR}/PS1MiniPadTest.exe TYPE BIN)
