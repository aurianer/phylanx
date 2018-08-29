from phylanx.util import *
import re

argcount = {}

# Get the list of all primitives/plugins
all = phylist()

for p in all:
    for m in all[p]:

        # We expect a pattern here, e.g. foo(_1, _2)
        g = re.match(r'(.*)\((.*)\)', m)
        if not g:
            continue

        # Keep the error string in err
        err = None

        # The name part of the pattern, e.g. "foo"
        n = g.group(1)

        # The args part of the pattern, e.g. "(_1, _2)"
        args = g.group(2)

        # The list of args, e.g. ["_1", "_2"]
        argli = re.findall(r'\w+', args)

        # The the phylanx help string for the function
        h = phyhelpex(n)

        # Initialize the argcount data structure
        if n not in argcount:
            argcount[n] = {'patterns': {}, "help": h}

        # If we have __1 in the pattern list, we can't
        # match arguments by count.
        if re.search(r'__\d+', args):
            argcount[n]["any"] = 1

        # Keep track of the number of args in this pattern
        argcount[n]["patterns"][len(argli)] = 1

        # The first line of the docstring has the arg names.
        first = h.splitlines()[0]
        argnames = re.findall(r'\w+', first)

        # Find the section of the docstring where args
        # are documented individually.
        g = re.search(r'\nArgs:.*Returns:', h, re.DOTALL)
        if not g:
            err = "Cannot find arg definition region"
        else:
            argdefs = g.group(0)
            for argname in argnames:
                if not re.search(r"\n    [* ]?" + argname + r" ?\(", argdefs):
                    err = "Missing definition or improper indentation for arg: " + argname

        # Get the count of the args in the docstring
        argli2 = re.findall(r'\w+', first)
        argcount[n]["max"] = len(argli2)

        # Check that there's an Args: section
        if not re.search(r'\nArgs:\n\n', h):
            err = "Missing 'Args:' section or 'Args:' not followed by a blank line"

        # Check that there's a Returns: section
        if not re.search(r'\nReturns:', h):
            err = "Missing 'Returns:' section"

        # Print error
        if err:
            if not re.search(r'@Deprecated@', h):
                print("=" * 50)
                print("Error:", err)
                print(p, m, h)

# Check for arg count mismatches
for a in argcount:
    v = argcount[a]
    if "any" in v:
        continue
    if re.search(r"@Deprecated@", v["help"]):
        continue
    if v["max"] not in v["patterns"]:
        print("=" * 50)
        print("Error: argument / pattern mismatch")
        print(a, v["max"], v["patterns"])
        print(v["help"])
