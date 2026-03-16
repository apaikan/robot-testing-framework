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
#include <robottestingframework/TestAssert.h>
#include <robottestingframework/dll/Plugin.h>
#include <robottestingframework/python/PythonPluginLoader.h>
#include <robottestingframework/python/impl/PythonPluginLoader_impl.h>

#ifdef _WIN32
#    include <stdlib.h>
#else
#    include <libgen.h>
#endif

using namespace std;
using namespace robottestingframework;
using namespace robottestingframework::plugin;


// ---------------------------------------------------------------------------
// Python 3 module definition for 'robottestingframework'.
// We create one module object per test (via PyModule_Create) and inject it
// directly into the test's namespace — no PyImport_AppendInittab needed.
// This avoids sys.modules conflicts when multiple tests run in one session.
// ---------------------------------------------------------------------------

static struct PyModuleDef s_rtfModuleDef = {
    PyModuleDef_HEAD_INIT,
    "robottestingframework",
    nullptr, /* no docstring */
    -1,      /* no per-interpreter state */
    PythonPluginLoaderImpl::testPythonMethods,
    nullptr, nullptr, nullptr, nullptr
};


// ---------------------------------------------------------------------------
// Singleton interpreter state
// ---------------------------------------------------------------------------

int  PythonPluginLoaderImpl::s_instanceCount = 0;
bool PythonPluginLoaderImpl::s_venvActivated = false;


// ---------------------------------------------------------------------------
// Method table
// ---------------------------------------------------------------------------

PyMethodDef PythonPluginLoaderImpl::testPythonMethods[] = {
    { "setName",    PythonPluginLoaderImpl::setName,    METH_VARARGS, "Setting the test name." },
    { "assertError", PythonPluginLoaderImpl::assertError, METH_VARARGS, "Error assertion." },
    { "assertFail", PythonPluginLoaderImpl::assertFail, METH_VARARGS, "Failure assertion." },
    { "testReport", PythonPluginLoaderImpl::testReport, METH_VARARGS, "report a test message." },
    { "testCheck",  PythonPluginLoaderImpl::testCheck,  METH_VARARGS, "report failure message with condition." },
    { nullptr, nullptr, 0, nullptr }
};


// ---------------------------------------------------------------------------
// PythonPluginLoaderImpl
// ---------------------------------------------------------------------------

PythonPluginLoaderImpl::PythonPluginLoaderImpl() :
        TestCase(""),
        pyName(nullptr),
        pyModule(nullptr),
        pyDict(nullptr),
        pyClass(nullptr),
        pyInstance(nullptr),
        pyModuleRobotTestingFramework(nullptr),
        m_opened(false)
{
}

PythonPluginLoaderImpl::~PythonPluginLoaderImpl()
{
    close();
}

void PythonPluginLoaderImpl::close()
{
    // Release Python objects owned by this instance
    Py_XDECREF(pyInstance);               pyInstance = nullptr;

    // Remove our module from sys.modules so the next test gets a fresh one
    if (pyModuleRobotTestingFramework != nullptr && Py_IsInitialized()) {
        PyObject* sysModules = PyImport_GetModuleDict(); // borrowed
        PyDict_DelItemString(sysModules, "robottestingframework");
        PyErr_Clear();
    }
    Py_XDECREF(pyModuleRobotTestingFramework); pyModuleRobotTestingFramework = nullptr;
    Py_XDECREF(pyModule);                 pyModule = nullptr;
    Py_XDECREF(pyName);                   pyName = nullptr;

    pyDict  = nullptr; // borrowed — do not decref
    pyClass = nullptr; // borrowed — do not decref

    // Shut down the interpreter when the last instance is destroyed.
    // Only decrement the count if this instance actually opened successfully
    // (i.e. incremented the count in open()). A fresh or already-closed
    // instance must not touch the count — otherwise the spurious decrement
    // would reach zero and call Py_Finalize() while other instances are
    // still running.
    if (m_opened) {
        m_opened = false;
        s_instanceCount--;
        if (s_instanceCount <= 0) {
            s_instanceCount  = 0;
            s_venvActivated  = false;
            if (Py_IsInitialized()) {
                Py_Finalize();
            }
        }
    }
}

TestCase* PythonPluginLoaderImpl::open(const std::string& filename,
                                        const std::string& venvPath)
{
    close();
    this->filename = filename;

    // -----------------------------------------------------------------------
    // Singleton interpreter: initialize only on the first open()
    // -----------------------------------------------------------------------
    if (s_instanceCount == 0) {
        Py_Initialize();
        s_venvActivated = false;
    }
    s_instanceCount++;
    m_opened = true;

    // -----------------------------------------------------------------------
    // Extract directory and base name of the test file
    // -----------------------------------------------------------------------
#ifdef _WIN32
    char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
    _splitpath_s(filename.c_str(),
                 drive, _MAX_DRIVE, dir, _MAX_DIR,
                 fname, _MAX_FNAME, ext, _MAX_EXT);
    string dname = string(drive) + string(dir);
    for (auto& c : dname) {
        if (c == '\\') c = '/';
    }
    string bname = fname;
#else
    char* dir_buf  = strdup(filename.c_str());
    char* name_buf = strdup(filename.c_str());
    string dname = dirname(dir_buf);
    string bname = basename(name_buf);
    free(dir_buf);
    free(name_buf);
#endif

    // -----------------------------------------------------------------------
    // Add the test file's directory to sys.path
    // -----------------------------------------------------------------------
    PyObject* sysPath = PySys_GetObject("path"); // borrowed
    PyObject* pyDname = PyUnicode_FromString(dname.c_str());
    if (pyDname) {
        PyList_Insert(sysPath, 0, pyDname);
        Py_DECREF(pyDname);
    }

    // -----------------------------------------------------------------------
    // Activate virtual environment (once per interpreter lifetime)
    // Safe: path is passed as a Python object, never embedded in a string.
    // -----------------------------------------------------------------------
    if (!venvPath.empty() && !s_venvActivated) {
        PyObject* pyMainModule = PyImport_AddModule("__main__"); // borrowed
        PyObject* pyMainDict   = PyModule_GetDict(pyMainModule); // borrowed

        PyObject* pyVenvPath = PyUnicode_FromString(venvPath.c_str());
        PyDict_SetItemString(pyMainDict, "_rtf_venv_path", pyVenvPath);
        Py_DECREF(pyVenvPath);

        int rc = PyRun_SimpleString(
            "import sys as _rtf_sys, os as _rtf_os, site as _rtf_site\n"
            "_rtf_lib = _rtf_os.path.join(_rtf_venv_path, 'lib')\n"
            "if _rtf_os.path.isdir(_rtf_lib):\n"
            "    for _rtf_d in _rtf_os.listdir(_rtf_lib):\n"
            "        _rtf_sp = _rtf_os.path.join(_rtf_lib, _rtf_d, 'site-packages')\n"
            "        if _rtf_os.path.isdir(_rtf_sp) and _rtf_sp not in _rtf_sys.path:\n"
            "            _rtf_site.addsitedir(_rtf_sp)\n"
            "del _rtf_sys, _rtf_os, _rtf_site, _rtf_lib, _rtf_venv_path\n");

        PyDict_DelItemString(pyMainDict, "_rtf_venv_path");
        PyErr_Clear(); // ignore KeyError if already deleted

        if (rc != 0) {
            error = Asserter::format("Failed to activate virtual environment at %s",
                                     venvPath.c_str());
            close();
            return nullptr;
        }
        s_venvActivated = true;
    }

    size_t lastdot = bname.find_last_of('.');
    string rawname = bname.substr(0, lastdot);

    PyObject* sysModules = PyImport_GetModuleDict(); // borrowed

    // -----------------------------------------------------------------------
    // Create a fresh 'robottestingframework' C extension module and register
    // it in sys.modules BEFORE importing the test file.
    //
    // This ensures that 'import robottestingframework' inside the test file
    // always succeeds regardless of whether the Python stub package is
    // installed — the real C extension is already in sys.modules.
    // -----------------------------------------------------------------------
    pyModuleRobotTestingFramework = PyModule_Create(&s_rtfModuleDef);
    if (pyModuleRobotTestingFramework == nullptr) {
        error = Asserter::format("Cannot create robottestingframework module because %s",
                                 getPythonErrorString().c_str());
        close();
        return nullptr;
    }

    // Store a back-pointer to 'this' as a capsule inside the module so that
    // the extension methods can retrieve it via their 'self' (= module object).
    // The capsule is added now so it is present even if the test file calls
    // RTF functions at module level (outside of method bodies).
    PyObject* capsule = PyCapsule_New(this, "PythonPluginLoaderImpl", nullptr);
    if (capsule == nullptr) {
        error = Asserter::format("Cannot create PyCapsule because %s",
                                 getPythonErrorString().c_str());
        close();
        return nullptr;
    }
    if (PyModule_AddObject(pyModuleRobotTestingFramework,
                           "PythonPluginLoaderImpl", capsule) < 0) {
        Py_DECREF(capsule);
        error = Asserter::format("Cannot add capsule to module because %s",
                                 getPythonErrorString().c_str());
        close();
        return nullptr;
    }

    // Make the module visible to 'import robottestingframework'
    PyDict_SetItemString(sysModules, "robottestingframework",
                         pyModuleRobotTestingFramework);

    // -----------------------------------------------------------------------
    // Remove any cached version of the test module from sys.modules so the
    // same filename can be loaded fresh from different directories.
    // -----------------------------------------------------------------------
    PyDict_DelItemString(sysModules, rawname.c_str());
    PyErr_Clear(); // suppress KeyError if it wasn't cached

    // -----------------------------------------------------------------------
    // Import the user's test file
    // -----------------------------------------------------------------------
    pyName = PyUnicode_FromString(rawname.c_str());
    if (pyName == nullptr) {
        error = Asserter::format("Cannot load %s because %s",
                                 filename.c_str(),
                                 getPythonErrorString().c_str());
        close();
        return nullptr;
    }

    pyModule = PyImport_Import(pyName);
    if (pyModule == nullptr) {
        error = Asserter::format("Cannot load %s because %s",
                                 filename.c_str(),
                                 getPythonErrorString().c_str());
        close();
        return nullptr;
    }

    // Also inject directly into the user module's global namespace so that
    // test files that do NOT have 'import robottestingframework' still work.
    PyObject* userDict = PyModule_GetDict(pyModule); // borrowed
    PyDict_SetItemString(userDict, "robottestingframework",
                         pyModuleRobotTestingFramework);

    // pyDict / pyClass are borrowed references — valid while pyModule is alive
    pyDict = PyModule_GetDict(pyModule);
    if (pyDict == nullptr) {
        error = Asserter::format("Cannot get module dict for %s because %s",
                                 filename.c_str(),
                                 getPythonErrorString().c_str());
        close();
        return nullptr;
    }

    pyClass = PyDict_GetItemString(pyDict, "TestCase");
    if (pyClass == nullptr) {
        error = Asserter::format("Cannot find class TestCase in %s",
                                 filename.c_str());
        close();
        return nullptr;
    }

    if ((PyCallable_Check(pyClass) == 0) ||
        (pyInstance = PyObject_CallObject(pyClass, nullptr)) == nullptr) {
        error = Asserter::format("TestCase is not defined as a callable class in %s",
                                 filename.c_str());
        close();
        return nullptr;
    }

    setTestName(bname);
    return this;
}

// ---------------------------------------------------------------------------
// Error reporting: short "Type: message" string returned to the framework,
// plus full traceback printed to stderr for debugging.
// ---------------------------------------------------------------------------
std::string PythonPluginLoaderImpl::getPythonErrorString()
{
    if (!PyErr_Occurred()) {
        return "";
    }

    PyObject* ptype     = nullptr;
    PyObject* pvalue    = nullptr;
    PyObject* ptraceback = nullptr;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

    // Build "ExcType: message" for the framework error string
    std::string result = "<unknown error>";
    if (pvalue) {
        PyObject* str = PyObject_Str(pvalue);
        if (str && PyUnicode_Check(str)) {
            const char* s = PyUnicode_AsUTF8(str);
            if (s) result = s;
        }
        Py_XDECREF(str);
    }
    if (ptype) {
        PyObject* tname = PyObject_GetAttrString(ptype, "__name__");
        if (tname && PyUnicode_Check(tname)) {
            const char* s = PyUnicode_AsUTF8(tname);
            if (s) result = std::string(s) + ": " + result;
        }
        Py_XDECREF(tname);
    }

    // Restore the exception and let PyErr_Print write the full traceback to
    // stderr — this is the standard embedded-Python way to show tracebacks.
    PyErr_Restore(ptype, pvalue, ptraceback); // hands back ownership
    PyErr_Print();                            // prints to stderr, clears error

    return result;
}

void PythonPluginLoaderImpl::setTestName(const std::string& name)
{
    Test::setName(name);
}

std::string PythonPluginLoaderImpl::getLastError()
{
    return error;
}

std::string PythonPluginLoaderImpl::getFileName()
{
    return filename;
}

bool PythonPluginLoaderImpl::setup(int argc, char** argv)
{
    PyObject* func = PyObject_GetAttrString(pyInstance, "setup");
    if (func == nullptr) {
        PyErr_Clear();
        return true;
    }

    PyObject* arglist = PyTuple_New(argc);
    if (arglist == nullptr) {
        Py_DECREF(func);
        error = Asserter::format("Cannot create arguments because %s",
                                 getPythonErrorString().c_str());
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(error);
    }

    for (int i = 0; i < argc; i++) {
        PyObject* str = PyUnicode_FromString(argv[i]);
        if (str == nullptr) {
            Py_DECREF(arglist);
            Py_DECREF(func);
            error = Asserter::format("Cannot create arguments because %s",
                                     getPythonErrorString().c_str());
            ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(error);
        }
        PyTuple_SetItem(arglist, i, str); // steals ref
    }

    PyObject* pyValue = PyObject_CallObject(func, arglist);
    Py_DECREF(arglist);
    Py_DECREF(func);

    if (pyValue == nullptr) {
        error = Asserter::format("Cannot call setup() because %s",
                                 getPythonErrorString().c_str());
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(error);
    }

    bool ret = (PyObject_IsTrue(pyValue) == 1);
    Py_DECREF(pyValue);
    return ret;
}

void PythonPluginLoaderImpl::tearDown()
{
    PyObject* func = PyObject_GetAttrString(pyInstance, "tearDown");
    if (func == nullptr) {
        PyErr_Clear();
        return;
    }

    PyObject* pyValue = PyObject_CallObject(func, nullptr);
    Py_DECREF(func);

    if (pyValue == nullptr) {
        error = Asserter::format("Cannot call tearDown() because %s",
                                 getPythonErrorString().c_str());
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(error);
    }
    Py_DECREF(pyValue);
}

void PythonPluginLoaderImpl::run()
{
    PyObject* func = PyObject_GetAttrString(pyInstance, "run");
    if (func == nullptr) {
        error = Asserter::format("Cannot find run() method because %s",
                                 getPythonErrorString().c_str());
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(error);
    }

    PyObject* pyValue = PyObject_CallObject(func, nullptr);
    Py_DECREF(func);

    if (pyValue == nullptr) {
        error = Asserter::format("Cannot call run() because %s",
                                 getPythonErrorString().c_str());
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(error);
    }
    Py_DECREF(pyValue);
}


// ---------------------------------------------------------------------------
// C extension methods exposed to Python as robottestingframework.*
// In Python 3, 'self' for module-level functions is the module object.
// We retrieve the impl pointer from the capsule stored in the module.
// ---------------------------------------------------------------------------

static PythonPluginLoaderImpl* getImpl(PyObject* self)
{
    PyObject* capsule = PyObject_GetAttrString(self, "PythonPluginLoaderImpl");
    if (capsule == nullptr) {
        return nullptr;
    }
    auto* impl = static_cast<PythonPluginLoaderImpl*>(
        PyCapsule_GetPointer(capsule, "PythonPluginLoaderImpl"));
    Py_DECREF(capsule);
    return impl;
}

PyObject* PythonPluginLoaderImpl::setName(PyObject* self, PyObject* args)
{
    const char* name = nullptr;
    auto* impl = getImpl(self);
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(impl != nullptr,
        "setName cannot find the instance of PythonPluginLoaderImpl");
    if (!PyArg_ParseTuple(args, "s", &name)) {
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(
            Asserter::format("setName() called with wrong parameters."));
    }
    impl->setTestName(name);
    Py_RETURN_NONE;
}

PyObject* PythonPluginLoaderImpl::assertError(PyObject* self, PyObject* args)
{
    const char* message = nullptr;
    auto* impl = getImpl(self);
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(impl != nullptr,
        "assertError cannot find the instance of PythonPluginLoaderImpl");
    if (!PyArg_ParseTuple(args, "s", &message)) {
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(
            Asserter::format("assertError() called with wrong parameters."));
    }
    Asserter::error(TestMessage("asserts error with exception",
                                message, impl->getFileName(), 0));
    Py_RETURN_NONE;
}

PyObject* PythonPluginLoaderImpl::assertFail(PyObject* self, PyObject* args)
{
    const char* message = nullptr;
    auto* impl = getImpl(self);
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(impl != nullptr,
        "assertFail cannot find the instance of PythonPluginLoaderImpl");
    if (!PyArg_ParseTuple(args, "s", &message)) {
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(
            Asserter::format("assertFail() called with wrong parameters."));
    }
    Asserter::fail(TestMessage("asserts failure with exception",
                               message, impl->getFileName(), 0));
    Py_RETURN_NONE;
}

PyObject* PythonPluginLoaderImpl::testReport(PyObject* self, PyObject* args)
{
    const char* message = nullptr;
    auto* impl = getImpl(self);
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(impl != nullptr,
        "testReport cannot find the instance of PythonPluginLoaderImpl");
    if (!PyArg_ParseTuple(args, "s", &message)) {
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(
            Asserter::format("testReport() called with wrong parameters."));
    }
    Asserter::report(TestMessage("reports", message, impl->getFileName(), 0),
                     static_cast<TestCase*>(impl));
    Py_RETURN_NONE;
}

PyObject* PythonPluginLoaderImpl::testCheck(PyObject* self, PyObject* args)
{
    const char* message = nullptr;
    PyObject*   cond    = nullptr;
    auto* impl = getImpl(self);
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(impl != nullptr,
        "testCheck cannot find the instance of PythonPluginLoaderImpl");
    if (!PyArg_ParseTuple(args, "Os", &cond, &message)) {
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(
            Asserter::format("testCheck() called with wrong parameters."));
    }
    Asserter::testCheck(PyObject_IsTrue(cond) != 0,
                        TestMessage("checks", message, impl->getFileName(), 0),
                        static_cast<TestCase*>(impl));
    Py_RETURN_NONE;
}


// ---------------------------------------------------------------------------
// PythonPluginLoader (pimpl wrapper)
// ---------------------------------------------------------------------------

PythonPluginLoader::PythonPluginLoader() :
        implementation(nullptr)
{
}

PythonPluginLoader::~PythonPluginLoader()
{
    close();
}

void PythonPluginLoader::close()
{
    if (implementation != nullptr) {
        delete static_cast<PythonPluginLoaderImpl*>(implementation);
        implementation = nullptr;
    }
}

TestCase* PythonPluginLoader::open(const std::string filename)
{
    close();
    auto* impl = new PythonPluginLoaderImpl();
    implementation = impl;
    return impl->open(filename, venvPath);
}

void PythonPluginLoader::setVenv(const std::string& path)
{
    venvPath = path;
}

std::string PythonPluginLoader::getLastError()
{
    if (implementation != nullptr) {
        return static_cast<PythonPluginLoaderImpl*>(implementation)->getLastError();
    }
    return string("");
}
