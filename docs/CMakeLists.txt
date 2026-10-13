find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(NOT DOXYGEN_FOUND)
  message(WARNING "Doxygen not found. Disabling document generation.")
else()
  # Project
  set(DOXYGEN_PROJECT_NAME "Matching Engine")
  set(DOXYGEN_PROJECT_VERSION ${PROJECT_VERSION})
  set(DOXYGEN_PROJECT_BRIEF "C++ Matching Engine")

  # Output
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_GENERATE_LATEX NO)
  set(DOXYGEN_GENERATE_TREEVIEW YES)

  # Parsing
  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_EXTRACT_PRIVATE YES) # Used internally
  set(DOXYGEN_RECURSIVE YES)

  # Excludes
  set(DOXYGEN_EXCLUDE_PATTERNS
    "*/vcpkg_installed/*"
    "*/build/*"
    "*/tests/*"
    "*/CMakeLists.txt"
  )

  # Graphviz integration
  if(Doxygen_dot_FOUND)
    set(DOXYGEN_HAVE_DOT YES)
    set(DOXYGEN_CALL_GRAPH YES)
    set(DOXYGEN_CALLER_GRAPH YES)
    set(DOXYGEN_UML_LOOK YES)
    set(DOXYGEN_INTERACTIVE_SVG YES)
  else()
    set(DOXYGEN_HAVE_DOT NO)
    message(STATUS "Graphviz (dot) not found. Disabling graph generation.")
  endif()

  # Home page
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${PROJECT_SOURCE_DIR}/README.md")

  # Add docs target
  doxygen_add_docs(
        docs
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/README.md
        COMMENT "Generating HTML documentation"
  )
endif()
