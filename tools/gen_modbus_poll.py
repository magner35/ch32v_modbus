#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Генератор файла конфигурации Modbus Poll (.mbp) из карты регистров
lib/Modbus/modbus_registers.h (enum IREG_*).

Формат .mbp (реверс входного файла):
  [0..571]   заголовок; int32 @ +16 = число строк, @ +8 = код функции
  [572..]    строки: FF FE FF <len> <имя UTF-16LE, len символов> 55 55 55 55
             + 48 байт тела (во входном файле константа для всех строк)
  [хвост]    блоки масштабов фиксированной ёмкости — копируются из шаблона

Логика строк повторяет поведение прошивки: значение записывается хелпером
по индексу enum и занимает 2 регистра для FLOAT/UL/TIME/U32 (вторая строка
без имени), 1 регистр для U16/ENUM/BOOL. Dummy-слоты PREV_LONG_MEMBER
поглощаются вторым словом предыдущего значения.

Запуск: вручную `python tools/gen_modbus_poll.py`
        или автоматически после сборки (extra_scripts = post:...).
"""

import os
import re
import struct
import sys

def _paths_from(project_dir):
    here = os.path.join(project_dir, "tools")
    # карту могли держать в src/ или lib/Modbus/
    regs = next(p for p in (
        os.path.join(project_dir, "src", "modbus_registers.h"),
        os.path.join(project_dir, "lib", "Modbus", "modbus_registers.h"),
    ) if os.path.exists(p))
    return {
        "TEMPLATE": os.path.join(here, "modbuspoll_template.mbp"),
        "REGS_H": regs,
        "OUTPUT": os.path.join(project_dir, "modbuspoll_ireg.mbp"),
    }


try:
    _p = _paths_from(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
except NameError:
    _p = None  # под SCons (pio extra_scripts) __file__ не определён

if _p is not None:
    TEMPLATE, REGS_H, OUTPUT = _p["TEMPLATE"], _p["REGS_H"], _p["OUTPUT"]

HEADER_SIZE = 572
ROW_BODY = bytes.fromhex("000000ff") * 6 + b"\x00" * 24  # тело строки из шаблона
TWO_WORD_TYPES = {"FLOAT", "UL", "TIME", "U32"}
ROW_CAPACITY = 300  # ёмкость хвоста шаблона (проверено на исходнике)


def parse_ireg_enum(path):
    """Вернуть список (имя, словарь_значения, тип) и IREG_LASTREGISTER."""
    src = open(path, encoding="utf-8", errors="replace").read()

    # первый enum, содержащий IREG_LASTREGISTER
    enums = re.findall(r"enum\s*\{(.*?)\};", src, re.S)
    body = next(e for e in enums if "IREG_LASTREGISTER" in e)

    members = []  # (имя, value_slot, two_word)
    counter = 0
    for raw in body.splitlines():
        line = raw.strip()
        m_prev = re.match(r"PREV_LONG_MEMBER\(\s*(\w+)\s*\)", line)
        m_plain = re.match(r"(IREG_\w+)\s*,", line)
        if not (m_prev or m_plain):
            continue
        mtype = re.search(r"type:\s*(\w+)", line)
        two_word = bool(mtype) and mtype.group(1) in TWO_WORD_TYPES
        if m_prev:
            name = m_prev.group(1)
            if "LASTREGISTER" in name:
                counter += 2
                continue
            counter += 1  # dummy слот
            value_slot = counter
            counter += 1
        else:
            name = m_plain.group(1)
            if "LASTREGISTER" in name:
                break
            value_slot = counter
            counter += 1
        members.append((name, value_slot, two_word))
    return members, counter  # counter == IREG_LASTREGISTER


def row_names(members, total_slots):
    """Слоты 0..total-1 → имя или '' (второе слово/сиротский dummy).
    Имя = идентификатор enum без префикса IREG_, в исходном регистре
    (IREG_ESTATUS → ESTATUS)."""
    names = {}
    for name, slot, two_word in members:
        base = name[len("IREG_"):]
        if slot in names:
            print("gen_mbp: ВНИМАНИЕ, коллизия слота %d (%s / %s) — "
                  "карта регистров кривая, см. PREV_LONG_MEMBER"
                  % (slot, names[slot][0], base), file=sys.stderr)
        names[slot] = (base, two_word)
    rows = []
    for slot in range(total_slots):
        rows.append(names[slot][0] if slot in names else "")
    return rows


def template_tail_offset(tpl):
    """Начало хвоста шаблона = конец последней записи строк."""
    i = tpl.find(b"\xff\xfe\xff", 0x200)
    while True:
        ln = tpl[i + 3]
        i = i + 4 + ln * 2 + 4 + 48  # маркер+имя+паддинг+тело
        if tpl[i:i + 3] != b"\xff\xfe\xff":
            return i


def build_mbp(rows):
    tpl = open(TEMPLATE, "rb").read()
    header = bytearray(tpl[:HEADER_SIZE])
    struct.pack_into("<i", header, 16, len(rows))  # счётчик строк

    out = bytearray(header)
    for name in rows:
        enc = name.encode("utf-16-le")
        assert len(name) < 256
        out += b"\xff\xfe\xff" + bytes([len(name)]) + enc + b"\x55\x55\x55\x55"
        out += ROW_BODY
    out += tpl[template_tail_offset(tpl):]  # хвост шаблона без изменений
    return bytes(out)


def main():
    members, total = parse_ireg_enum(REGS_H)
    if total > ROW_CAPACITY:
        print("gen_mbp: СЛИШКОМ МНОГО строк (%d > %d)" % (total, ROW_CAPACITY),
              file=sys.stderr)
        return 1
    rows = row_names(members, total)
    blob = build_mbp(rows)
    open(OUTPUT, "wb").write(blob)
    print("gen_mbp: %s — %d строк (%d регистров)" % (OUTPUT, len(rows), total))
    return 0


if "SCons.Script" in sys.modules:
    # запуск как post-скрипта PlatformIO (extra_scripts = post:...):
    # выполняется после завершения сборки; __file__ здесь не определён,
    # пути берём из окружения сборки
    Import("env")  # noqa: F821 — билтин SCons
    _p = _paths_from(env.subst("$PROJECT_DIR"))
    TEMPLATE, REGS_H, OUTPUT = _p["TEMPLATE"], _p["REGS_H"], _p["OUTPUT"]
    main()
else:
    sys.exit(main())
