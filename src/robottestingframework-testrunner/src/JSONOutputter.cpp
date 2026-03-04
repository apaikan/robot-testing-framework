/*
 * Robot Testing Framework
 *
 * Copyright (C) 2015-2019 Istituto Italiano di Tecnologia (IIT)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include <robottestingframework/Asserter.h>
#include <robottestingframework/ResultEvent.h>

#include <JSONOutputter.h>
#include <fstream>
#include <cerrno>
#include <cstring>
#include <json.hpp>

using namespace robottestingframework;
using namespace std;
using namespace json;

#define MSG_ERROR "ERROR"
#define MSG_FAIL "FAIL"
#define MSG_REPORT "INFO"

JSONOutputter::JSONOutputter(TestResultCollector& collector, bool verbose) :
        collector(collector), verbose(verbose)
{
}

JSONOutputter::~JSONOutputter() = default;

bool JSONOutputter::write(std::string filename,
                           TestMessage* errorMsg)
{
    if (filename.empty()) {
        if (errorMsg != nullptr) {
            errorMsg->setMessage("Cannot open the file.");
            errorMsg->setDetail("Empty file name.");
        }
        return false;
    }

    string classname;
    JSON test;
    JSON suite;
    JSON report;
    JSON obj;
    obj["summary"] = Object();
    obj["tests"] = Array();
    // If there are test suite, create object!
    if (collector.suiteCount() != 0) {
        obj["suites"] = Array();
    }

    TestResultCollector::EventResultIterator itr;
    TestResultCollector::EventResultContainer events = collector.getResults();

    for (itr = events.begin(); itr != events.end(); ++itr) {
        ResultEvent* e = *itr;

        // start suite
        if (dynamic_cast<ResultEventStartSuite*>(e) != nullptr) {
            obj["suites"] = Array();
        } 
        else if (dynamic_cast<ResultEventEndSuite*>(e) != nullptr) {
            classname = e->getTest()->getName();
            if (e->getTest()->succeeded()) {
                suite["result"] = "passed!";
            } else {
                suite["result"] = "failed!";
            }
            suite["name"] = classname;
            obj["suites"].append(suite);
        }

        // start test
        else if (dynamic_cast<ResultEventStartTest*>(e) != nullptr) {
           test["reports"] = Array();
        }
        else if (dynamic_cast<ResultEventEndTest*>(e) != nullptr) {
            classname = e->getTest()->getName();
            if (e->getTest()->succeeded()) {
                test["result"] = "passed!";
            } else {
                test["result"] = "failed!";
            }
            test["name"] = classname;
            obj["tests"].append(test);
        }

       // report event
        else if (dynamic_cast<ResultEventReport*>(e) != nullptr) {
            string msg;
            string details = e->getMessage().getDetail();
            if(details != " "){
                msg = e->getMessage().getMessage() + ": " + e->getMessage().getDetail();
                if (verbose && e->getMessage().getSourceLineNumber() != 0) {
                    msg += Asserter::format(" at %d.",e->getMessage().getSourceLineNumber());
                }
                report["type"] = MSG_REPORT;
                report["msg"] = msg;
                test["reports"].append(report);
            }
        }

        // failure event
        else if (dynamic_cast<ResultEventFailure*>(e) != nullptr) {
            string msg;
            msg = e->getMessage().getMessage() +": " + e->getMessage().getDetail();
            if (verbose && e->getMessage().getSourceLineNumber() != 0) {
                msg += Asserter::format(" at %d.",e->getMessage().getSourceLineNumber());
            }
            report["type"] = MSG_FAIL;
            report["msg"] = msg;
            test["reports"].append(report);
        }

        // error event
        else if (dynamic_cast<ResultEventError*>(e) != nullptr) {
            string msg;
            msg = e->getMessage().getMessage() + ": " + e->getMessage().getDetail();
            if (verbose && e->getMessage().getSourceLineNumber() != 0) {
                msg += Asserter::format("at %d.",e->getMessage().getSourceLineNumber());
            }
            report["type"] = MSG_ERROR;
            report["msg"] = msg;
            test["reports"].append(report);
        }

    } // end for

    if (collector.suiteCount()) {
        obj["summary"]["suite_total_count"] = collector.suiteCount();
        obj["summary"]["suite_count_passed"] = collector.passedSuiteCount();
        obj["summary"]["suite_count_failed"] = collector.failedSuiteCount();
    }
    obj["summary"]["tests_total_count"] = collector.testCount();
    obj["summary"]["tests_count_passed"] = collector.passedCount();
    obj["summary"]["tests_count_failed"] = collector.failedCount();

    ofstream file(filename.c_str());
    file << obj;
    return true;
}
