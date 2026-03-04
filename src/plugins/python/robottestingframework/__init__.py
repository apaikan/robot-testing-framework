# This package exists solely to make 'import robottestingframework' resolvable
# by IDEs and type checkers during development.
#
# At runtime, when a test is loaded by the Robot Testing Framework C++ runner,
# the C++ code injects the real C extension module into the test's global
# namespace, overriding whatever this package provides.
#
# Usage in test files:
#   import robottestingframework
#
#   class TestCase:
#       def run(self) -> None:
#           robottestingframework.testCheck(1 + 1 == 2, "basic arithmetic")
