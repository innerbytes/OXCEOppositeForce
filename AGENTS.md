## C++ code rules

- Those rules apply to the new code we add on top of the existing OXCE codebase. They do not require us to refactor or modify the existing code.

- In `src/OppositeForce`, write all the pointer and reference declarations as `Type *name` and `Type &name`;
- At integration points in existing OXCE files, follow the predominant pointer and reference marker style of that specific file. Do not reformat unrelated OXCE code.
- After editing any of our modules (in the `src/OppositeForce` directory and below), run `pnpm prettier` to format the code.

- Names of our functions should be clear of what they do and always start with a verb. getSomething, setSomething, calculateSomething, etc. Name of functions that return boolean can start with "is", "has", "can", or "should", or other appropriate verbs that convey their purpose - "lacksSomething", "needsSomething", "requiresSomething", etc.
- Names of our local variables should never use "of" prefix. The variable name should clearly indicate what it represents, not how it is used.
- Names of the class and module members (both functions and variables) we introduce in the OXCE existing files should have "of" prefix. This does not concern members of our own classes and modules, which should follow our usual naming conventions.

## Integration Rules

- Integration of the our code to the existing OXCE code is done with the goal to make future synchronization from the upstream OXCE code easier. That's why we allow to add our lines, but not mix them with the existing code, even if it means duplicating some code.

- Keep the integration surface to minimum rule. We should strive to keep our integration code in the existing OXCE code as small and clear as possible, prefering to extract any custom needed logic into our modules.

- Every integration point to the existing code should be always done in clear blocks. Avoid any mixing with the existing OXCE code, even if for this you need to duplicate some code. The integration block should be properly separated by "OF -" comments. and new line both before and after the block.

For example:

```cpp
// ... original code before integration point

// OF - Do something new
if (OurClass::someCondition()) {
    // Your code here
}
// End OF

else {
    // Original code here
}
```

- Wrapping the existing code in "else" blocks is possible when necessary, but should be avoided by using shortcuts like continue, break, or return whenever possible. The "End OF" comment should still end before the original code else block, even if it was added by us.

- If integration point is just one line, no "End OF" comment is necessary, but the new lines before and after the integration point should still be there.

- #include and using directives for our code modules do not require any "OF -" comments, but should be still separated by new lines from the existing code.
