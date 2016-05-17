# -*- Python -*-
# license
# license.

# ======================================================================

import json
from pathlib import Path

# ----------------------------------------------------------------------

class JSONEncoder (json.JSONEncoder):

    def default(self, o):
        if isinstance(o, Path):
            r = str(o)
        elif hasattr(o, "json"):
            r = o.json()
        else:
            r = "<" + str(type(o)) + " : " + repr(o) + ">"
        return r

# ----------------------------------------------------------------------

def dumps(data, indent=2, sort_keys=True, cls=JSONEncoder):
    return json.dumps(data, indent=indent, sort_keys=sort_keys, cls=cls)

# ----------------------------------------------------------------------

def dumpf(filepath, data, indent=2, sort_keys=True, cls=JSONEncoder):
    with filepath.open("w") as f:
        f.write(dumps(data, indent=indent, sort_keys=sort_keys, cls=cls) + "\n")

# ----------------------------------------------------------------------

def loadf(filepath):
    return json.load(filepath.open())

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
