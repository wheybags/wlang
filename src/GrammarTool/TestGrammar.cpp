#include "Grammar.hpp"
#include "../Common/Assert.hpp"

void test_can_be_nil()
{
  Grammar rules(R"STR(
    Root = "3" Y $End;
    Y = A |"1"; // comment
    A = "2" | Nil;
  )STR");

  release_assert(!rules.can_be_nil("Root"));
  release_assert(rules.can_be_nil("A"));
  release_assert(rules.can_be_nil("Y"));


  rules = Grammar(R"STR(
    Root = "x" Y $End;
    Y = A B C D;
    A = "1" |
        Nil;
    B = "2" | Nil;
    C = "3" | Nil;
    D = "4" | Nil;
  )STR");

  release_assert(!rules.can_be_nil("Root"));
  release_assert(rules.can_be_nil("A"));
  release_assert(rules.can_be_nil("B"));
  release_assert(rules.can_be_nil("C"));
  release_assert(rules.can_be_nil("D"));
  release_assert(rules.can_be_nil("Y"));


  rules = Grammar(R"STR(
      Root = "x" Y;
      Y = A B C D;
      A = "1" | Nil;
      B = "2" | Nil;
      C = "3";
      D = "4" | Nil;
  )STR");

  release_assert(!rules.can_be_nil("Root"));
  release_assert(rules.can_be_nil("A"));
  release_assert(rules.can_be_nil("B"));
  release_assert(!rules.can_be_nil("C"));
  release_assert(rules.can_be_nil("D"));
  release_assert(!rules.can_be_nil("Y"));
}

using vvs = std::vector<std::vector<std::string>>;

void test_first()
{
  Grammar rules(R"STR(
        Root = "3" Y $End;
        Y = A | "1";
        A = "2" | Nil;
  )STR");
  release_assert(rules.first("Root") == vvs{{"\"3\""}});
  release_assert((rules.first("A") == vvs{{"\"2\""}, {}}));
  release_assert((rules.first("Y") == vvs{{"\"2\""}, {"\"1\""}}));

  rules = Grammar(R"STR(
      Root = "3" Y $End;
      Y = A | "1";
      A = "2" | "4";
  )STR");
  release_assert(rules.first("Root") == vvs{{"\"3\""}});
  release_assert((rules.first("A") == vvs{{"\"2\""}, {"\"4\""}}));
  release_assert((rules.first("Y") == vvs{{"\"2\"", "\"4\""}, {"\"1\""}}));

  rules = Grammar(R"STR(
      Root = "x" Y $End;
      Y = A B C D;
      A = "1" | Nil;
      B = "2" | Nil;
      C = "3";
      D = "4" | Nil;
  )STR");
  release_assert((rules.first("A") == vvs{{"\"1\""}, {}}));
  release_assert((rules.first("B") == vvs{{"\"2\""}, {}}));
  release_assert(rules.first("C") == vvs{{"\"3\""}});
  release_assert((rules.first("D") == vvs{{"\"4\""}, {}}));
  release_assert((rules.first("Y") == vvs{{"\"1\"", "\"2\"", "\"3\""}}));

  rules = Grammar(R"STR(
      Root = "x" Y $End;
      Y = A B C D;
      A = "1" | Nil;
      B = "2" | Nil;
      C = "3" | Nil;
      D = "4" | Nil;
  )STR");
  release_assert((rules.first("A") == vvs{{"\"1\""}, {}}));
  release_assert((rules.first("B") == vvs{{"\"2\""}, {}}));
  release_assert((rules.first("C") == vvs{{"\"3\""}, {}}));
  release_assert((rules.first("D") == vvs{{"\"4\""}, {}}));
  release_assert((rules.first("Y") == vvs{{"\"1\"", "\"2\"", "\"3\"", "\"4\""}}));

  rules = Grammar(R"STR(
      Root = A "X" | B "Y";
      A = "1" | Nil;
      B = "2" | Nil;
  )STR");

  release_assert((rules.first("Root") == vvs{{"\"1\"", "\"X\""}, {"\"2\"", "\"Y\""}}));
}

using ss = std::unordered_set<std::string>;

void test_follow()
{
  Grammar rules(R"STR(
      Root = A Y $End;
      Y = A | "1";
      A = "2" | "3";
  )STR");

  release_assert((rules.follow("Root") == ss{}));
  release_assert((rules.follow("A") == ss{"\"1\"", "\"2\"", "\"3\"", "$End"}));
  release_assert((rules.follow("Y") == ss{"$End"}));

  rules = Grammar(R"STR(
      Root = "x" Y $End;
      Y = A B C D;
      A = "1" | Nil;
      B = "2" | Nil;
      C = "3";
      D = "4" | Nil;
  )STR");

  release_assert((rules.follow("Root") == ss{}));
  release_assert((rules.follow("Y") == ss{"$End"}));
  release_assert((rules.follow("A") == ss{"\"2\"", "\"3\""}));
  release_assert((rules.follow("B") == ss{"\"3\""}));
  release_assert((rules.follow("C") == ss{"\"4\"", "$End"}));
}

void testReturnType()
{
  Grammar rules(R"STR(
    Root<{R*}> = "3" Y $End;
    Y<{std::vector<something>}> = A | "1";
    A = "2" | Nil;
  )STR");

  release_assert(rules.getRules().at("Root").returnType == "R*");
  release_assert(rules.getRules().at("Y").returnType == "std::vector<something>");
}

void testCodeInsert()
{
  {
    Grammar rules(R"STR(
      Root<{Root*}> = {{test code;}} FuncList $End;
      FuncList<{std::vector<Func*>}> = "A";
    )STR");

    release_assert(rules.getRules().at("Root").productions[0][0].codeInsertBefore == "test code;");
    release_assert(rules.getRules().at("Root").productions[0][0].codeInsertAfter.empty());
    release_assert(rules.getRules().at("Root").productions[0][1].codeInsertBefore.empty());
    release_assert(rules.getRules().at("Root").productions[0][1].codeInsertAfter.empty());

    release_assert(rules.getRules().at("FuncList").productions[0][0].codeInsertBefore.empty());
    release_assert(rules.getRules().at("FuncList").productions[0][0].codeInsertAfter.empty());
    release_assert(rules.getRules().at("FuncList").productions[0][1].codeInsertBefore.empty());
    release_assert(rules.getRules().at("FuncList").productions[0][1].codeInsertAfter.empty());
  }

  {
    Grammar rules(R"STR(
      Root<{Root*}> = FuncList {{test code;}} "b" $End;
      FuncList<{std::vector<Func*>}> = "A";
    )STR");

    release_assert(rules.getRules().at("Root").productions[0][0].codeInsertBefore.empty());
    release_assert(rules.getRules().at("Root").productions[0][0].codeInsertAfter.empty());
    release_assert(rules.getRules().at("Root").productions[0][1].codeInsertBefore == "test code;");
    release_assert(rules.getRules().at("Root").productions[0][1].codeInsertAfter.empty());
    release_assert(rules.getRules().at("Root").productions[0][2].codeInsertBefore.empty());
    release_assert(rules.getRules().at("Root").productions[0][2].codeInsertAfter.empty());

    release_assert(rules.getRules().at("FuncList").productions[0][0].codeInsertBefore.empty());
    release_assert(rules.getRules().at("FuncList").productions[0][0].codeInsertAfter.empty());
    release_assert(rules.getRules().at("FuncList").productions[0][1].codeInsertBefore.empty());
    release_assert(rules.getRules().at("FuncList").productions[0][1].codeInsertAfter.empty());
  }

  {
    Grammar rules(R"STR(
      Root<{Root*}> = FuncList $End;
      FuncList<{std::vector<Func*>}> = "A" "B"  {{test code;}};
    )STR");

    release_assert(rules.getRules().at("Root").productions[0][0].codeInsertBefore.empty());
    release_assert(rules.getRules().at("Root").productions[0][0].codeInsertAfter.empty());
    release_assert(rules.getRules().at("Root").productions[0][1].codeInsertBefore.empty());
    release_assert(rules.getRules().at("Root").productions[0][1].codeInsertAfter.empty());

    release_assert(rules.getRules().at("FuncList").productions[0][0].codeInsertBefore.empty());
    release_assert(rules.getRules().at("FuncList").productions[0][0].codeInsertAfter.empty());
    release_assert(rules.getRules().at("FuncList").productions[0][1].codeInsertBefore.empty());
    release_assert(rules.getRules().at("FuncList").productions[0][1].codeInsertAfter == "test code;");
  }

  {
    Grammar rules(R"STR(
      A = "A" | "B" B; {{test code;}}
      B = Nil;
    )STR");

    release_assert(rules.getRules().at("A").codeInsertAfter == "test code;");
    release_assert(rules.getRules().at("B").codeInsertAfter.empty());
  }

  {
    Grammar rules(R"STR(
      A = "A" | "B" B;
      B = Nil; {{test code;}}
    )STR");

    release_assert(rules.getRules().at("A").codeInsertAfter.empty());
    release_assert(rules.getRules().at("B").codeInsertAfter == "test code;");
  }

  {
    Grammar rules(R"STR(
      A = "A" | "B" B;
      B {{test code before;}} = Nil; {{test code after;}}
    )STR");

    release_assert(rules.getRules().at("B").codeInsertBefore == "test code before;");
    release_assert(rules.getRules().at("B").codeInsertAfter == "test code after;");
  }
}

void testRuleParameters()
{
  {
    Grammar rules(R"STR(
      A = "B" B<{1, 2}>;
      B<{void}><{int x, int y}> = "C" | Nil;
    )STR");

    release_assert(rules.getRules().at("A").productions[0][1].parameters == "1, 2");
    release_assert(rules.getRules().at("B").arguments == "int x, int y");
  }
}

void test()
{
  testRuleParameters();
  testCodeInsert();
  testReturnType();
  test_can_be_nil();
  test_first();
  test_follow();
}