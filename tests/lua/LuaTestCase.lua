--
-- Copyright (C) 2015 iCub Facility
-- Authors: Ali Paikan
-- CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
--
 
--
-- The TestCase table is used by the lua plugin loader
-- to invoke the corresponding methods:
--
-- TestCase.setup = function(options) ... return true end
-- TestCase.run = function() ... end 
-- TestCase.tearDown = function() ... end 
--
-- The following methods are for reporting, failures or assertions: 
--
-- RTF.setName(name)              : sets the test name (defualt is the test filename)
-- RTF.testReport(msg)            : reports a informative message
-- RTF.testCheck(condition, msg)  : reports the test message and marks the test as failed if condition is false 
-- RTF.testFailIf(condition, msg) : marks the test as failed and reports failure message (the reason) if condition is false
-- RTF.assertError(msg)           : throws an error exception with message
-- RTF.asserFail(msg)             : throws a failure exception with message
-- RTF.getEnvironment()           : returns the test environment params
--

--
-- setup is called before the test's run to setup 
-- the user defined fixture
-- @return Boolean (true/false uppon success or failure)
--
TestCase.setup = function(parameter)
    RTF.setName("LuaTestCase")
    return true;
end

--
-- The implementation of the test goes here
--
TestCase.run = function()
    RTF.testCheck(5>3, "Cheking RTF.testCheck")
    RTF.testReport("Cheking RTF.testFailIf")
    RTF.testFailIf(true, "testFailIf")
end


--
-- tearDown is called after the test's run to tear down
-- the user defined fixture
--
TestCase.tearDown = function()
end

