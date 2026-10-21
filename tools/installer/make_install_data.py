#!/usr/bin/env python3
"""Generate install_data.h: the HBC app files the one-shot SD installer embeds."""

import argparse
from pathlib import Path


def bytes_literal(data: bytes) -> str:
    """Format bytes compactly while keeping compiler input line lengths sane."""
    values = [str(value) for value in data]
    return ",\n".join(",".join(values[pos : pos + 32]) for pos in range(0, len(values), 32))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--app-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    files = [(name, args.app_dir / name) for name in ("boot.dol", "meta.xml", "icon.png")]
    missing = [str(path) for _, path in files if not path.is_file()]
    if missing:
        parser.error("missing input file(s): " + ", ".join(missing))

    lines = [
        "#ifndef INSTALL_DATA_H",
        "#define INSTALL_DATA_H",
        "#include <stddef.h>",
    ]
    entries = []
    total = 0
    for index, (name, path) in enumerate(files):
        data = path.read_bytes()
        total += len(data)
        lines.extend(
            (
                f"static const unsigned char blob{index}[]={{",
                bytes_literal(data),
                "};",
            )
        )
        entries.append(f'  {{"{name}",blob{index},sizeof(blob{index})}},')

    lines.extend(
        (
            "struct ifile{const char*name;const unsigned char*data;size_t len;};",
            "static const struct ifile INSTALL_FILES[]={",
            *entries,
            "};",
            "#define INSTALL_NFILES (sizeof(INSTALL_FILES)/sizeof(INSTALL_FILES[0]))",
            f"#define INSTALL_TOTAL_BYTES {total}u",
            "#endif",
            "",
        )
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines))
    print(f"embedded {len(files)} files ({total} bytes) in {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
