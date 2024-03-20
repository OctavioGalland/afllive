import sys
import json
import string

# ConstraintList -> Constraint (, Constraint)*
# Constraint -> Lefthandside Relation Righthandside
# Lefthandside -> num
# Relation -> d?(<|>|=)
# Righthandside -> num | z | p | c | r | f

# Maybe not ideal, but using something like PLY seemed overkill
# Would be nice to properly tokenize before parsing, as well as implementing proper error reporting
class ConstraintListParser:
    def __init__ (self, buf):
        self._buf = buf

    def _consumeChar (self):
        res = self._buf[0]
        self._buf = self._buf[1:]
        return res

    def _peekChar (self):
        return self._buf[0]

    def _isWhitespace (self, char):
        return char in [' ', '\t', '\n']

    def _isAtWhitespace (self):
        return self._isWhitespace(self._peekChar())

    def _isAtDigit (self):
        return self._peekChar() in string.digits

    def _skipWhitespace (self):
        while not self._eof() and self._isAtWhitespace():
            self._consumeChar()

    def _eof (self):
        return len(self._buf) == 0

    def num (self):
        "Terminal symbol representing a number"
        res = ''
        self._skipWhitespace()
        while not self._eof() and self._isAtDigit():
            res += self._consumeChar()
        return int(res)

    def Relation (self):
        "Relation -> d?(<|>|=)"
        self._skipWhitespace()

        res = {}

        rel = self._consumeChar()
        if rel == 'd':
            res['deref'] = True
            rel = self._consumeChar()

        if rel == '<':
            res['rel'] = 'le'
        elif rel == '>':
            res['rel'] = 'ge'
        elif rel == '=':
            res['rel'] = 'eq'
        else:
            raise SyntaxError(f'Unexpected character {rel} while parsing relation')

        return res

    def Lefthandside (self):
        "Lefthandside -> num"
        self._skipWhitespace()
        return self.num()

    def Righthandside (self):
        "Righthandside -> num | z | p | c | r | u"
        self._skipWhitespace()
        constants = {
            'z': {'constant': 0},
            'c': {'constant': 4096},
            'p': {'constant': 1},
            'r': 'rec',
            'f': 'file',
			'u': {'constant': 18446744073709551615}
        }
        if self._isAtDigit():
            return self.num()
        firstChar = self._consumeChar()
        if firstChar in constants:
            return constants[firstChar]
        else:
            raise SyntaxError(f'Unexpected right hand side {firstChar}')

    def Constraint (self):
        "Constraint -> Lefthandside Relation Righthandside"
        self._skipWhitespace()
        lhs = self.Lefthandside()
        rel = self.Relation()
        rhs = self.Righthandside()
        rel['lhs'] = lhs
        if rhs == 'file':
            rel['is_file'] = True
        else:
            rel['rhs'] = rhs
        return rel

    def ConstraintList(self):
        "ConstraintList -> Constraint (, Constraint)*"
        self._skipWhitespace()
        res = []
        try:
            res.append(self.Constraint())
        except Exception as e:
            print(f'Invalid token at {self._buf}')
            raise e
        self._skipWhitespace()
        while not self._eof():
            try:
                self._skipWhitespace()
                if self._consumeChar() != ',':
                    raise SyntaxError('Expected separator after constraint')
                self._skipWhitespace()
                res.append(self.Constraint())
                self._skipWhitespace()
            except Exception as e:
                print(f'Invalid token at {self._buf}')
                raise e
        return res

    def getConstraintList(self):
        return self.ConstraintList()


config = {
        "root": "/opt/aflliv",
        "max_millis": 5,
        "typeinfo_file": "/opt/typeinfo.json",
        "targets": [],
        "constraints": {},
        "skip": {}
}

def splitParamTypeName(param):
    namePos = max(param.rfind(' '), param.rfind('*')) + 1
    pname = param[namePos:].lower()
    ptype = param[:namePos].lower()
    return ptype, pname

def getConstraintList(functionName, paramString):
    constraints = []

    params = paramString[1:-1].split(',') # param string is quoted
    params = [splitParamTypeName(param) for param in params]
    i = 0
    while i < len(params):
        ptype, pname = params[i]
        if 'char' in ptype or ('int' in ptype and '8' in ptype):
            if 'file' in pname or 'url' in pname or 'name' in pname:
                constraints.append(f'{i} = f')
            elif 'out' not in pname:
                foundLength = False
                # TODO: dry
                if i + 1 < len(params) and not any([f'{i+1}' in c.split() for c in constraints]):
                    nextPtype, nextPname = params[i+1]
                    if (('int' in nextPtype or 'size' in nextPtype) and '*' not in nextPtype and
                        ('len' in nextPname or 'size' in nextPname or pname in nextPname)):
                        constraints = list(filter(lambda c : f'{i+1}' not in c.split(), constraints))
                        if ptype.count('*') == 1:
                            constraints.append(f'{i} = {i+1}')
                        elif ptype.count('*') == 2:
                            constraints.append(f'{i} d= {i+1}')
                        else:
                            raise ValueError(f'We cannot yet deal with triple pointers: {ptype}')
                        constraints.append(f'{i+1} < r')
                        i += 1
                        foundLength = True
                if not foundLength and i > 0 and not any([f'{i-1}' in c.split() for c in constraints]):
                    nextPtype, nextPname = params[i-1]
                    if (('int' in nextPtype or 'size' in nextPtype) and '*' not in nextPtype and
                        ('len' in nextPname or 'size' in nextPname or pname in nextPname)):
                        constraints = list(filter(lambda c : f'{i-1}' not in c.split(), constraints))
                        if ptype.count('*') == 1:
                            constraints.append(f'{i} = {i-1}')
                        elif ptype.count('*') == 2:
                            constraints.append(f'{i} d= {i-1}')
                        else:
                            raise ValueError(f'We cannot yet deal with triple pointers: {ptype}')
                        constraints.append(f'{i-1} < r')
                        i += 1
        i += 1

    return ','.join(constraints)


# This function does two things, first it tells us which parameters should be skipped from fuzzing by looking at the constraints
# (the assumption being that if we do not specify any constraints for some parameter, we want to skip it). Second, it maps the index
# of all parameters involved in some constraint to their relative index after removing the skipped parameters.
# For instance, if a function takes parameters "a0, a1, a2, a3" and we specify the constraint "1 < 3", we want to skip the paramaters
# at indexes 0 and 2 (skip becomes "[0, 1]") and have a less-than relationship between the first and second non-skipped parameters
# (so the constraint becomes "0 < 1"). Yes, this is quite awful, but it does the job.
def getSkipParamsAndTransformConstraints (paramsNum, constraintList):
    # gather which params are being used
    referencedParams = []
    for constraint in constraintList:
        lhs = constraint['lhs']
        if lhs not in referencedParams:
            referencedParams.append(lhs)
        if 'rhs' in constraint:
            rhs = constraint['rhs']
            if type(rhs) == int and rhs not in referencedParams:
                referencedParams.append(rhs)
    referencedParams.sort()

    # find how every used params maps from their absolute position to ther relative position after skipping the unused ones
    paramMap = {}
    lastParam = 0
    for param in referencedParams:
        paramMap[param] = lastParam
        lastParam += 1

    # replace each used param by its relative equivalent
    for constraint in constraintList:
        lhs = constraint['lhs']
        if lhs in paramMap:
            constraint['lhs'] = paramMap[lhs]
        if 'rhs' in constraint:
            rhs = constraint['rhs']
            if type(rhs) == int and rhs in paramMap:
                constraint['rhs'] = paramMap[rhs]

    return [i for i in range(paramsNum) if i not in referencedParams]

# on a high level:
# 1 - read data from csv file
# 2 - use signature to derive arguments
# 2.5 - maybe report cases where we cannot do this confidently and allow the user to step in?
# 3 - parse the arguments we generated into a config object
# 4 - dump the generated ai

# 1-3 can be done on a per-function basis

with open(sys.argv[1], 'r') as f:
    lines = f.read().split('\n')[1:-1]
    for line in lines:
        try:
            # functionName, amountOfArgs, paramString = line.split(',')
            fields = line.split(',')
            functionName = fields[0].replace('"', '')
            amountOfArgs = int(fields[1])
            paramString = ','.join(fields[2:])
            amountOfArgs = int(amountOfArgs)
            constraintListString = getConstraintList(functionName, paramString)
            if len(constraintListString) == 0:
                # TODO: maybe the user should also step in here?
                continue
            print(f'Constraint obtained for {line}: {constraintListString}')
        except Exception as e:
            # TODO: let the user step in and modify the generated list
            print(f'Unable to derive constraints for line: "{line}"')
            raise e
        try:
            constraintList = ConstraintListParser(constraintListString).getConstraintList()
        except Exception as e:
            print(f'Unable to parse constraints: "{constraintListString}" stemming from line "{line}"')
            raise e
        skipList = getSkipParamsAndTransformConstraints(amountOfArgs, constraintList)
        if functionName not in config["targets"]:
            config["targets"].append(functionName)
            config["constraints"][functionName] = constraintList
            config["skip"][functionName] = skipList

with open(sys.argv[2], 'w') as f:
    f.write(json.dumps(config, indent=4))

