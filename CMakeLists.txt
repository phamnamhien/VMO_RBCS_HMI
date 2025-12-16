# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# ========================================
# Auto setup UI component from SquareLine
# ========================================
set(UI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/components/ui")
set(SQUARELINE_CMAKE "${UI_DIR}/CMakeLists.txt.squareline")
set(OUTPUT_CMAKE "${UI_DIR}/CMakeLists.txt")
set(SETUP_SCRIPT "${UI_DIR}/setup_ui.py")

# Find Python interpreter
find_package(Python3 COMPONENTS Interpreter)

if(Python3_FOUND)
    # Case 1: Script doesn't exist, create it
    if(NOT EXISTS ${SETUP_SCRIPT})
        message(STATUS "Creating setup_ui.py...")
        file(WRITE ${SETUP_SCRIPT} "#!/usr/bin/env python3\n")
        file(APPEND ${SETUP_SCRIPT} [[
import os, re
from pathlib import Path

def is_squareline_format(cmake_path):
    try:
        with open(cmake_path, 'r') as f:
            content = f.read()
        return 'SET(SOURCES' in content and 'add_library(ui' in content
    except:
        return False

def parse_squareline_cmake(cmake_path):
    with open(cmake_path, 'r') as f:
        content = f.read()
    sources_match = re.search(r'SET\(SOURCES\s+(.*?)\)', content, re.DOTALL)
    if not sources_match:
        raise ValueError("Could not find SET(SOURCES ...) in CMakeLists.txt")
    sources = []
    for line in sources_match.group(1).split('\n'):
        line = line.strip()
        if line and not line.startswith('#'):
            sources.append(line)
    return sources

def generate_esp_idf_cmake(sources, output_path):
    sources_fmt = '\n        '.join(f'"{s}"' for s in sources)
    cmake = f'''# ESP-IDF Component CMakeLists.txt
# Auto-generated from SquareLine Studio export

idf_component_register(
    SRCS 
        {sources_fmt}
    INCLUDE_DIRS 
        "."
        "screens"
        "components"
        "images"
    REQUIRES 
        lvgl
)

target_compile_options(${{COMPONENT_LIB}} PRIVATE
    -Wno-unused-variable
    -Wno-unused-function
)
'''
    with open(output_path, 'w') as f:
        f.write(cmake)

script_dir = Path(__file__).parent
squareline_cmake = script_dir / "CMakeLists.txt.squareline"
output_cmake = script_dir / "CMakeLists.txt"

if output_cmake.exists() and not squareline_cmake.exists():
    if is_squareline_format(output_cmake):
        print("[INFO] Auto-backup CMakeLists.txt -> .squareline")
        os.rename(output_cmake, squareline_cmake)

if not squareline_cmake.exists():
    print("[ERROR] Could not find CMakeLists.txt.squareline")
    exit(1)

if output_cmake.exists() and output_cmake.stat().st_mtime > squareline_cmake.stat().st_mtime:
    print("[INFO] Skip - CMakeLists.txt is up to date")
    exit(0)

print("[INFO] Converting SquareLine -> ESP-IDF format")
sources = parse_squareline_cmake(squareline_cmake)
print(f"[INFO] Found {len(sources)} source files")
generate_esp_idf_cmake(sources, output_cmake)
print("[INFO] Conversion completed successfully")
]])
        execute_process(COMMAND chmod +x ${SETUP_SCRIPT})
    endif()

    # Case 2: CMakeLists.txt exists from SquareLine, needs conversion
    if(EXISTS ${UI_DIR}/CMakeLists.txt OR EXISTS ${SQUARELINE_CMAKE})
        execute_process(
            COMMAND ${Python3_EXECUTABLE} ${SETUP_SCRIPT}
            WORKING_DIRECTORY ${UI_DIR}
            RESULT_VARIABLE SETUP_RESULT
            OUTPUT_VARIABLE SETUP_OUTPUT
            ERROR_VARIABLE SETUP_ERROR
        )
        
        if(SETUP_RESULT EQUAL 0)
            message(STATUS "UI component: ${SETUP_OUTPUT}")
        else()
            message(WARNING "UI setup failed: ${SETUP_ERROR}")
        endif()
    endif()
else()
    message(WARNING "Python3 not found - UI auto-setup disabled")
endif()

# ========================================
# ESP-IDF Project
# ========================================
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(RBCS_HMI)
