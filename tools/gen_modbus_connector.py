#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Генератор конфига ModbusConnector (settings.json-совместимого) из карты
регистров src/modbus_registers.h: обе карты (IREG/HREG) + биты FSTATUS.

Выход: modbusconnector.json в корне проекта — один master-вкладыш с нашим
прибором (RTU, 9600 8N1, unit 1 — порт/параметры поправьте под себя).
Запуск: вручную `python tools/gen_modbus_connector.py` или после сборки
(extra_scripts = post:...).
"""

import json
import os
import re
import sys

# нормализация типов из карты (регистронезависимо): тип → (format утилиты, слов)
TYPE_MAP = {"FLOAT": ("f32", 2), "F32": ("f32", 2),
            "U32": ("u32", 2), "UL": ("u32", 2), "TIME": ("u32", 2),
            "U64": ("u64", 4)}


def _paths_from(project_dir):
    regs = next(p for p in (
        os.path.join(project_dir, "src", "modbus_registers.h"),
        os.path.join(project_dir, "lib", "Modbus", "modbus_registers.h"),
    ) if os.path.exists(p))
    return {"REGS_H": regs,
            "OUTPUT": os.path.join(project_dir, "modbusconnector.json")}


try:
    _p = _paths_from(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
except NameError:
    _p = None
if _p is not None:
    REGS_H, OUTPUT = _p["REGS_H"], _p["OUTPUT"]


def parse_enum(body):
    """Строки enum → [(имя, адрес, тип, юнит)]. Адрес PREV-члена = его
    значение (запись хелперами идёт по этому индексу, занимая 2 слова)."""
    out = []
    counter = 0
    for raw in body.splitlines():
        line = raw.strip()
        m_prev = re.match(r"PREV_LONG_MEMBER\(\s*(\w+)\s*\)", line)
        m_plain = re.match(r"(\w+),", line)
        if m_prev:
            name = m_prev.group(1)
            if "LASTREGISTER" in name:
                break
            counter += 1  # dummy слот
            value = counter
            counter += 1
        elif m_plain:
            name = m_plain.group(1)
            if "LASTREGISTER" in name:
                break
            value = counter
            counter += 1
        else:
            continue
        mtype = re.search(r"type:\s*(\S+?)(?:,|$|\s/)", line)
        munit = re.search(r"unit:(.+?)(?:\s*//|$)", line)
        rtype = (mtype.group(1).strip() if mtype else "U16").upper()
        unit = munit.group(1).strip() if munit else ""
        out.append((name, value, rtype, unit))
    return out


def parse_bits(body):
    """enum FSTATUS → {"бит": "имя"} для value_names статуса"""
    bits = {}
    for i, raw in enumerate(l for l in body.splitlines() if re.match(r"\s*FSTATUS_\w+,?", l)):
        m = re.match(r"\s*(FSTATUS_\w+),?", raw)
        bits[str(i)] = m.group(1)[len("FSTATUS_"):]
    return bits


def build_registers():
    src = open(REGS_H, encoding="utf-8", errors="replace").read()
    enums = re.findall(r"enum\s*\{(.*?)\};", src, re.S)
    ireg = next(e for e in enums if "IREG_LASTREGISTER" in e)
    hreg = next(e for e in enums if "HREG_LASTREGISTER" in e)
    fstatus = next((e for e in enums if "FSTATUS_" in e), None)

    bits = parse_bits(fstatus) if fstatus else {}

    def entry(name, value, rtype, unit, kind):
        base = name.split("_", 1)[1] if "_" in name else name
        fmt, words = TYPE_MAP.get(rtype, ("dec", 1))
        e = {
            "name": base,
            "kind": kind,
            "address": value,
            "count": words,
            "unit_id": "",
            "poll_ms": "",
            "poll_enabled": True,
            "format": fmt,
            "order": "",
            "scale": 1.0,
            "offset": 0.0,
            "unit": unit,
            "log": True,
            "value_names": {},
            "bitmask": False,
            "alarms": [],
        }
        if name == "IREG_STATUS":  # слово статуса: биты FSTATUS
            e["format"] = "hex"
            e["bitmask"] = True
            e["value_names"] = bits
        elif name == "HREG_CONTROL":  # слово команд: hex
            e["format"] = "hex"
        return e

    regs = []
    for name, value, rtype, unit in parse_enum(ireg):
        regs.append(entry(name, value, rtype, unit, "input_registers"))
    for name, value, rtype, unit in parse_enum(hreg):
        regs.append(entry(name, value, rtype, unit, "holding_registers"))
    return regs


def main():
    doc = {
        "theme": "light",
        "language": "en",
        "tabs": [
            {
                "mode": "master",
                "connection": {
                    "type": "RTU",
                    "tcp_host": "127.0.0.1",
                    "tcp_port": 502,
                    "rtu_port": "COM12",
                    "rtu_baud": "9600",
                    "rtu_bytesize": "8",
                    "rtu_parity": "N",
                    "rtu_stopbits": "1",
                    "unit": 1,
                    "timeout": 3.0,
                },
                "registers": build_registers(),
                "registers_options": {
                    "order": "ABCD",
                    "column_widths": [34, 229, 136, 62, 59, 63, 81, 80, 395, 120, 118, 100],
                    "column_order": ["poll_enabled", "name", "type", "address", "count",
                                     "unit_id", "poll_ms", "format", "value", "new_value",
                                     "trend", "actions"],
                    "hidden_columns": [],
                    "expressions_visible": False,
                    "group_reads": True,
                },
                "expressions": [],
                "logging": {"path": "", "format": "csv",
                            "fields": ["timestamp", "name", "address", "kind"],
                            "append": True},
                "scanner": {
                    "start": 1, "end": 247,
                    "probes": [{"kind": "holding_registers", "address": 0, "count": 1},
                               {"kind": "input_registers", "address": 0, "count": 1},
                               {"kind": "coils", "address": 0, "count": 1}],
                    "addr_unit": 1, "addr_kind": "coils", "addr_from": 0, "addr_to": 99,
                },
                "sim": {"server": {"type": "TCP", "host": "127.0.0.1", "port": 1502,
                                   "rtu_port": "COM14", "rtu_baud": "9600",
                                   "rtu_bytesize": "8", "rtu_parity": "N",
                                   "rtu_stopbits": "1", "unit": "any"},
                        "rows": [], "tick_ms": 1000},
                "sniffer": {"params": {"port": "COM14", "baudrate": 9600,
                                       "bytesize": 8, "parity": "N", "stopbits": 1},
                            "units": []},
                "gateway": {
                    "listen": {"type": "TCP", "tcp_host": "0.0.0.0", "tcp_port": 1502,
                               "rtu_port": "COM14", "rtu_baud": "9600",
                               "rtu_bytesize": "8", "rtu_parity": "N", "rtu_stopbits": "1"},
                    "target": {"type": "TCP", "tcp_host": "127.0.0.1", "tcp_port": 502,
                               "rtu_port": "COM14", "rtu_baud": "9600",
                               "rtu_bytesize": "8", "rtu_parity": "N", "rtu_stopbits": "1",
                               "timeout": 3.0},
                    "units": "",
                },
            }
        ],
        "active_tab": 0,
    }
    with open(OUTPUT, "w", encoding="utf-8", newline="\n") as f:
        json.dump(doc, f, ensure_ascii=False, indent=2)
    n_in = sum(1 for r in doc["tabs"][0]["registers"] if r["kind"] == "input_registers")
    n_h = len(doc["tabs"][0]["registers"]) - n_in
    print("gen_connector: %s — %d input + %d holding" % (OUTPUT, n_in, n_h))
    return 0


if "SCons.Script" in sys.modules:
    Import("env")  # noqa: F821
    _p = _paths_from(env.subst("$PROJECT_DIR"))
    REGS_H, OUTPUT = _p["REGS_H"], _p["OUTPUT"]
    main()
else:
    sys.exit(main())
