#include "Grammar.hpp"
#include "../Assert.hpp"

void test_can_be_nil()
{
  Grammar rules(R"STR(
    Root = "3" Y $End
    Y = A | "1" // comment
    A = "2" | Nil
  )STR");

  release_assert(!rules.can_be_nil("Root"));
  release_assert(rules.can_be_nil("A"));
  release_assert(rules.can_be_nil("Y"));


  rules = Grammar(R"STR(
    Root = "x" Y $End
    Y = A B C D
    A = "1" | Nil
    B = "2" | Nil
    C = "3" | Nil
    D = "4" | Nil
  )STR");

  release_assert(!rules.can_be_nil("Root"));
  release_assert(rules.can_be_nil("A"));
  release_assert(rules.can_be_nil("B"));
  release_assert(rules.can_be_nil("C"));
  release_assert(rules.can_be_nil("D"));
  release_assert(rules.can_be_nil("Y"));


  rules = Grammar(R"STR(
      Root = "x" Y
      Y = A B C D
      A = "1" | Nil
      B = "2" | Nil
      C = "3"
      D = "4" | Nil
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
        Root = "3" Y $End
        Y = A | "1"
        A = "2" | Nil
  )STR");
  release_assert(rules.first("Root") == vvs{{"\"3\""}});
  release_assert((rules.first("A") == vvs{{"\"2\""}, {"Nil"}}));
  release_assert((rules.first("Y") == vvs{{"\"2\"", "Nil"}, {"\"1\""}}));

  rules = Grammar(R"STR(
      Root = "3" Y $End
      Y = A | "1"
      A = "2" | "4"
  )STR");
  release_assert(rules.first("Root") == vvs{{"\"3\""}});
  release_assert((rules.first("A") == vvs{{"\"2\""}, {"\"4\""}}));
  release_assert((rules.first("Y") == vvs{{"\"2\"", "\"4\""}, {"\"1\""}}));

  rules = Grammar(R"STR(
      Root = "x" Y $End
      Y = A B C D
      A = "1" | Nil
      B = "2" | Nil
      C = "3"
      D = "4" | Nil
  )STR");
  release_assert((rules.first("A") == vvs{{"\"1\""}, {"Nil"}}));
  release_assert((rules.first("B") == vvs{{"\"2\""}, {"Nil"}}));
  release_assert(rules.first("C") == vvs{{"\"3\""}});
  release_assert((rules.first("D") == vvs{{"\"4\""}, {"Nil"}}));
  release_assert((rules.first("Y") == vvs{{"\"1\"", "Nil", "\"2\"", "\"3\""}}));

  rules = Grammar(R"STR(
      Root = "x" Y $End
      Y = A B C D
      A = "1" | Nil
      B = "2" | Nil
      C = "3" | Nil
      D = "4" | Nil
  )STR");
  release_assert((rules.first("A") == vvs{{"\"1\""}, {"Nil"}}));
  release_assert((rules.first("B") == vvs{{"\"2\""}, {"Nil"}}));
  release_assert((rules.first("C") == vvs{{"\"3\""}, {"Nil"}}));
  release_assert((rules.first("D") == vvs{{"\"4\""}, {"Nil"}}));
  release_assert((rules.first("Y") == vvs{{"\"1\"", "Nil", "\"2\"", "\"3\"", "\"4\""}}));
}

using ss = std::unordered_set<std::string>;

void test_follow()
{
  Grammar rules(R"STR(
      Root = A Y $End
      Y = A | "1"
      A = "2" | "3"
  )STR");

  release_assert((rules.follow("Root") == ss{}));
  release_assert((rules.follow("A") == ss{"\"1\"", "\"2\"", "\"3\"", "$End"}));
  release_assert((rules.follow("Y") == ss{"$End"}));

  rules = Grammar(R"STR(
      Root = "x" Y $End
      Y = A B C D
      A = "1" | Nil
      B = "2" | Nil
      C = "3"
      D = "4" | Nil
  )STR");

  release_assert((rules.follow("Root") == ss{}));
  release_assert((rules.follow("Y") == ss{"$End"}));
  release_assert((rules.follow("A") == ss{"\"2\"", "\"3\""}));
  release_assert((rules.follow("B") == ss{"\"3\""}));
  release_assert((rules.follow("C") == ss{"\"4\"", "$End"}));
}

void test()
{
  test_can_be_nil();
  test_first();
  test_follow();
}