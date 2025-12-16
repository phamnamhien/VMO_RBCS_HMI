#!/usr/bin/env python3
"""
Setup script to convert SquareLine Studio CMakeLists.txt to ESP-IDF component format
Features auto-detection, automatic backup, and LVGL include path fixing
"""

import os
import re
from pathlib import Path

def is_squareline_format(cmake_path):
    """Check if CMakeLists.txt is in SquareLine Studio format"""
    try:
        with open(cmake_path, 'r') as f:
            content = f.read()
        # SquareLine uses SET(SOURCES ...) and add_library(ui ...)
        return 'SET(SOURCES' in content and 'add_library(ui' in content
    except:
        return False

def parse_squareline_cmake(cmake_path):
    """Parse SquareLine CMakeLists.txt to extract SOURCES list"""
    with open(cmake_path, 'r') as f:
        content = f.read()
    
    # Find SET(SOURCES ...) block
    sources_match = re.search(r'SET\(SOURCES\s+(.*?)\)', content, re.DOTALL)
    if not sources_match:
        raise ValueError("Could not find SET(SOURCES ...) in CMakeLists.txt")
    
    sources_block = sources_match.group(1)
    
    # Parse source files
    sources = []
    for line in sources_block.split('\n'):
        line = line.strip()
        if line and not line.startswith('#'):
            sources.append(line)
    
    return sources

def generate_esp_idf_cmake(sources, output_path):
    """Generate CMakeLists.txt for ESP-IDF component"""
    
    # Format sources list with proper indentation
    sources_formatted = '\n        '.join(f'"{src}"' for src in sources)
    
    cmake_content = f'''# ESP-IDF Component CMakeLists.txt
# Auto-generated from SquareLine Studio export

idf_component_register(
    SRCS 
        {sources_formatted}
    INCLUDE_DIRS 
        "."
        "screens"
        "components"
        "images"
    REQUIRES 
        lvgl
)

# Add compile options if needed
target_compile_options(${{COMPONENT_LIB}} PRIVATE
    -Wno-unused-variable
    -Wno-unused-function
)
'''
    
    with open(output_path, 'w') as f:
        f.write(cmake_content)

def fix_lvgl_includes(ui_dir):
    """Fix LVGL include paths from 'lvgl/lvgl.h' to 'lvgl.h'"""
    print("[INFO] Fixing LVGL includes...")
    fixed_count = 0
    
    # Walk through all .c and .h files
    for root, dirs, files in os.walk(ui_dir):
        for file in files:
            if file.endswith(('.c', '.h')):
                filepath = Path(root) / file
                try:
                    # Read file content
                    with open(filepath, 'r', encoding='utf-8') as f:
                        content = f.read()
                    
                    # Replace lvgl/lvgl.h with lvgl.h
                    new_content = content.replace('#include "lvgl/lvgl.h"', '#include "lvgl.h"')
                    
                    # Only write if there were changes
                    if content != new_content:
                        with open(filepath, 'w', encoding='utf-8') as f:
                            f.write(new_content)
                        print(f"[INFO] Fixed LVGL includes in: {filepath.relative_to(ui_dir)}")
                        fixed_count += 1
                        
                except Exception as e:
                    print(f"[WARNING] Could not process {filepath}: {e}")
    
    if fixed_count > 0:
        print(f"[INFO] Fixed LVGL includes in {fixed_count} file(s)")
    else:
        print("[INFO] No LVGL includes needed fixing")

def main():
    # File paths
    script_dir = Path(__file__).parent
    squareline_cmake = script_dir / "CMakeLists.txt.squareline"
    output_cmake = script_dir / "CMakeLists.txt"
    
    # AUTO-DETECT: Check if CMakeLists.txt is in SquareLine format
    if output_cmake.exists() and not squareline_cmake.exists():
        if is_squareline_format(output_cmake):
            print("[INFO] Detected SquareLine CMakeLists.txt, creating automatic backup...")
            os.rename(output_cmake, squareline_cmake)
    
    # Check for .squareline file
    if not squareline_cmake.exists():
        print("[ERROR] Could not find CMakeLists.txt or CMakeLists.txt.squareline")
        print("[INFO] Make sure SquareLine Studio has exported CMakeLists.txt to this directory")
        return 1
    
    # If output exists and is newer than squareline, skip conversion
    if output_cmake.exists():
        if output_cmake.stat().st_mtime > squareline_cmake.stat().st_mtime:
            print("[INFO] CMakeLists.txt is already up to date, skipping conversion")
            # Still fix includes even if CMakeLists.txt is up to date
            fix_lvgl_includes(script_dir)
            return 0
    
    print(f"[INFO] Reading {squareline_cmake.name}")
    sources = parse_squareline_cmake(squareline_cmake)
    
    print(f"[INFO] Found {len(sources)} source files")
    
    print(f"[INFO] Generating {output_cmake.name} for ESP-IDF")
    generate_esp_idf_cmake(sources, output_cmake)
    
    # Fix LVGL includes after generating CMakeLists.txt
    fix_lvgl_includes(script_dir)
    
    print("[INFO] Conversion completed successfully")
    return 0

if __name__ == "__main__":
    exit(main())

    