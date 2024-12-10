#include<pybind11/pybind11.h>
#include<pybind11/operators.h>
namespace py = pybind11;

/* =========================================
== Overriding Virtual Functions in Python ==
========================================= */

class Animal
{
public:
	virtual ~Animal() {}
	virtual std::string go(int n_times) = 0;
	virtual std::string name() { return "unknown"; }
private:

};

//class PyAnimal : public Animal
//{
//public:
//	/* Inherit the constructors */
//	using Animal::Animal;
//
//	/* Trampoline (need one for each virtual function) */
//	std::string go(int n_times) override {
//		PYBIND11_OVERRIDE_PURE(
//			std::string,  // return type
//			Animal,		  // parent class
//			go,			  // function name in C++ (must match Python name)
//			n_times		  // argument
//		);
//	}
//};
class Dog : public Animal 
{
public:
	std::string go(int n_times) override {
		std::string results;
		for (int i = 0; i < n_times; i++)
			results += bark() + " "; // "woof!";
		return results;
	}
	virtual std::string bark() { return "woof!"; }
};

template<typename AnimalBase = Animal /* <-- default template class */>
class PyAnimal : public AnimalBase
{
public:
	using AnimalBase::AnimalBase;  // Inherit constructors
	std::string go(int n_times) override { PYBIND11_OVERRIDE_PURE(std::string, AnimalBase, go, n_times); }
	std::string name() override { PYBIND11_OVERRIDE_PURE(std::string, AnimalBase, name, ); }
 };

template <class DogBase = Dog> class PyDog : public PyAnimal<DogBase> {
public:
	using PyAnimal<DogBase>::PyAnimal; // Inherit constructors
	// Override PyAnimal's pure virtual go() with a non-pure one:
	std::string go(int n_times) override { PYBIND11_OVERRIDE(std::string, DogBase, go, n_times); }
	std::string bark() override { PYBIND11_OVERRIDE(std::string, DogBase, bark, ); }
};

std::string call_go(Animal* animal) {
	return animal->go(3);
}


/* ======================
== Cuntom Constructors ==
====================== */
class Example
{
private:
	Example(int) {}  // private constructor
public:
	// Factory function - returned by value:
	static Example create(int a) { return Example(a); }

	// These constructors are publicly callable:
	Example(double) {}
	Example(int, int) {}
	Example(std::string) {}
};

/* =======================
== Operator Overloading ==
======================= */
class MyVector
{
public:
	MyVector(float x, float y) : x(x), y(y) {}

	MyVector operator+ (const MyVector& v) const { return MyVector(x + v.x, y + v.y); }
	MyVector operator*(float value) const { return MyVector(x * value, y * value); }
	MyVector operator+=(const MyVector& v) { x += v.x; y += v.y; return *this; }
	MyVector operator*=(float value) { x *= value; y *= value; return *this; }

	friend MyVector operator*(float f, const MyVector& v) {
		return MyVector(f * v.x, f * v.y);
	}

	std::string toString() const {
		return "[" + std::to_string(x) + ", " + std::to_string(y) + "]";
	}
private:
	float x, y;
};

/* =====================
== Pickling Support ==
===================== */
class Pickleable
{
public:
	Pickleable(const std::string &value) : m_value(value) {}
	const std::string& value() const { return m_value; }

	void setExtra(int extra) { m_extra = extra; }
	int extra() const { return m_extra; }

private:
	std::string m_value;
	int m_extra = 0;
};

/* =====================================
== Binding Protected Member Functions ==
===================================== */
class A
{
public:
	virtual ~A() = default;

protected:
	int foo() const { return 42; }
	virtual int foo2() const { return 24; }
};

class Trampoline : public A {
public:
	int foo2() const override { PYBIND11_OVERRIDE(int, A, foo2, ); }
};

class Publicist : public A  // helper type for exposing protected functions
{
public:
	using A::foo;  // inherited with different access modifier
	using A::foo2;
};

/* ===============================
== Custom Automatic Downcasters ==
=============================== */
enum class PetKind { Cat, Dog, Zembra };
struct Pet  // Not polymorphic: has no virtual methods
{
	const PetKind kind;
	int age = 0;

protected:
	Pet(PetKind _kind) : kind(_kind) {}
};

struct Zembra :Pet
{
	Zembra() :Pet(PetKind::Zembra) {}
	std::string sound = "woof!";
	std::string bark() const { return sound; }
};

namespace pybind11 {
	template<>
	struct polymorphic_type_hook<Pet>
	{
		static const void* get(const Pet* src, const std::type_info*& type) {
			// src maybe nullptr
			if (src && src->kind == PetKind::Zembra) {
				type = &typeid(Zembra);
				return static_cast<const Zembra*>(src);
			}
			return src;
		}
	};
}
PYBIND11_MODULE(example3, m) {
	py::class_<Animal, PyAnimal<> /* <-- Traampoline */>(m, "Animal")
		.def(py::init<>())
		.def("go", &Animal::go);  // bindings should be made againist the actual class

	py::class_<Dog, Animal, PyDog<>>(m, "Dog")
		.def(py::init<>())
		.def("go", &Dog::go);
	
	m.def("call_go", &call_go);

	py::class_<Example>(m, "Example")
		// Factory function - returned by value:
		.def(py::init(&Example::create))
		// Bind a lambda function returning a pointer wrapped in a holder:
		.def(py::init([](std::string args) {
		return std::unique_ptr<Example>(new Example(args));
			}))
		// Return a raw pointer
		.def(py::init([](int a, int b) { return new Example(a, b); }))
		// Mix the abover with regular C++ constructor bindings as well:
		.def(py::init<double>());

	py::class_<MyVector>(m, "MyVector")
		.def(py::init<float, float>())
		.def(py::self + py::self)
		.def(py::self += py::self)
		.def(py::self *= float())
		.def(float() * py::self)
		.def(py::self * float())
		//.def(-py::self)
		.def("__repr__", &MyVector::toString);

	py::class_<Pickleable>(m, "Pickleable")
		.def(py::init<std::string>())
		.def("value", &Pickleable::value)
		.def("extra", &Pickleable::extra)
		.def("setExtra", &Pickleable::setExtra)
		// Pickling support in Python is enabled by defining
		// the __getstate__ and __setstate__ methods
		.def(py::pickle(  // A class with pickle support is automatically also (deep)copy compatible. 
			[](const Pickleable& p) {  // __getstate__
				/* Return a tuple that fully encodes the state of the object */
				return py::make_tuple(p.value(), p.extra());
			},
			[](py::tuple t) {  // __setstate__
				if (t.size() != 2)
					throw std::runtime_error("Invalid state!");

				/* Create a new C++ instance */
				Pickleable p(t[0].cast<std::string>());

				/* Assing any additional state */
				p.setExtra(t[1].cast<int>());

				return p;
			}
		));

	py::class_<A, Trampoline>(m, "A") // bind the primary class
		.def("foo", &Publicist::foo) // expose protected methods via the publicist
		.def("foo2", &Publicist::foo2);  // <-- `Publicist` hwre, not `Trampoline`
}