# Name-only helper from GensokyoClub/th06 (CC0), commit 7411374cef282756ef5a8984ce896fb8db797e9e.
def demangle_msvc(sym):
    if len(sym) == 0:
        return sym

    if sym[0:1] == b"_":
        # Handle stdcall symbols first, those are simple. First remove the leading
        # underscore, then split on the last @ and remove everything that comes afterwards
        end_of_sym = sym.rfind(b"@")
        if end_of_sym == -1:
            # Not an stdcall, let's just not demangle it.
            return sym
        else:
            return sym[1:end_of_sym]

    if sym[0:1] != b"?":
        # Unmangled symbol?
        return sym

    # Handle CPP mangling.
    offset = 1

    # Read name. Start with special symbols
    special = None
    name = None
    if sym[offset : offset + 1] == b"?" and sym[offset + 1 : offset + 2].isdigit():
        # Special symbol.
        special = sym[offset + 1] - ord("0")
        offset += 2
    else:
        # Read a normal name.
        start_of_name = offset
        end_of_name = sym.find(b"@", offset)
        if end_of_name == -1:
            end_of_name = len(sym)
            offset = len(sym)
        else:
            offset = end_of_name + 1
        name = sym[start_of_name:end_of_name]

    # Read scope
    scope = []
    while True:
        start_of_scope = offset
        end_of_scope = sym.find(b"@", offset)
        if end_of_scope == -1:
            end_of_scope = len(sym)
            offset = len(sym)
        else:
            offset = end_of_scope + 1
        cur_scope = sym[start_of_scope:end_of_scope]
        if len(cur_scope) == 0:
            break
        scope.append(cur_scope)

    if name is not None:
        return b"::".join(scope[::-1]) + b"::" + name
    elif special == 0:
        return b"::".join(scope[::-1]) + b"::" + scope[0]
    elif special == 1:
        return b"::".join(scope[::-1]) + b"::~" + scope[0]
    else:
        return sym
