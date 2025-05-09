#include <pricer/sample_library.hpp>
#include <pybind11/pybind11.h>

PYBIND11_MODULE(pypricer, m) {
    m.def("factorial", &factorial, "A function that computes factorials");
}
