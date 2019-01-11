## Some Keywords ###
* explicit
* = delete vs = default
* Smart Pointers
* Stack Object (automatic object) vs Heap Object
* noexcept
* Dependent types
* constexpr
* decltype and auto
* Function try blocks
* object slicing
* Template, typename vs class
* move vs forward (rvalues)
* 
## C++ Guide
http://www.stroustrup.com/C++11FAQ.html
https://channel9.msdn.com/Events/GoingNative/2013/An-Effective-Cpp11-14-Sampler

### References 
https://www.mytectra.com/interview-question/top-advanced-c-programming-interview-questions-2017/

### What is difference between C and C++ ?
* C++ is Multi-Paradigm: OOP + Procedural
* C++ supports function overloading
* C++ allows use of functions in structures
* C++ supports reference variables
* C++ directly supports exception handling

### What is a class?
Class defines a datatype, specifies the structure of data.
To use them you need to create objects out of the class. 

### What are the basics concepts of OOPS?
* Classes and Objects (Complex Data Types)
* Encapsulation (public, private, protected)
* Data abstraction (interfaces and abstract classes)
* Inheritance (class Square: public SymmetricShape {})

### What is STL (Standard Template Library)?
C++ standard library = C Standard Library + STL
STL provides:
* Containers: list, vector, set, map …
* Iterators
* Algorithms: search, sort, …

### What are the steps of a C++ executable?
Building a C++ program: 3 steps
* preprocessor (line starting with #)
* compiler
* linker

### What do you mean by C++ access specifiers ?
There are three access specifiers defined which are `public`, `private`, and `protected`.

### Is there a difference between class and struct?
The only difference between a class and struct are the access modifiers. Struct members are public by default; class members are private. It is good practice to use classes when you need an object that has methods and structs when you have a simple data object.

### Read Execise 
```cpp
const string& rhs   // const string ref
string&& rhs        // rvalue reference to string
```

### What is an Object/Instance?
Object is the instance of a class, which is concrete. We can create instance of classes. 

### What do you mean by translation unit in c++?
(The source code after preprocessing.)
We organize our C++ programs into different source files (.cpp, .cxx etc). When you consider a source file, at the preprocessing stage, some extra content may get added to the source code ( for example, the contents of header files included) and some content may get removed ( for example, the part of the code in the #ifdef of #ifndef block which resolve to false/0 based on the symbols defined). This effective content is called a translation unit. 

In other words, a translation unit consists of
* Contents of source file
* Plus contents of files included directly or indirectly
* Minus source code lines ignored by any conditional pre processing directives ( the lines ignored by #ifdef,#ifndef etc)

### What do you mean by internal linking and external linking in c++?
A symbol is said to be linked internally when it can be accessed only from with-in the scope of a single translation unit. By external linking a symbol can be accessed from other translation units as well. This linkage can be controlled by using static and extern keywords.

### What do you mean by storage classes?

Storage class are used to specify the visibility/scope and life time of symbols(functions and variables). That means, storage classes specify where all a variable or function can be accessed and till what time those variables will be available during the execution of program.
* auto (scope: local, default for locals)
* static (scope: local)
* register (scope: local)
* extern (scope: global)
 
### What do you mean by persistent and non persistent objects?

* Persistent objects are the ones which we can be serialized and written to disk, or any other stream. So before stopping your application, you can serialize the object and on restart you can deserialize it. [ Drawing applications usually use serializations.]
* Objects that can not be serialized are called non persistent objects. [ Usually database objects are not serialized because connection and session will not be existing when you restart the application. ]

### What are virtual functions and what is its use?

Virtual functions are member functions of class which is declared using keyword ‘virtual’. When a base class type reference is initialized using object of sub class type and an overridden method which is declared as virtual is invoked using the base reference, the method in child class object will get invoked.

### What is virtual destructors? Why they are used?
[polymorphism related]
Virtual destructors are used for the same purpose as virtual functions. When you remove an object of subclass, which is referenced by a parent class pointer, only destructor of base class will get executed. But if the destructor is defined using virtual keyword, both the destructors [ of parent and sub class ] will get invoked.

### Explain the volatile and mutable keywords

The volatile keyword informs the compiler that a variable will be used by multiple threads. Variables that are declared as volatile will not be cached by the compiler to ensure the most up-to-date value is held.

The mutable keyword can be used for class member variables. Mutable variables are allowed to change from within const member functions of the class.

### What are the Rule Of Five? ###
By default, a class has 5 operations:
* copy assignment
* copy constructor
* move assignment
* move constructor
* destructor

### In how many ways we can initialize an int variable in C++?
* Traditional C++ initilization
    ```cpp
    int i = 10
    ```
* Using C++ constructor notation
    ```cpp
    int i(10);
    ```
### What is the difference of decltype and auto?
* `Auto` lets you declare a variable with particular type whereas, (yeni bir variable deklare ediliyor.)
* `decltype` lets you extract the type from the variable so decltype is sort of an operator that evaluates the type of passed expression. (var olan bir degiskenin tipini aliyor.)
* Examples:
    ```cpp
    decltype(fun1()) x; // Gets the return variable type
    
    auto y = 3.37; // Declares y as double
    auto ptr = &x; // Declares ptr as int-pointer 
    
    // A generic function which finds minimum of two values 
    // return type is type of variable which is minimum 
    template <class A, class B> 
    auto findMin(A a, B b) -> decltype(a < b ? a : b) 
    { 
        return (a < b) ? a : b; 
    } 
    ```

### What are smart pointers? ###
* std::unique_ptr, 
* std::shared_ptr, 
* std::weak_ptr

### What are the main differences between pointer and reference?
* Reference can only be initialized, not reassigned. It is just an `alias`. Has no own address in memory, it shares the same adress with the object it refers to.
* Quick tricky example:
    ```cpp
    int main()
    {
      int a{ 9 };
      int b{ 3 };
     
      int &c{ b }; // c is an alias for b
     
      c = a; // We're not reassigning the reference, we're assigning a new value to b.
     
      // c is still an alias for b, but b now has the value 9
     
      return 0;
    }
    ```
* Object slicing through reference:
    ```cpp
    int main()
    {
        Derived d1(5);
        Derived d2(6);
        Base &b = d2;
        
        b = d1; // this line is problematic
        // only the Base portion of d1 is copied into d2
        return 0;
    }
    ```

### What is `stack object` and `heap object` and their life times? ###
A `stack object` (`automatic object`) is created at the point of its definition, and lives until the end of its scope. Basically, until the closing curly brace of the block it is declared in.
A `heap object` is created with the new operator and lives until delete is called on it.

### What is the usage of `=delete`?
We can eliminate an undesired conversion like this:
```c
struct Z {
	// ...
	Z(long long);     // can initialize with an long long
	Z(long) = delete; // but not anything less
};
```
### What are the usage of `std::move` and `std::forward`?
https://tests4geeks.com/cpp-online-test

### What is `if-it-has-a-name rule` of rvalues?
Things that are declared as rvalue reference can be lvalues or rvalues. The distinguishing criterion is: if it has a name, then it is an lvalue. Otherwise, it is an rvalue.
```cpp
Derived(Derived&& rhs) 
  : Base(rhs) // wrong: rhs is an lvalue
{
  // Derived-specific stuff
}
Derived(Derived&& rhs) 
  : Base(std::move(rhs)) // good, calls Base(Base&& rhs)
{
  // Derived-specific stuff
}
```
std::move "turns its argument into an rvalue even if it isn't," and it achieves that by "hiding the name." It passes its argument right through by reference, doing nothing with it at all, and its result type is rvalue reference.

### Exceptions
---
* Any throw in constructer will block the constructer to move, and the destructor will not invoked. So that, constructor should handle memory management itself.
* Since Derived class has a relation is-a with the Base class, the catch for Derived should be positioned first. 
* ```#include <stdexcept> throw std::runtime_error("Bad things happened");``` might be good point to use exceptions. 
* When rethrowing the same exception, use the throw keyword by itself from a catch block; ```throw; // note: We're now rethrowing the object here```
* ```Function try blocks``` can catch the exception in initializer list of a constructor. If you do not explicitly throw a new exception in catch block, the exception will be implicitly rethrown up the stack.
* Utilize smart pointers to deallocate dynamically allocated memory:
    ```cpp
    #include <memory>; // for std::unique_ptr
    ... 
    try{
        Person *john = new Person("John", 18, PERSON_MALE);
        unique_ptr<Person> upJohn(john); // upJohn now owns john
     
        ProcessPerson(john);
        // when upJohn goes out of scope, it will delete john
    }
    catch (PersonException &exception) {
        cerr << "Failed to process person: " << exception.what() << '\n';
    }
    ```



### What is implicit conversion/coercion in c++?
```cpp
short a = 2000 + 20;
```
In the above example, variable a will get automatically promoted from short to int. This is called implicit conversion/coercion in c++.


### What’s the difference between a numeric promotion and a numeric conversion?
* Numeric promotion is converting a smaller (typically integral or floating point) type to a larger similar (integral or floating point) type. Promotions generally involve extending the binary representation of a number (e.g. for integers, adding leading 0s)

* Numeric conversion is converting a larger type to a smaller type, or between different types. Conversions require converting the underlying binary representation to a different format.

### How does compiler evaluate arithmetic expressions?

When evaluating expressions, the compiler breaks each expression down into individual subexpressions. The arithmetic operators require their operands to be of the same type. To ensure this, the compiler uses the following rules:

If an operand is an integer that is narrower than an int, it undergoes integral promotion (as described above) to int or unsigned int.If the operands still do not match, then the compiler finds the highest priority operand and implicitly converts the other operand to match.

The priority of operands is as follows:

* long double (highest)
* double
* float
* unsigned long long
* long long
* unsigned long
* long
* unsigned int
* int (lowest)


### How many types of casts in c++?
There are 5 different types of casts: 
* C-style casts, 
* static casts, 
* const casts, 
* dynamic casts, and 
* reinterpret casts.

### Downcasting vs upcasting
* C++ will implicitly let you convert a Derived pointer into a Base pointer (in fact, getObject() does just that). This process is sometimes called upcasting. 
* Converting base-class pointers into derived-class pointers. This process is called downcasting


### dynamic_cast vs static_cast
Dynamic_cast should be used when downcasting.
Otherwise static_cast will be the option. 

### Downcasting vs virtual functions
In general, using a virtual function should be preferred over downcasting. However, there are times when downcasting is the better choice:

* When you can not modify the base class to add a virtual function (e.g. because the base class is part of the standard library)
* When you need access to something that is derived-class specific (e.g. an access function that only exists in the derived class)
* When adding a virtual function to your base class doesn’t make sense (e.g. there is no appropriate value for the base class to return). Using a pure virtual function may be an option here if you don’t need to instantiate the base class.

### Some Keywords
#### Data Structures
Vectors
Vectors and Memory
Two-Dimensional Vectors
Lists
Maps
Custom Objects as Map Values
Custom Objects as Map Keys
Multimaps
Sets
Stacks and Queues
Sorting Vectors, Deque and Friend
STL Complex Data Types

#### Passing Functions to Functions
Function Pointers
Using Function Pointers
Object Slicing and Polymorphism
Abstract Classes and Pure Virtual Functions
Functors

#### Template Classes and Functions
Template Classes
Template Functions
Template Functions and Type Inference

#### C++ 11's Amazing New Features
Decltype, Typeid and Name Mangling
The Auto Keyword
Range-Based Loops
Nested Template Classes
A Ring Buffer Class
Making Classes Iterable
Initialization in C++ 98
Initialization in C++ 11
Initializer Lists
Object Initialization, Default and Delete
Introducing Lambda Expressions
Lambda Parameters and Return Types
Lambda Capture Expressions
Capturing this With Lambdas
The Standard Function Type
Mutable Lambdas
Delegating Constructors
Elision and Optimization.avi
Constructors and Memory
Rvalues and LValues
LValue References
Rvalue References
Move Constructors
Move Assignment Operators
Static Casts
Dynamic Cast
Reinterpret Cast
Perfect Forwarding
Bind
Unique Pointers
Shared Pointers

## Practical Questions
### What is the output of the following code?
```cpp
class Base {
    virtual void method() {
        std::cout << “from Base” << std::endl;
    }
public:
    virtual ~Base() {
        method();
    }
    void baseMethod() {
        method();
    }
};

class A : public Base{
    void method() {
        std::cout << “from A” << std::endl;
    }

public:
    ~A() {
        method();
    }
};

int main(void) {
    Base* base = new A;
    base->baseMethod();
    delete base;
    return 0;
}
```
The above will output:
```
from A
from A
from Base
```
The important thing to note here is the order of destruction of classes and how Base’s method reverts back to its own implementation once A has been destroyed.
