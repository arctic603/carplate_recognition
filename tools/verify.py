import os, re

base = 'D:/Arctic_QT/PlateRecognition'
errors = []
warnings = []

# 1. Check all expected files exist
expected_files = [
    'CMakeLists.txt',
    'README.md',
    'src/main.cpp',
    'src/core/PlateDetector.h', 'src/core/PlateDetector.cpp',
    'src/core/CharacterClassifier.h', 'src/core/CharacterClassifier.cpp',
    'src/core/PlateRecognizer.h', 'src/core/PlateRecognizer.cpp',
    'src/data/Record.h',
    'src/data/RecordManager.h', 'src/data/RecordManager.cpp',
    'src/utils/ImageConverter.h', 'src/utils/ImageConverter.cpp',
    'src/utils/CsvExporter.h', 'src/utils/CsvExporter.cpp',
    'src/ui/MainWindow.h', 'src/ui/MainWindow.cpp',
    'src/ui/RecognitionWidget.h', 'src/ui/RecognitionWidget.cpp',
    'src/ui/CameraWidget.h', 'src/ui/CameraWidget.cpp',
    'src/ui/HistoryWidget.h', 'src/ui/HistoryWidget.cpp',
    'src/ui/StatisticsWidget.h', 'src/ui/StatisticsWidget.cpp',
    'resources/resources.qrc',
    'tools/generate_templates.py',
    'docs/LEARNING_GUIDE.md',
]

for f in expected_files:
    path = os.path.join(base, f)
    if not os.path.exists(path):
        errors.append(f'MISSING: {f}')
    elif os.path.getsize(path) < 50:
        warnings.append(f'SUSPICIOUS SIZE: {f} ({os.path.getsize(path)} bytes)')

# 2. Check CMakeLists.txt references match actual files
cmake_path = os.path.join(base, 'CMakeLists.txt')
with open(cmake_path, 'r', encoding='utf-8') as f:
    cmake_content = f.read()

# Extract source files from CMakeLists.txt
src_pattern = re.findall(r'src/\S+\.cpp', cmake_content)
hdr_pattern = re.findall(r'src/\S+\.h', cmake_content)

for f in src_pattern + hdr_pattern:
    path = os.path.join(base, f)
    if not os.path.exists(path):
        errors.append(f'IN CMAKE BUT MISSING: {f}')

# 3. Check for common issues
for root, dirs, files in os.walk(os.path.join(base, 'src')):
    for f in files:
        if f.endswith(('.h', '.cpp')):
            path = os.path.join(root, f)
            with open(path, 'r', encoding='utf-8') as fp:
                content = fp.read()

            # Check for Q_OBJECT without include
            if 'Q_OBJECT' in content and '#include' not in content:
                errors.append(f'Q_OBJECT without includes: {f}')

            # Check header guards in .h files
            if f.endswith('.h') and '#ifndef' not in content and '#pragma once' not in content:
                warnings.append(f'No header guard: {f}')

# 4. Summary
print(f'=== Project Verification ===')
print(f'Expected files: {len(expected_files)}')
print(f'Errors: {len(errors)}')
print(f'Warnings: {len(warnings)}')

if errors:
    print('\n--- ERRORS ---')
    for e in errors:
        print(f'  ✗ {e}')

if warnings:
    print('\n--- WARNINGS ---')
    for w in warnings:
        print(f'  ⚠ {w}')

if not errors and not warnings:
    print('\n✓ All checks passed!')
