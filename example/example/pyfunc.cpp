#include "pybind11/pybind11.h"
#include <iostream>
namespace py = pybind11;
#include <vector>
class MyContainer
{
public:
	MyContainer() { data.push_back(42); }

	int& get(int index) {  // Return reference to the element at index
		return data[index];  
	}

private:
	std::vector<int> data;
};

void print_dict(const py::dict& dict) {
	/* Easily interact with Python types */
	for (auto item : dict)
		std::cout << "key= " << std::string(py::str(item.first)) << ","
		<< "value= " << std::string(py::str(item.second)) << std::endl;
}

template <typename T>
void print_t(T t) {
	std::cout << t;
}

PYBIND11_MODULE(example2, m) {
	/* =========================
	== Keep Alive of C++ Object ==
	========================= */
	py::class_<MyContainer>(m, "MyContainer")
		.def(py::init<>())
		// C++ object stays alive as long as Python object exists
		.def("get", &MyContainer::get, py::keep_alive<0, 1>());
	m.def("print_dict", &print_dict);

	/* =========================
	== Keyword-only Arguments ==
	========================= */
	m.def("add_keyword", [](int a, int b) {return a + b; },
		py::arg("a"), py::kw_only(), py::arg("b"));  // b must be set with keyword

	/* ============================
	== Positional-only Arguments ==
	============================ */
	m.def("add_positional", [](int a, int b) {return a + b; },
		py::arg("a"), py::pos_only(), py::arg("b"));  // a must be set in positional

	/* ============================
	== No-converting Arguments ==
	============================ */
	m.def("floats_only", [](float f) {return 0.5 * f; }, py::arg("f").noconvert());

	/* ===================================
	== Allow-Prohibiting None Arguments ==
	=================================== */
	m.def("allow_None", [](py::object obj) -> std::string {
		if (obj) return "obj is not None";
		else return "obj isNone";
		}, py::arg("obj").none(true));  // Allow None

	/* ===================================
	== Binding Functions with Template Parameters ==
	=================================== */
	// must bind each instantiated function template separately
	m.def("print_t", &print_t<std::string>);
	m.def("print_t", &print_t<int>);
}
