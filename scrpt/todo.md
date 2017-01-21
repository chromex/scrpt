# High Level Tasks
* Debugging socket
* Lambda / closure support
* Convert to register machine
* Classes
* Static analyzers: error checking and tree re-writing for optimization
* Standard error system
* Multiple return values
* Multi-file support

# Low Level Tasks
* String table
* Function table: name, location, # args
* Ident offsets: params and locals
* Design stack format for call: args, return ip, previous fp, locals, sp, etc.
* Call syntax support: push args, make call (operand of # args and offset), pop args
* Start generating real bytecode and a decompiler
* Short jump, short branch (offset based)
* Max param check (255)
* Max bytecode check
* Max function check
* Max string check