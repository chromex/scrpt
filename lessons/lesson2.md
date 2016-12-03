# Phase 1: Designing the language

## Base Goals
* Compiler and VM written entirely in C code
* Class-less language (at least initially)
* Imperitive style language
* Reference counted memory management
* Untyped 
* Value types: number, string, list, dictionary, etc.
* Foreign Function Interface 
* Leak free
* Functions as first class objects
* Standard library

## Potential Bonus Features
* Classes 
* Modules
* async / await with thread pool
* Bytecode size optimization and cache
* Atoms
* Enums
* Tail recursion
* Anonymous functions
* Ranges
* Pattern matching
* Exceptions
* Dead code removal

## How languages work
* Statements versus expressions
* Functions as units

## BNF for language
* https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form
* Goal is to create a context free grammer that can define all possible input strings
* The more specific the BNF, the easier it is to find errors in syntax while parsing
* Describe the structure of BNF, history, etc.
* Show an example BNF and walk through how it works, what it means
* Work with people to start crafting their BNF in class
* The BNF will be a living document, it will most likely have bugs and be updated
* Expression is-a statement, statement is-not-a expression
* Left recursion

## Homework:
* Start with writing out some example code for your hypothetical language. Be sure to include function calls, variable use, math, etc. Recommend implementing some known algorithsm such as sorting functions to fully flesh it out. Think about syntax features like array splits, implicit return values, etc.
* Implement your BNF. Try to test your code against it manually, make sure that especially complex math scenarios work. Bring this to the next class.