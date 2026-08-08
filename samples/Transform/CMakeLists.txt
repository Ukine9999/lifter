add_library(LifterTransformFixture SHARED Transform.c)

set_target_properties(LifterTransformFixture PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    OUTPUT_NAME LifterTransformFixture)

if(MSVC)
    target_compile_options(LifterTransformFixture PRIVATE /W4 /WX /GS- /utf-8)
    target_link_options(LifterTransformFixture PRIVATE /INCREMENTAL:NO)
else()
    target_compile_options(LifterTransformFixture PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
