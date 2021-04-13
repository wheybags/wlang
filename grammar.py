import typing

# Man, I forgot a lot of my compiler design theory since college...
# https://www.tutorialspoint.com/compiler_design/compiler_design_syntax_analysis.htm
# https://stackoverflow.com/questions/20317198/purpose-of-first-and-follow-sets-in-ll1-parsers

grammar = """
  Root                = FuncList
  FuncList            = Func FuncList'
  FuncList'           = FuncList | Nil
  Func                = Type $Id "(" Func'
  Func'               = ArgList ")" StatementList | ")" StatementList
  StatementList       = Statement ";" | "{" Statement ";" StatementList "}"
  Statement           = "return" Expression | $Id Statement'
  
  //                    Decl  Assign 
  Statement'          = $Id | Expression'' "=" Expression 
  Type                = $Id
  ArgList             = Arg ArgList'
  ArgList'            = "," ArgList | Nil
  Arg                 = Type $Id
  Expression          = $Id Expression' | $Int32 Expression'
  Expression'         = Expression'' | Nil
  Expression''        = "==" Expression
"""


def str_to_basic(str_table):
    rules_temp = {}

    for line in str_table.splitlines():
        line = line.split("//")[0]
        line = line.split()

        if len(line) == 0:
            continue

        productions = []
        acc = []

        assert line[1] == "="
        current = line[2:]
        while len(current):
            if current[0] == '|':
                productions.append(acc)
                acc = []
            else:
                acc.append(current[0])
            current = current[1:]

        if len(acc):
            productions.append(acc)

        rules_temp[line[0]] = productions

    return rules_temp


def make_grammar(str_table: str) -> typing.Dict[str, "NonTerminal"]:
    rules_temp = str_to_basic(str_table)
    rules: typing.Dict[str, NonTerminal] = {key: NonTerminal(key) for key in rules_temp.keys()}

    for name in rules:
        for prod_index in range(len(rules_temp[name])):
            production = rules_temp[name][prod_index]
            if len(production) > 1 and 'Nil' in production:
                raise Exception('Nil can only appear alone')
            if production == ['Nil'] and prod_index != len(rules_temp[name]) -1:
                raise Exception('Nil has to be the last production')

            for i in range(len(production)):
                if production[i] != 'Nil' and production[i][0].isupper():
                    production[i] = rules[production[i]]

            rules[name].productions.append(production)

    return rules


Production = typing.List[typing.Union[str, "NonTerminal"]]


class NonTerminal:
    def __init__(self, name: str):
        self.name = name
        self.productions: typing.List[Production] = []

    def rules_str(self):
        strs = []
        for production in self.productions:
            strs += [str(x) for x in production]
        return f"{self} = {' | '.join(strs)}"

    def __str__(self):
        return f"NT({self.name})"

    def __repr__(self):
        return self.__str__()


class Grammar:
    def __init__(self, str_table: str):
        self.rules: typing.Dict[str, NonTerminal] = make_grammar(str_table)

    def can_be_nil(self, name: str) -> bool:
        for production in self.rules[name].productions:
            if production[0] == 'Nil':
                return True
            elif isinstance(production[0], NonTerminal):
                test_prod = production
                while True:
                    if not isinstance(test_prod[0], NonTerminal):
                        break
                    if not self.can_be_nil(test_prod[0].name):
                        break

                    test_prod = test_prod[1:]

                    if len(test_prod) == 0:
                        return True

        return False

    def _first_internal(self, name: str) -> typing.List[typing.List[str]]:
        retval = []
        for production in self.rules[name].productions:
            prod_temp = production
            prod_firsts = []
            while len(prod_temp):
                if not isinstance(prod_temp[0], NonTerminal):
                    if prod_temp[0] != 'Nil':
                        prod_firsts.append(prod_temp[0])
                    break

                for x in self.first(prod_temp[0].name):
                    for item in x:
                        prod_firsts.append(item)

                if not self.can_be_nil(prod_temp[0].name):
                    break

                prod_temp = prod_temp[1:]

            if len(prod_firsts):
                retval.append(prod_firsts)

        return retval

    def first(self, name: str) -> typing.List[typing.List[str]]:
        retval = self._first_internal(name)
        tmp = set({})
        for x in retval:
            for item in x:
                if item in tmp:
                    raise Exception("duplicate first")
                tmp.add(item)
        return retval

    def _pad(self, name: str) -> str:
        max_len = 0
        for other_name in self.rules:
            max_len = max(len(other_name), max_len)
        return ' ' * (max_len - len(name))

    def str_first(self, name: str) -> str:
        strs = [f"{name} {self._pad(name)}="]
        for prod_firsts in self.first(name):
            for item in prod_firsts:
                strs.append(item)
            strs.append('|')
        strs = strs[:-1]
        return " ".join(strs)

    def follow(self, name: str, _visited=None) -> typing.Set[str]:
        if _visited is None:
            _visited = set()

        if name in _visited:
            return set()

        _visited.add(name)

        follows: typing.Set[str] = set()
        nt = self.rules[name]

        if name == 'Root':
            follows.add('$End')

        for other_name in self.rules:
            other_nt = self.rules[other_name]
            for production in other_nt.productions:
                for i in range(len(production)):
                    if nt is production[i]:
                        prod_temp = production[i:]
                        while True:
                            if len(prod_temp) == 1:
                                if isinstance(prod_temp[0], str) or prod_temp[0] is nt or self.can_be_nil(prod_temp[0].name):
                                    follows = follows.union(self.follow(other_nt.name, _visited))
                                break
                            else:
                                next_symbol = prod_temp[1]
                                if isinstance(next_symbol, NonTerminal):
                                    for x in self.first(next_symbol.name):
                                        for item in x:
                                            follows.add(item)
                                    if not self.can_be_nil(next_symbol.name):
                                        break
                                else:
                                    follows.add(next_symbol)
                                    break

                            prod_temp = prod_temp[1:]
        return follows

    def str_follow(self, name: str) -> str:
        strs = [f"{name} {self._pad(name)}="]
        for item in self.follow(name):
            strs.append(item)
        return " ".join(strs)


def test_can_be_nil():
    rules = Grammar("""
        Root = "3" Y
        Y = A | "1"
        A = "2" | Nil
        """)
    assert not rules.can_be_nil('Root')
    assert rules.can_be_nil('A')
    assert rules.can_be_nil('Y')

    rules = Grammar("""
        Root = "x" Y
        Y = A B C D
        A = "1" | Nil
        B = "2" | Nil
        C = "3" | Nil
        D = "4" | Nil
        """)
    assert not rules.can_be_nil('Root')
    assert rules.can_be_nil('A')
    assert rules.can_be_nil('B')
    assert rules.can_be_nil('C')
    assert rules.can_be_nil('D')
    assert rules.can_be_nil('Y')

    rules = Grammar("""
        Root = "x" Y
        Y = A B C D
        A = "1" | Nil
        B = "2" | Nil
        C = "3"
        D = "4" | Nil
        """)
    assert not rules.can_be_nil('Root')
    assert rules.can_be_nil('A')
    assert rules.can_be_nil('B')
    assert not rules.can_be_nil('C')
    assert rules.can_be_nil('D')
    assert not rules.can_be_nil('Y')


def test_first():
    rules = Grammar("""
        Root = "3" Y
        Y = A | "1"
        A = "2" | Nil
        """)
    assert rules.first('Root') == [['"3"']]
    assert rules.first('A') == [['"2"']]
    assert rules.first('Y') == [['"2"'], ['"1"']]

    rules = Grammar("""
        Root = "3" Y
        Y = A | "1"
        A = "2" | "4"
        """)
    assert rules.first('Root') == [['"3"']]
    assert rules.first('A') == [['"2"'], ['"4"']]
    assert rules.first('Y') == [['"2"', '"4"'], ['"1"']]

    rules = Grammar("""
        Root = "x" Y
        Y = A B C D
        A = "1" | Nil
        B = "2" | Nil
        C = "3"
        D = "4" | Nil
        """)
    assert rules.first('A') == [['"1"']]
    assert rules.first('B') == [['"2"']]
    assert rules.first('C') == [['"3"']]
    assert rules.first('D') == [['"4"']]
    assert rules.first('Y') == [['"1"', '"2"', '"3"']]

    rules = Grammar("""
        Root = "x" Y
        Y = A B C D
        A = "1" | Nil
        B = "2" | Nil
        C = "3" | Nil
        D = "4" | Nil
        """)
    assert rules.first('A') == [['"1"']]
    assert rules.first('B') == [['"2"']]
    assert rules.first('C') == [['"3"']]
    assert rules.first('D') == [['"4"']]
    assert rules.first('Y') == [['"1"', '"2"', '"3"', '"4"']]


def test_follow():
    rules = Grammar("""
        Root = A Y
        Y = A | "1"
        A = "2" | "3"
        """)

    assert rules.follow('Root') == {'$End'}
    assert rules.follow('A') == {'"1"', '"2"', '"3"', '$End'}
    assert rules.follow('Y') == {'$End'}

    rules = Grammar("""
        Root = "x" Y
        Y = A B C D
        A = "1" | Nil
        B = "2" | Nil
        C = "3"
        D = "4" | Nil
        """)

    assert rules.follow('Root') == {'$End'}
    assert rules.follow('Y') == {'$End'}
    assert rules.follow('A') == {'"2"', '"3"'}
    assert rules.follow('B') == {'"3"'}
    assert rules.follow('C') == {'"4"', '$End'}


def test():
    test_can_be_nil()
    test_first()
    test_follow()
    print("tests passed!")


def main():
    rules = Grammar(grammar)

    print("Firsts:")
    for name in rules.rules:
        print('   ', rules.str_first(name))

    print()
    print("Follows:")
    for name in rules.rules:
        print('   ', rules.str_follow(name))


if __name__ == "__main__":
    test()
    main()
