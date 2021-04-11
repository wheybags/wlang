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
        for production in rules_temp[name]:
            for i in range(len(production)):
                if production[i] != 'Nil' and production[i][0].isupper():
                    production[i] = rules[production[i]]

            rules[name].productions.append(production)

    return rules


class NonTerminal:
    def __init__(self, name: str):
        self.name = name
        self.productions: typing.List[typing.List[typing.Union[str, NonTerminal]]] = []

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

    def _first_internal(self, name: str) -> typing.List[typing.List[str]]:
        retval = []
        for production in self.rules[name].productions:
            if isinstance(production[0], NonTerminal):
                prod_items = []
                for x in self.first(production[0].name):
                    for item in x:
                        prod_items.append(item)
                retval.append(prod_items)

            # TODO: handle fallthrough in case of Nil
            elif production[0] != 'Nil':
                retval.append([production[0]])
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

    # def _can_be_nil(self, items: typing.List[typing.Union[str, NonTerminal]]) -> bool:
    #     pass
    #
    #
    # def follow(self, name: str, _visited=None) -> typing.Set[str]:
    #     if _visited is None:
    #         _visited = set()
    #
    #     if name in _visited:
    #         return set()
    #
    #     _visited.add(name)
    #
    #     follows: typing.Set[str] = set()
    #     nt = self.rules[name]
    #
    #     if name == 'Root':
    #         follows.add('$End')
    #
    #     for other_name in self.rules:
    #         other_nt = self.rules[other_name]
    #         for production in other_nt.productions:
    #             for i in range(len(production)):
    #                 if nt is production[i]:
    #
    #                     if i+1 == len(production) or self._can_be_nil(production[i+1:]):
    #                         follows = follows.union(self.follow(other_nt.name, _visited))
    #
    #                     if i+1 < len(production):
    #                         # need to handle a nil symbol in the middle
    #                         if isinstance(production[i+1], NonTerminal):
    #                             for x in self.first(production[i+1].name):
    #                                 for item in x:
    #                                     follows.add(item)
    #                         else:
    #                             follows.add(production[i+1])
    #     return follows
    #
    # def str_follow(self, name: str) -> str:
    #     strs = [f"{name} {self._pad(name)}="]
    #     for item in self.follow(name):
    #         strs.append(item)
    #     return " ".join(strs)


def main():
    rules = Grammar(grammar)

    print("Firsts:")
    for name in rules.rules:
        print('   ', rules.str_first(name))

    # print()
    # print("Follows:")
    # for name in rules.rules:
    #     print('   ', rules.str_follow(name))


if __name__ == "__main__":
    main()
