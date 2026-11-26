#!/usr/bin/env python3
import sys, json, urllib.request, urllib.parse, os

if len(sys.argv) < 4:
    sys.exit(1)

pkg = sys.argv[2]
out = sys.argv[3]
try:
    url = "https://" + "screenshots.debian.net/json/package/" + urllib.parse.quote(pkg)
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    data = json.loads(urllib.request.urlopen(req, timeout=5).read().decode())
    shots = data.get("screenshots", [])
    if shots:
        img_url = shots[0].get("large_image_url") or shots[0].get("small_image_url")
        if img_url:
            if img_url.startswith("/"):
                img_url = "https://" + "screenshots.debian.net" + img_url
            req_img = urllib.request.Request(img_url, headers={"User-Agent": "Mozilla/5.0"})
            with urllib.request.urlopen(req_img, timeout=6) as resp, open(out, "wb") as f:
                f.write(resp.read())
            if os.path.getsize(out) > 1000:
                sys.exit(0)
except Exception:
    pass
sys.exit(1)
