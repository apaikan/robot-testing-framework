"""Type stubs for the robottestingframework C extension module.

The real implementation is injected by the Robot Testing Framework C++ loader.
Add ``import robottestingframework`` at the top of your test file to enable
IDE auto-complete and type checking.

Example::

    import robottestingframework

    class TestCase:
        def setup(self, param: str) -> bool:
            robottestingframework.testReport("Setting up...")
            return True

        def run(self) -> None:
            robottestingframework.testCheck(1 + 1 == 2, "basic arithmetic")
            robottestingframework.testCheck(len("hi") == 2, "string length")

        def tearDown(self) -> None:
            robottestingframework.testReport("Done.")
"""

def setName(name: str) -> None:
    """Set the test case name reported by the framework.

    Args:
        name: The new name for this test case.
    """
    ...

def assertError(message: str) -> None:
    """Raise a test *error* and stop execution immediately.

    Use this for unexpected / environmental failures (e.g. cannot connect to
    hardware).  The test is marked as errored, not simply failed.

    Args:
        message: Human-readable description of the error.
    """
    ...

def assertFail(message: str) -> None:
    """Raise a test *failure* and stop execution immediately.

    Use this when the test logic determines the result is definitively wrong
    and continuing is pointless.

    Args:
        message: Human-readable description of the failure.
    """
    ...

def testReport(message: str) -> None:
    """Emit an informational message without affecting pass/fail status.

    Args:
        message: Human-readable message to include in the test report.
    """
    ...

def testCheck(condition: bool, message: str) -> None:
    """Check a condition and record a failure if it is ``False``.

    Unlike :func:`assertFail`, this does **not** stop test execution —
    subsequent checks in the same ``run()`` call will still be evaluated.

    Args:
        condition: The boolean expression to evaluate.
        message:   Description shown in the report for this check.
    """
    ...
