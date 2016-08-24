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
        # elif hasattr(o, "json"):
        #     r = o.json()
        else:
            r = "<" + str(type(o)) + " : " + repr(o) + ">"
        return r

# ----------------------------------------------------------------------

def dumps(data, indent=2, compact=True, sort_keys=True):
    if indent is not None and compact:
        data = json_dumps(data, indent=indent, indent_increment=indent)
    else:
        data = json.dumps(data, indent=indent, sort_keys=sort_keys, cls=JSONEncoder)
    return data

# ----------------------------------------------------------------------

def dumpf(filepath, data, indent=2, sort_keys=True):
    with Path(filepath).open("w") as f:
        f.write(dumps(data, indent=indent, sort_keys=sort_keys) + "\n")

# ----------------------------------------------------------------------

def loadf(filepath):
    data = json.load(Path(filepath).open())
    if data.get("_")[:20] == "-*- js-indent-level:":
        del data["_"]
    return data

# ----------------------------------------------------------------------

def json_dumps(data, indent=2, indent_increment=None, toplevel=True):
    """More compact dumper with wide lines."""

    def simple(d):
        r = True
        if isinstance(d, dict):
            r = not any(isinstance(v, (list, tuple, set, dict)) for v in d.values()) and len(d) < 5
        elif isinstance(d, (tuple, list)):
            r = not any(isinstance(v, (list, tuple, set, dict)) for v in d)
        return r

    def end(symbol, indent):
        if indent > indent_increment:
            r = "{:{}s}{}".format("", indent - indent_increment, symbol)
        else:
            r = symbol
        return r

    if indent_increment is None:
        indent_increment = indent
    r = []
    if simple(data):
        if isinstance(data, set):
            r.append(json.dumps(sorted(data), sort_keys=True, cls=JSONEncoder))
        else:
            r.append(json.dumps(data, sort_keys=True, cls=JSONEncoder))
    else:
        if isinstance(data, dict):
            if toplevel:
                r.append("{{\"_\":\"-*- js-indent-level: {} -*-\",".format(indent_increment))
            else:
                r.append("{")
            for no, k in enumerate(sorted(data), start=1):
                comma = "," if no < len(data) else ""
                r.append("{:{}s}{}: {}{}".format("", indent, json.dumps(k, cls=JSONEncoder), json_dumps(data[k], indent + indent_increment, indent_increment, toplevel=False), comma))
            r.append(end("}", indent))
        elif isinstance(data, (tuple, list)):
            r.append("[")
            for no, v in enumerate(data, start=1):
                comma = "," if no < len(data) else ""
                r.append("{:{}s}{}{}".format("", indent, json_dumps(v, indent + indent_increment, indent_increment, toplevel=False), comma))
            r.append(end("]", indent))
    return "\n".join(r)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
