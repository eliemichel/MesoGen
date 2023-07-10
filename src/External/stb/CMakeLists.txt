add_library(stb_image_write STATIC
	stb_image_write.h
	stb_image_write.c
)
target_include_directories(stb_image_write PUBLIC .)
