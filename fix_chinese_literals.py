#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
将 .cpp 文件中含中文的窄字符串字面量统一包装为 wxT("..."),
避免在英文 Windows 构建环境(MSVC 默认执行字符集 Windows-1252)下
中文字符被编译成 '?'。
"""
import re
import sys
from pathlib import Path

CJK_RE = re.compile(r"[\u4e00-\u9fa5]")
# 匹配一个双引号字符串字面量(简单处理,不考虑跨行)
STRING_RE = re.compile(r'"(?:[^"\\]|\\.)*"')


def has_cjk(s: str) -> bool:
    return bool(CJK_RE.search(s))


def should_skip_prefix(text_before: str) -> bool:
    # 已经是宽字符串/宏的字面量,不要再包一层
    stripped = text_before.rstrip()
    if stripped.endswith('L'):
        return True
    if stripped.endswith('wxT') or stripped.endswith('_T'):
        return True
    # 处理 wxT( 或 _T( 后紧跟 " 的情况(前面正则已经确保没有空白)
    if re.search(r'(?:wxT|_T)\s*\($', stripped):
        return True
    return False


def process_line(line: str) -> str:
    # 粗略跳过行尾注释,但保留注释前需要处理的内容
    comment_pos = line.find('//')
    if comment_pos != -1:
        prefix = line[:comment_pos]
        suffix = line[comment_pos:]
    else:
        prefix = line
        suffix = ""

    result = []
    last_end = 0
    for m in STRING_RE.finditer(prefix):
        s = m.group(0)
        if has_cjk(s):
            text_before = prefix[last_end:m.start()]
            if not should_skip_prefix(text_before):
                result.append(text_before)
                result.append('wxT(')
                result.append(s)
                result.append(')')
            else:
                result.append(text_before)
                result.append(s)
        else:
            result.append(prefix[last_end:m.end()])
        last_end = m.end()

    result.append(prefix[last_end:])
    return ''.join(result) + suffix


def read_text_flexible(path: Path) -> tuple[str, str]:
    """尝试多种编码读取文件,返回 (text, encoding)。"""
    for enc in ('utf-8-sig', 'utf-8', 'gbk', 'gb2312', 'gb18030'):
        try:
            return path.read_text(encoding=enc), enc
        except UnicodeDecodeError:
            continue
    raise UnicodeDecodeError(f"无法解码文件: {path}")


def process_file(path: Path, dry_run: bool = False) -> int:
    text, enc = read_text_flexible(path)
    lines = text.splitlines(keepends=True)
    new_lines = [process_line(line) for line in lines]
    new_text = ''.join(new_lines)
    changed = new_text != text
    if changed:
        if dry_run:
            print(f"[DRY-RUN] would change: {path}")
        else:
            # 统一写为 UTF-8 with BOM,便于 MSVC 识别中文
            path.write_text(new_text, encoding='utf-8-sig')
            print(f"[FIXED] {path}")
    return int(changed)


def main():
    dry_run = '--dry-run' in sys.argv
    root = Path(__file__).parent / 'Plugins' / 'StockV2'
    total_changed = 0
    for ext in ('*.cpp', '*.h'):
        for f in sorted(root.glob(ext)):
            total_changed += process_file(f, dry_run=dry_run)
    print(f"Total files {'would be ' if dry_run else ''}changed: {total_changed}")


if __name__ == '__main__':
    main()
