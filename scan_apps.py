#!/usr/bin/env python3
import os, glob, configparser, subprocess

icon_map = {}
for idir in ["/usr/share/icons/Crule", "/usr/share/icons/hicolor", "/usr/share/icons", "/usr/share/pixmaps", os.path.expanduser("~/.local/share/icons")]:
    if not os.path.exists(idir): continue
    for root, _, files in os.walk(idir):
        for fn in files:
            if fn.endswith((".png", ".xpm")):
                stem = os.path.splitext(fn)[0]
                full = os.path.join(root, fn)
                if stem not in icon_map or any(s in full for s in ("/64x64/", "/128x128/", "/256x256/", "/48x48/")):
                    icon_map[stem] = full

seen = set()
for d in ["/usr/share/applications", os.path.expanduser("~/.local/share/applications")]:
    for f in sorted(glob.glob(d + "/*.desktop")):
        cp = configparser.ConfigParser(interpolation=None)
        try: cp.read(f, encoding="utf-8")
        except Exception: continue
        if "Desktop Entry" not in cp: continue
        de = cp["Desktop Entry"]
        if de.get("Type") != "Application" or de.get("NoDisplay", "").lower() == "true" or de.get("Hidden", "").lower() == "true": continue
        name = de.get("Name[tr]") or de.get("Name", "")
        if not name or name in seen: continue
        seen.add(name)
        comm = de.get("Comment[tr]") or de.get("Comment") or name
        raw_ic = de.get("Icon", "application-x-executable")
        icon = raw_ic if (os.path.exists(raw_ic) and raw_ic.endswith(".png")) else icon_map.get(os.path.basename(raw_ic), raw_ic)
        pkg = ""
        try:
            out = subprocess.check_output(["dpkg", "-S", f], stderr=subprocess.DEVNULL, text=True).strip()
            if ":" in out: pkg = out.split(":")[0].split(",")[0].strip()
        except Exception:
            exe = de.get("Exec", "").split()[0] if de.get("Exec") else ""
            pkg = os.path.basename(exe)
        if pkg: print(f"{name}\t{comm}\t{icon}\t{pkg}")
