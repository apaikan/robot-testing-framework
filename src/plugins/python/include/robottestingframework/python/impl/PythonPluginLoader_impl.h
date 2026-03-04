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


#ifndef ROBOTTESTINGFRAMEWORK_PYTHONPLUGINLOADER_IMPL_H
#define ROBOTTESTINGFRAMEWORK_PYTHONPLUGINLOADER_IMPL_H

#include <robottestingframework/TestCase.h>

#include <Python.h>
#include <string>

namespace robottestingframework {
namespace plugin {

/**
 * @brief The PythonPluginLoaderImpl loads a Python test case plug-in and
 * gives the direct access to the TestCase.
 */
class PythonPluginLoaderImpl : public robottestingframework::TestCase
{
public:
    /**
     * PythonPluginLoaderImpl constructor
     */
    PythonPluginLoaderImpl();

    /**
     *  PythonPluginLoaderImpl destructor
     */
    ~PythonPluginLoaderImpl() override;

    /**
     * @brief open Loads a test case plugin
     * @param filename the plugin filename
     * @param venvPath optional Python virtual environment path
     * @return A pointer to the test case loaded from the
     * plugin or a null pointer in case of failure.
     */
    TestCase* open(const std::string& filename, const std::string& venvPath = "");

    /**
     * @brief close Unloads the plugin and deletes any
     * allocated memory.
     */
    void close();

    /**
     * @brief getLastError gets the last error if any.
     * @return returns the last error string.
     */
    std::string getLastError();

    /**
     * @brief getFileName returns the loaded script file name
     * @return the script file name
     */
    std::string getFileName();

    /**
     * @brief setTestName set the test case name
     * @param name the test case name
     */
    void setTestName(const std::string& name);

    bool setup(int argc, char** argv) override;

    void tearDown() override;

    void run() override;

public:
    static PyObject* setName(PyObject* self, PyObject* args);
    static PyObject* assertError(PyObject* self, PyObject* args);
    static PyObject* assertFail(PyObject* self, PyObject* args);
    static PyObject* testReport(PyObject* self, PyObject* args);
    static PyObject* testCheck(PyObject* self, PyObject* args);

    static PyMethodDef testPythonMethods[];

private:
    std::string getPythonErrorString();

private:
    std::string filename;
    std::string error;

    PyObject* pyName;
    PyObject* pyModule;
    PyObject* pyDict;
    PyObject* pyClass;
    PyObject* pyInstance;
    PyObject* pyModuleRobotTestingFramework;

    // Singleton interpreter state: shared across all instances so that
    // Py_Initialize / Py_Finalize are called only once per process.
    static int  s_instanceCount;
    static bool s_venvActivated;
};

} // namespace plugin
} // namespace robottestingframework

#endif // ROBOTTESTINGFRAMEWORK_PYTHONPLUGINLOADER_IMPL_H
