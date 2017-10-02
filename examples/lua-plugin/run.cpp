// -*- mode:C++ { } tab-width:4 { } c-basic-offset:4 { } indent-tabs-mode:nil -*-

/*
 * Copyright (C) 2015 iCub Facility
 * Authors: Ali Paikan
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 *
 */

#include <stdio.h>
#include <rtf/TestCase.h>
#include <rtf/TestResult.h>
#include <rtf/ConsoleListener.h>
#include <rtf/TestRunner.h>

#include <rtf/lua/LuaPluginLoader.h>


using namespace RTF;
using namespace RTF::plugin;

int main(int argc, char *argv[]) {

    if(argc < 2) {
        printf("Usage: %s <lua plugin file name>\n", argv[0]);
        printf("for example: %s mytest.lua\n", argv[0]);
        return 0;
    }

    // load the test case plugin
    printf("Loading the plugin... \n");
    LuaPluginLoader loader;
    TestCase* test = loader.open(argv[1]);
    if(test == NULL) {
        printf("%s\n", loader.getLastError().c_str());
        return 0;
    }

    // create a test listener to collect the result
    ConsoleListener listener(false);

    // create a test result and add the listeners
    TestResult result;
    result.addListener(&listener);

    // create a test runner and run the test case
    TestRunner runner;
    runner.addTest(test);
    runner.run(result);

    return 0;
}

