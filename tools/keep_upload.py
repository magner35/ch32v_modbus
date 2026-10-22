# -*- coding: utf-8 -*-
"""
Дополнительные цели сборки:

  pio run -e <env> -t upload_keep   — прошивка БЕЗ полного стирания flash:
                                      пользовательский регион (журнал критических
                                      данных 0x08003C00, будущие настройки) сохраняется.
                                      Границу региона защищает линкер
                                      (board_upload.maximum_size = 15360).
  pio run -e <env> -t journal_dump  — показать страницу журнала (0x08003C00).

Обычная цель `upload` тоже не стирает регион (wlink flash без -e стирает
только записываемые страницы), но upload_keep дополнительно печатает
первую запись журнала ПОСЛЕ прошивки — визуальный контроль, что данные
пережили заливку.
"""

import os

Import("env")

pkg_dir = env.PioPlatform().get_package_dir("tool-wlink") or ""
wlink = os.path.join(pkg_dir, "wlink.exe" if os.name == "nt" else "wlink")

firmware_elf = os.path.join(env.subst("$BUILD_DIR"), "firmware.elf")

USER_REGION_ADDR = 0x08003C00
USER_REGION_SIZE = 1024

env.AddPlatformTarget(
    "upload_keep",
    firmware_elf,
    [
        env.VerboseAction('"%s" flash "%s"' % (wlink, firmware_elf),
                          "Flashing (user flash region preserved)"),
        env.VerboseAction('"%s" dump 0x%08X %d' % (wlink, USER_REGION_ADDR, 64),
                          "Journal first record after flash (magic must be CRIT)"),
    ],
    "Upload WITHOUT erasing user flash region (settings/counters)",
)

env.AddPlatformTarget(
    "journal_dump",
    None,
    [
        env.VerboseAction('"%s" dump 0x%08X %d' % (wlink, USER_REGION_ADDR, USER_REGION_SIZE),
                          "User region dump (critical data journal)"),
    ],
    "Dump critical-data journal page",
)

# Примечание: подмена Upload для env *_keep сделана в platformio.ini
# (upload_protocol = custom + upload_command) — надёжнее, чем Replace здесь:
# pre-скрипты выполняются ДО построения цели upload платформой, и её
# env.Replace(UPLOADCMD=...) затирал бы нашу подмену.
