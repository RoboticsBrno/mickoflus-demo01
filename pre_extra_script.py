import os
import hashlib
import subprocess
import json
import re

Import("env", "projenv")

def before_upload(source, target, env):
    hasher = hashlib.sha1()
    for root, dirs, files in os.walk("data"):
        dirs.sort()
        for name in sorted(files):
            with open(os.path.join(root, name), "rb") as f:
                for chunk in iter(lambda: f.read(32787), b""):
                    hasher.update(chunk)

    dev_list = subprocess.check_output([ "pio", "device", "list", "--serial", "--json-output" ], env=env["ENV"])
    dev_list = json.loads(dev_list)
    for d in dev_list:
        hasher.update(d.get("hwid", ""))

    current_sha1 = hasher.hexdigest()
    if os.path.exists(".last_uploadfs_sha1"):
        with open(".last_uploadfs_sha1", "r") as f:
            if f.read() == current_sha1:
                print("SPIFFS data are the same.")
                return
    
    print("SPIFFS data changed, running uploadfs target!")
    with open(".last_uploadfs_sha1", "w") as f:
        f.write(current_sha1)

    env.Execute("pio run -t uploadfs")

env.AddPostAction("upload", before_upload)
