from pathlib import Path
import os
import re

c_header = """
/**
 * This file is part of MesoGen, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2023).
 *   MesoGen: Designing Procedural On-Surface Stranded Mesostructures,
 *   ACM Transaction on Graphics (SIGGRAPH '23 Conference Proceedings),
 *   https://doi.org/10.1145/3588432.3591496
 *
 * Copyright (c) 2020 - 2023 -- Télécom Paris (Élie Michel <emichel@adobe.com>)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided "as is", without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */
""".strip()

cmake_header = """
# This file is part of MesoGen, the reference implementation of:
#
#   Michel, Élie and Boubekeur, Tamy (2023).
#   MesoGen: Designing Procedural On-Surface Stranded Mesostructures,
#   ACM Transaction on Graphics (SIGGRAPH '23 Conference Proceedings),
#   https://doi.org/10.1145/3588432.3591496
#
# Copyright (c) 2020 - 2023 -- Télécom Paris (Élie Michel <emichel@adobe.com>)
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# The Software is provided "as is", without warranty of any kind, express or
# implied, including but not limited to the warranties of merchantability,
# fitness for a particular purpose and non-infringement. In no event shall the
# authors or copyright holders be liable for any claim, damages or other
# liability, whether in an action of contract, tort or otherwise, arising
# from, out of or in connection with the software or the use or other dealings
# in the Software.
""".strip()

c_header_regex = re.compile(r"^/\*(.*?)\*/\n*", re.DOTALL)

cmake_header_regex = re.compile(r"^(#(.*)\n)+\n*")

root = Path(__file__).parent.parent.parent.parent
ignore_dirs = [
    r"^build.*",
    r"^src/External",
    r"^tests/External",
]

def is_ignored(path):
    for pattern in ignore_dirs:
        if re.match(pattern, path.as_posix()):
            return True
    return False

def main():

    for parent, subdirs, files in os.walk(root):
        if is_ignored(Path(parent).relative_to(root)):
            continue
        for fname in files:
            fpath = Path(parent) / fname
            if fpath.suffix in {'.h', '.c', '.cpp', '.glsl'}:
                ensure_file_header(fpath, c_header, c_header_regex)
            elif fpath.suffix in {'.cmake'}:
                ensure_file_header(fpath, cmake_header, cmake_header_regex)
            elif fname == "CMakeLists.txt":
                ensure_file_header(fpath, cmake_header, cmake_header_regex)

def ensure_file_header(fpath, header, header_regex):
    with open(fpath, 'rb') as f:
        try:
            content = f.read().decode('utf-8')
        except UnicodeDecodeError:
            print(f"Warning: File '{fpath}' does not use Unicode.")
            f.seek(0)
            content = f.read().decode('latin-1')

    m = re.match(header_regex, content)
    if m and "copyright" in m.group(0).lower():
        # Replace initial comment
        begin = m.end()
    else:
        # Add before initial comment
        begin = 0

    prev_content = content
    content = header + "\n" + content[begin:]

    tmp_fpath = fpath.parent / (fpath.name + '.tmp')
    with open(tmp_fpath, 'wb') as f:
        f.write(content.encode("utf-8"))
    os.replace(tmp_fpath, fpath)

if __name__ == '__main__':
    main()
