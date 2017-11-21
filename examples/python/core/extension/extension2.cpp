#include "extension2_reflection.h"
#include <bond/python/struct.h>

BOOST_PYTHON_MODULE(python_extension2)
{
    using namespace example::python;

    // Expose the AndroidPayload struct to Python.
    bond::python::struct_<AndroidPayload>()
        .def();
}

