#include "pybind11/pybind11.h"
namespace py = pybind11;

int add(int i, int j) {
    return i + j;
}

struct Pet
{
    std::string name;
    Pet(const std::string& name_) :name(name_){}
    void setName(const std::string& name_) { name = name_; }
    const std::string getName() const { return name; }
};

struct Dog: Pet 
{
    Dog(const std::string &name) : Pet(name) {}
    std::string bark() const { return "woof!"; }
};

// A type is considered polymorphic if it has at least one virtual function
struct PolymorphicPet
{
    virtual ~PolymorphicPet() = default;
};

struct PolymorphicDog : PolymorphicPet
{
    std::string bark() const { return "bark"; }
};

struct Widget
{
    int add(int x, int y) { return x + y; }
    int add(float x, float y) { return x + y; }
    int foo(int x, float y) { return x + y; }
    int foo(int x, float y) const { return x + y; }
};

struct EnumPet 
{
    enum Kind {
        Dog = 0,
        Cat
    };

    struct Attributes
    {
        int age = 0;
    };

    EnumPet(const std::string &name_, Kind type_):name(name_),type(type_){}

    std::string name;
    Kind type;
    Attributes attr;
};

PYBIND11_MODULE(example, m) {

    /* ==================
    == Binding Function ==
    ====================*/

    m.doc() = "TestPybind plugin";  // optional module docstring
    m.def("add", &add, "A function that adds two integers");
    // keyword argument
    m.def("add1", &add, "A function that adds two integers",
        py::arg("i"), py::arg("j"));
    // shorthand version
    using namespace py::literals;
    m.def("add2", &add, "A function that adds two integers",
        "i"_a, "j"_a);
    // default arguments
    m.def("add3", &add, "A function that adds two integers",
        "i"_a = 1, "j"_a = 2);
    // exporting variables
    m.attr("the_answer") = 42;
    py::object world = py::cast("World");
    m.attr("what") = world;

    /* ==================
    == Binding Struct ==
    ====================*/

    py::class_<Pet>(m, "Pet", py::dynamic_attr())  // dynamic attributes
        .def(py::init<const std::string&>())
        .def("setName", &Pet::setName)
        .def("getName", &Pet::getName)
        // binding lambda function
        .def("__repr__",
            [](const Pet& a) {
                return "<example.Pet named '" + a.name + "'>";
            })
        // expose name field
        .def_readwrite("name", &Pet::name)
        // provide a field-like interface within Python 
        .def_property("name", &Pet::getName, &Pet::setName);

    /* ======================================
    == Inheritance & Automatic Downcasting ==
    =======================================*/

    py::class_<Dog, Pet /* <- specify C++ parent type -> */>(m, "Dog")
        .def(py::init<const std::string&>())
        .def("bark", &Dog::bark);
    // Dog is a non-polymorphic type behind a base pointer, Python only sees a Pet.
    m.def("pet_store", []() {return std::unique_ptr<Pet>(new Dog("Molly")); });

    py::class_<PolymorphicPet>(m, "PolymorphicPet");
    py::class_<PolymorphicDog, PolymorphicPet>(m, "PolymorphicDog")
        .def(py::init<>())
        .def("bark", &PolymorphicDog::bark);
    m.def("pet_store2", []() {return std::unique_ptr<PolymorphicPet>(new PolymorphicDog); });
    
    /* =====================
    == Overloaded Methods ==
    ===================== */

    py::class_<Widget>(m, "Widget")
        // C++14 compatible compiler supports py::overloaded_cast,
        // and the return type is deduced
        .def("add", py::overload_cast<int, int>(&Widget::add), "Sum of two integers")
        .def("add", py::overload_cast<float, float>(&Widget::add), "Sum of two floats")
        // overloaded a cosnt version
        .def("foo_mutable", py::overload_cast<int, float>(&Widget::foo))
        .def("foo_const", py::overload_cast<int, float>(&Widget::foo, py::const_));
   
    /* ================================
    == Enumerations & Internal Types ==
    ================================ */

    py::class_<EnumPet> enumPet(m, "EnumPet");

    enumPet.def(py::init<const std::string&, EnumPet::Kind>())
        .def_readwrite("name", &EnumPet::name)
        .def_readwrite("type", &EnumPet::type)
        .def_readwrite("attr", &EnumPet::attr);
    
    py::enum_<EnumPet::Kind>(enumPet, "Kind")
        .value("Dog", EnumPet::Kind::Dog)
        .value("Cat", EnumPet::Kind::Cat)
        .export_values();

    py::class_<EnumPet::Attributes>(enumPet, "Attributes")
        .def(py::init<>())
        .def_readwrite("age", &EnumPet::Attributes::age);
}