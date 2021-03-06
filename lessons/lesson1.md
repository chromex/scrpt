# Course Overview

## TODO
* Show example languages
* Talk about features
* Give the why

## Why? Why do this? 
* True learning comes from projects, not presentations
* Most engineers I've met still haven't done a large solo project
* Poor treatment of languages and tooling in school leads understanding issues
* Because existing literature focuses too much on theory (picture dumpster fire)
* ... and not enough on reality (picture dumpster fire of a different color)

## Course Work
* Design and build a custom scripting language from the ground up
** Lexer, parser, bytecode generation, virtual machine, FFI, etc.
* Connect your shiny new language to a pre-built 2D game engine and build a game!
* Or try...
* And then go back and add some features... maybe some debugging systems... then game!

## How it Works
* Making and using a language is a multi-phase and iterative project -- you won't get it righ the first time
* At each iteration we will strive for correctness over anything else 
* Once you've built the core language, you'll integrate it into a ready made 2D game engine and then build a game using your language
* Classes are used to introduce new concepts, discuss approaches, check on progress, get help, etc.
* There will be homework with each lesson, both research and coding
* Will meet roughly once per week
* Requires commitment

## Phase 1: Designing
* Coming up with a base feature set and formal definition for it aka the grammer
* Grammer definition is an organic creation and will evolve as you build and use your language
* Understanding long term goals that can impact the syntax: error handling, built in types, classes, etc.
* Example languages: Lua, Python, Ruby, Javascript, etc. 

## Phase 2: Compilation
* Lexical analyzer -- breaking down written code into tokens that you can more easily work with
* Parser -- converting the token stream into a meaningful syntax tree
* Compiler -- translating the syntax tree into bytecode

## Phase 3: Virtual Machine
* Design and build the runtime
* Byte code interpretter
* Garbage collection
* FFI (foreign function interface)
* Debugging

## Phase 4: Game
* Integrate VM with a pre-built simple game engine
* Build a game using the new language: asteroids, defender, etc.
* Get the game running on a RasPi

## Homework: 
* Read up on (scripting) languages: Ruby, Python, Lua, Rust, C#, Lobster, etc. -- recommended to look at the various getting started documents and noting interesting language level features
* Freshen up on C -- The C Programming Language is a great book and can be found online