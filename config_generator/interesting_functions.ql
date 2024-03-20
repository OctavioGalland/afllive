import cpp

bindingset[name]
predicate isNameInteresting (string name) {
    name.regexpMatch(".*[a-z0-9]To[A-Z0-9]") or
    name.regexpMatch(".*[a-z]2[A-Z].*") or
    exists(string s | s = name.toLowerCase() and
        not s.regexpMatch(".*/thirdparty/.*") and
        not s.regexpMatch(".*test.*") and
        not s.regexpMatch(".*read.*") and
        (s.regexpMatch(".*buf.*") or
         s.regexpMatch(".*parse.*") or
         s.regexpMatch(".*decode.*") or
         s.regexpMatch(".*asm.*") or
         s.regexpMatch(".*serialize.*") or
         s.regexpMatch(".*bytes.*") or
         s.regexpMatch(".*convert.*") or
         s.regexpMatch(".*transform.*") or
         s.regexpMatch(".*_to_.*") or
         s.regexpMatch(".*open.*") or
         s.regexpMatch(".*pack.*") or
         s.regexpMatch(".*file.*") or
         s.regexpMatch(".*crypt.*") or
         s.regexpMatch(".*tokenize.*") or
         s.regexpMatch(".*encode.*") or
         s.regexpMatch(".*digest.*") or
         s.regexpMatch(".*verify.*") or
         s.regexpMatch(".*update.*") or
         s.regexpMatch(".*compress.*") or
         s.regexpMatch(".*extract.*") or
         s.regexpMatch(".*mangle.*") or
         s.regexpMatch(".*read.*") or
         s.regexpMatch(".*file.*") or
         s.regexpMatch(".*process.*")))
}

// this actually catches any type that looks like "char\**", but let's assume
// that char pointer with more than two levels of indirection are somewhat rare
predicate isCharOrInt8Ptr (Type t) {
    t instanceof PointerType and
    exists(PointerType pt | pt = t and
        pt.getBaseType().getSize() = 1 or
        isCharOrInt8Ptr(pt))
}

bindingset[name]
predicate doesNameSuggestSize (string name) {
    exists(string s | s = name.toLowerCase() and (
        s.regexpMatch(".*len.*") or
        s.regexpMatch(".*size.*") or
        s.regexpMatch("[ls].*") or
        (s.regexpMatch(".*[ls]") and not s.regexpMatch(".*options"))))
}

bindingset[name]
predicate doesNameSuggestFile (string name) {
    exists(string s | s = name.toLowerCase() and (
        s.regexpMatch(".*file.*") or
        s.regexpMatch(".*url.*")
    ))
}

predicate areParametersBufferAndLength (Function f, Parameter p1, Parameter p2) {
    p1 = f.getAParameter() and
    p2 = f.getAParameter() and
    (p1.getIndex() - p2.getIndex()).abs() = 1 and
    isCharOrInt8Ptr(p1.getType()) and
    p2.getType() instanceof IntType and
    not doesNameSuggestFile(p1.getName()) and
    doesNameSuggestSize(p2.getName())
}

predicate isParameterFile (Function f, Parameter p1) {
    p1 = f.getAParameter() and
    isCharOrInt8Ptr(p1.getType()) and
    doesNameSuggestFile(p1.getName())
}

predicate takesBufferAndLength (Function f) {
    exists(Parameter p1, Parameter p2 | 
        areParametersBufferAndLength(f, p1, p2))
}

predicate takesFileName (Function f) {
    exists(Parameter p1 | 
        isParameterFile(f, p1))
}

predicate isInteresting (Function f) {
    isNameInteresting(f.getName()) and
    (takesBufferAndLength(f) or
     takesFileName(f))
}

predicate isTopLevelInteresting (Function f) {
    isInteresting(f) and
    not(exists(Function f2, FunctionCall fc |
        isInteresting(f2) and
        fc.getEnclosingFunction() = f2 and
        fc.getTarget() = f))
}

from Function f
where
    isTopLevelInteresting(f) and
    not f.getFile().getAbsolutePath().regexpMatch(".*/third_party/.*") and 
    not f.getFile().getAbsolutePath().regexpMatch(".*/test/.*") and
    not f.getFile().getAbsolutePath().regexpMatch(".*/examples/.*") and
    not f.getFile().getAbsolutePath().regexpMatch(".*/config/.*") and
    not f.getFile().getAbsolutePath().regexpMatch(".*/3rdparty/.*") and
    not f.getFile().getAbsolutePath().regexpMatch(".*/usr/.*") and
    not f.getFile().getAbsolutePath().regexpMatch(".*/apps/.*")
select f, f.getNumberOfParameters(), f.getParameterString()