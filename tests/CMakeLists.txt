# Each test is a small standalone executable using plain assert() - no
# test framework dependency, consistent with the rest of this project's
# "vendor only what's truly needed" approach. A failing assert() aborts
# the process with a non-zero exit code, which CTest reports as FAIL.
#
# Run all of them with:  cd build && ctest --output-on-failure

add_executable(test_json test_json.cpp)
target_link_libraries(test_json PRIVATE presentation_builder_lib)
add_test(NAME JsonTests COMMAND test_json)

add_executable(test_json_helpers test_json_helpers.cpp)
target_link_libraries(test_json_helpers PRIVATE presentation_builder_lib)
add_test(NAME JsonHelpersTests COMMAND test_json_helpers)

add_executable(test_utils test_utils.cpp)
target_link_libraries(test_utils PRIVATE presentation_builder_lib)
add_test(NAME UtilsTests COMMAND test_utils)

add_executable(test_html_importer test_html_importer.cpp)
target_link_libraries(test_html_importer PRIVATE presentation_builder_lib)
add_test(NAME HtmlImporterTests COMMAND test_html_importer)

add_executable(test_pptx_roundtrip test_pptx_roundtrip.cpp)
target_link_libraries(test_pptx_roundtrip PRIVATE presentation_builder_lib)
add_test(NAME PptxRoundtripTests COMMAND test_pptx_roundtrip)

add_executable(test_theme test_theme.cpp)
target_link_libraries(test_theme PRIVATE presentation_builder_lib)
add_test(NAME ThemeTests COMMAND test_theme)

add_executable(test_slides test_slides.cpp)
target_link_libraries(test_slides PRIVATE presentation_builder_lib)
add_test(NAME SlideTests COMMAND test_slides)

add_executable(test_presentation test_presentation.cpp)
target_link_libraries(test_presentation PRIVATE presentation_builder_lib)
add_test(NAME PresentationTests COMMAND test_presentation)

add_executable(test_template_manager test_template_manager.cpp)
target_link_libraries(test_template_manager PRIVATE presentation_builder_lib)
add_test(NAME TemplateManagerTests COMMAND test_template_manager)

add_executable(test_http_client test_http_client.cpp)
target_link_libraries(test_http_client PRIVATE presentation_builder_lib)
# add_test(NAME HttpClientTests COMMAND test_http_client)
