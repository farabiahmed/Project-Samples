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
* lambdas
* RAII (Resource Acquisition Is Initialization)
* SBRM (scope-bound resource management) same as RAII.
* Type Safety
* RTTI (Run Time Type Information)
* override keyword
* explicit vs implicit casting
* upcasting and downcasting
* storage specifiers (static, extern, )
* type qualifiers (const, volatile, )
* explicit vs implicit constructors
* stack unwinding
* reference counting (shared_ptr)

## References 
https://www.mytectra.com/interview-question/top-advanced-c-programming-interview-questions-2017/
https://hackernoon.com/how-to-improve-your-c-skills-from-awesome-projects-251b300ed5a1

## FAQ
### Why C++ is faster than the Javascript?
* C++ is a fully compiled language - so there is no runtime parsing of source code and no “just in time” compilation cost.
* C++ does not use “garbage collection” - which basically requires JavaScript to periodically stop - and look at EVERY reference to an allocated memory block to see if it’s no longer needed.
* The reason C++ is faster is because it’s very natural to allocate memory for your data structures in advance, and C++ data structures can often be packed much more efficiently into contiguous blocks of data, which make it much easier for the processor to optimize for cache. 
* In JavaScript, because of the dynamic nature of the language, any variable name could point to any data type. This makes it much more difficult to allocate in advance, and much more difficult to pack data in arrays (it’s just a bunch of pointers to objects on the heap).
* In short, C++ tries to force the programmer to organize data in a way that is efficient for the machine. JavaScript is focused on making data organization easy for the programmer and can’t give the machine any guarantees about what it will need before the code runs. 

### What is the difference between a `strongly typed` language and a `weakly/loosely typed` language?
* In a weakly typed language, the type of a value depends on how it is used. For example if I can pass a string to the addition operator and it will AutoMagically be interpreted as a number or cause an error if the contents of the string cannot be translated into a number.
* In a strongly typed language, a value has a type and that type cannot change. What you can do to a value depends on the type of the value. The advantage of a strongly typed language is that you are forced to make the behaviour of your program explicit. 

### What us `static/dynamic typing`?
* Static typing is where the type is bound to the variable. Types are checked at compile time.
* Dynamic typing is where the type is bound to the value. Types are checked at run time.

### What is `bytecode`?
Bytecode is program code that has been compiled from source code into low-level code designed for a software interpreter. It may be executed by a virtual machine (such as a JVM) or further compiled into machine code, which is recognized by the processor.
* *Remember*: C++ must compile into object code, then to machine language. Because of this, it's possible for Java to compile only a single class for minor changes, while C++ object files must be re-linked with other object files to machine code executable (or DLLs). This may make the process take a bit longer.

### What is the difference between `JVM` and `JIT`?
* Java source code is compiled into class files, which contains byte code. These byte codes are then executed by JVM. 
* Since execution of byte code is slower than execution of machine language code, because JVM first needs to translate byte code into machine language code. 
* JIT helps JVM here by compiling currently executing byte code into machine language. JIT also offers caching of compiled code which result in improved performance of JVM.
* Java compiles to JVM bytecode at compile-time, but, like JavaScript, it is JIT compiled at runtime, but it is nonetheless generally faster. 
 
### C++ Guide
http://www.stroustrup.com/C++11FAQ.html
https://channel9.msdn.com/Events/GoingNative/2013/An-Effective-Cpp11-14-Sampler

## Concurrency
https://baptiste-wicht.com/posts/2012/03/cp11-concurrency-tutorial-part-2-protect-shared-data.html
https://thispointer.com/c-11-multithreading-part-1-three-different-ways-to-create-threads/
http://www.cplusplus.com/reference/condition_variable/condition_variable/
https://en.cppreference.com/w/cpp/thread/shared_lock
`TODO Elaborate`

### What are the disadvantages of `wait loop` or `continous polling`?
Though the sleep avoids the high CPU consumption of a direct busy wait, there are still some obvious downsides to this formulation. Let`s take the example given below for a thread that is trying tou exploit a thread-safe queue:
```c
   while(some_queue.empty())
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(50));
    }
```
 * Firstly, the thread has to wake every 50ms or so (or whatever the sleep period is) in order to lock the mutex, check the queue, and unlock the mutex, forcing a `context switch`. 
 * Secondly, the sleep period imposes a `limit on how fast the thread can respond` to data being added to the queue — if the data is added just before the call to sleep, the thread will wait at least 50ms before checking for data. On average, the thread will only respond to data after about half the sleep time (25ms here).
 * The alternative is using `condition variable`.

### Waiting with a `Condition Variable`
https://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
* As an alternative to continuously polling the state of the queue, the sleep in the wait loop can be replaced with a condition variable wait. 
* If the condition variable is notified in push when data is added to an empty queue, then the waiting thread will wake. 
* This requires access to the mutex used to protect the queue, so needs to be implemented as a member function of concurrent_queue:
    ``` c
    template<typename Data>
    class concurrent_queue
    {
    private:
        boost::condition_variable the_condition_variable;
    public:
        void wait_for_data()
        {
            boost::mutex::scoped_lock lock(the_mutex);
            while(the_queue.empty())
            {
                the_condition_variable.wait(lock);
            }
        }
        void push(Data const& data)
        {
            boost::mutex::scoped_lock lock(the_mutex);
            bool const was_empty=the_queue.empty();
            the_queue.push(data);
            
            /* Optimal: See the third important thing given below*/
            lock.unlock(); // unlock the mutex
            
            /* SubOptimal: having no unlock here */
            // lock.unlock(); // unlock the mutex
            
            if(was_empty)
            {
                the_condition_variable.notify_one();
            }
        }
        // rest as before
    };
    ```
There are three important things to note here. 
* Firstly, the lock variable is passed as a parameter to wait — this allows the condition variable implementation to atomically unlock the mutex and add the thread to the wait queue, so that another thread can update the protected data whilst the first thread waits.
* Secondly, the condition variable wait is still inside a while loop — condition variables can be subject to spurious wake-ups, so it is important to check the actual condition being waited for when the call to wait returns.
* Thirdly, the call to notify_one comes after the data is pushed on the internal queue. This avoids the waiting thread being notified if the call to the_queue.push throws an exception. As written, the call to notify_one is still within the protected region, which is potentially sub-optimal: the waiting thread might wake up immediately it is notified, and before the mutex is unlocked, in which case it will have to block when the mutex is reacquired on the exit from wait. 

### Best Practices
* Make your classes thread-safe if they are used by multiple threads.
   *  mutex.lock() and mutex.unlock()
* Use RAII in thread safe classes to be resistable to exceptions. 
   * std::lock_guard<std::mutex> guard(mutex);

## Paradigms
C++ is not just an object-oriented language. As Bjarne Stroustrup points out, “C++ is a multi-paradigmed language.” It supports many different styles of programs or paradigms, and object-oriented programming is only one of these. Some of the others are procedural programming and generic programming.

Paradigms supported by C++:
* Object Oriented Programming (OOP)
* Procedural Programming
* Generic Programming
* Functional Programming

## Object Oriented Programming (OOP) Basics
### Inheritance
In object-oriented programming (OOP), inheritance is a way to establish Is-a relationship between objects. It is often confused as a way to reuse the existing code which is not a good practice because inheritance for implementation reuse leads to Tight Coupling. Re-usability of code is achieved through composition (Composition over inheritance). 

### Polymorhism

### Encapsulation

### Coupling and Cohesion

#### Single Responsibility

## Procedural Programming
Procedural programming (PP), also known as inline programming takes a top-down approach. It is about writing a list of instructions to tell the computer what to do step by step. It relies on procedures or routines.

* Global functions
* Static global functions

## Generic Programming
C++ provides unique abilities to express the ideas of Generic Programming through templates. Templates provide a form of parametric polymorphism that allows the expression of generic algorithms and data structures. The instantiation mechanism of C++ templates ensures that when a generic algorithm or data structure is used, a fully-optimized and specialized version will be created and tailored for that particular use, allowing generic algorithms to be as efficient as their non-generic counterparts.

Templates popularized the notion of generics. Templates allow code to be written without consideration of the type with which it will eventually be used. Templates are defined in the Standard Template Library (STL), where generic programming was introduced into C++.

## Functional Programming
Functional programming (FP) is about passing data from function to function to function to get a result. In FP, functions are treated as data, meaning you can use them as parameters, return them, build functions from other functions, and build custom functions. Functions in FP have to be pure functions, they should avoid shared state, and side effects and data should be immutable.
### Pure Functions
A pure function is a function that given the same type of input will always return the same output, it is not dependent on a local or global state. 
### Shared State
A shared state is a state that is shared between more than one function or more than one data-structure. So with shared state, in order to understand the effects of a function, you need to know all the details of every shared variable. It adds a lot of complexity and permits less modularity.

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

#### Advantages of containers
* They eliminate an entire set of boilerplate code that you re-implement on every project. 
* The container library also benefits from having a standardized interface for member functions. 
* These standardized interfaces reduce your memory burden and allow containers to be used with `STL algorithms`.

#### What are the container concepts?
https://embeddedartistry.com/blog/2017/8/2/an-overview-of-c-stl-containers
* `SequenceContainer` are used for data structures that store objects of the same type in a linear manner.
    * std::array represents a static contiguous array
    * std::vector represents a dynamic contiguous array
    * std::deque represents a double-ended queue, where elements can be added to the front or back of the queue
    * std::forward_list represents a singly-linked list
    * std::list represents a doubly-linked list
    * While std::string is not included in most container lists, it does in fact meet the requirements of a SequenceContainer.
* `Container Adapters` They are not full container classes on their own, but wrappers around other container types (such as a vector, deque, or list).  
    * std::stack provides an LIFO data structure
    * std::queue provides a FIFO data structure
    * std::priority_queue provides a priority queue, which allows for constant-time lookup of the largest element (by default)
* `AssociativeContainer` provide sorted data structures that provide a fast lookup (O(log n) time) using keys. Typically implemented using red-black trees.
    * std::set is a collection of unique keys, sorted by keys, Keys are unique
    * std::map is a collection of key-value pairs, sorted by keys, Keys are unique
    * std::multiset is a collection of keys, sorted by keys, Multiple entries for the same key are permitted
    * std::multimap is a collection of key-value pairs, sorted by keys, Multiple entries for the same key are permitted
* `UnorderedAssociativeContainer` provide unsorted data structures that can be accessed using a hash. Access times are O(n) in the worst-case, but much faster than linear time for most operations.
    * std::unordered_set is a collection of keys, hashed by keys, Keys are unique
    * std::unordered_map is a collection of key-value pairs, hashed by keys, Keys are unique
    * std::unordered_multiset is a collection of keys, hashed by keys, Multiple entires for the same key are permitted
    * std::unordered_multimap is a collection of key-value pairs, hashed by keys, Multiple entires for the same key are permitted

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

### What is the `override` keyword?
The override keyword serves two purposes: 
* It shows the reader of the code that "this is a virtual method, that is overriding a virtual method of the base class." 
* The compiler also knows that it's an override, so it can "check" that you are not altering/adding new methods that you think are overrides.

### Explain the volatile and mutable keywords

The volatile keyword informs the compiler that a variable will be used by multiple threads. Variables that are declared as volatile will not be cached by the compiler to ensure the most up-to-date value is held.

The mutable keyword can be used for class member variables. Mutable variables are allowed to change from within const member functions of the class.

### What is `RAII`?
(Resource Acquisition Is Initialization)
Specifically, RAII ensures that the acquisition of a resource occurs at the initialization of the object, and the release of the resources occurs when the object is no longer needed.
`std::unique_ptr[]` and `std::shared_ptr{}` leverage this exact design pattern

### What is `RTTI`?
RTTI refers to the ability of the system to report on the dynamic type of an object and to provide information about that type at runtime (as opposed to at compile time). However, RTTI becomes controversial within the C++ community. Many C++ developers chose to not use this mechanism.

### What is `Type Safety`?
Standard C is not a type-safe language. Type safety refers to protections put in place to prevent one type from being confused with another type. 
```cpp
/* Example: C */
int *p = malloc(sizeof(int)); //(implicit type conversion)
// Example: C++
auto p = new int;
```
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
    
    // auto is useless for functions (since you have to write the return type either way), right? NO! see template based example below!
    auto f() -> int { return 0; } // a legal C++ function only in C++11
    
    // Suppose you have template<class T> class Vector, 
    // and want to implement Vector-skalar multiplcation multiply(Vector<T> const& v, X x). 
    // What would be the return type?
    template<class T, class X> 
    auto multiply(Vector<T> const& v, X x) -> Vector<decltype(v[0] * x)> {...}
    ```
### What is `constexpr`?
With C++11, constexpr was added as a statement to the compiler that a variable, function, and so on, can be evaluated at compile time and optimized, reducing the complexity of the code at runtime and improving performance overall.

C++17 adds a `constexpr if` statement, which tells the compiler to specifically optimize the branch at compile time.
#### What are the differences between `define` and `constexpr` and `const`?
`Define` vs `constexpr`
- scope
- memory location
- type

`constexpr` vs `const`
The principal difference between const and constexpr is the time when their initialization values are known (evaluated). While the values of const variables can be evaluated at both compile time and runtime, constexpr are always evaluated at compile time.

### What are smart pointers? ###
It is a wrapper around raw pointers. You don`t need to free memory manually. Smart pointers does so through its destructor. They will be deleted on out of scope.
* std::unique_ptr, 
* std::shared_ptr, 
* std::weak_ptr
They are resided in <memory>.

#### std::unique_ptr
* You cannot copy std::unique_ptr.
* It will delete the memory upon destruction.

#### std::shared_ptr
* References are counted. Each increases reference by 1.
* More shared_ptr can points to same memory.
* Memory will be destroyed after all the shared pointers are destroyed.

#### std::weak_ptr
* It shares the memory with other pointers but doesnt increases reference by 1.

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
### RValues
---
### How to catch rValue in a function?
```cpp
void overloaded( int const &arg ) { std::cout << "by lvalue\n"; }
void overloaded( int && arg ) { std::cout << "by rvalue\n"; }
```
#### What are the usage of `std::move` and `std::forward`?
https://tests4geeks.com/cpp-online-test
`move`: (unconditioned, l to r, r to r) : takes an object and allows you to treat it as a temporary (an rvalue).
`forward`: (conditioned, l to l, r to r) This allows rvalue arguments to be passed on as rvalues, and lvalues to be passed on as lvalues. 

### Then, what is the usage of std::forward?
If an rvalue has a name, it is normally considered as lvalue if `forward` is not used, in other words if it passed via simple passing. 

Example:
```cpp
void overloaded( int const &arg ) { std::cout << "by lvalue\n"; }
void overloaded( int && arg ) { std::cout << "by rvalue\n"; }
 
template< typename t >
/* "t &&" with "t" being template param is special, and  adjusts "t" to be
   (for example) "int &" or non-ref "int" so std::forward knows what to do. */
void forwarding( t && arg ) {
    std::cout << "via std::forward: ";
    overloaded( std::forward< t >( arg ) );
    std::cout << "via std::move: ";
    overloaded( std::move( arg ) ); // conceptually this would invalidate arg
    std::cout << "by simple passing: ";
    overloaded( arg );
}
 
int main() {
    std::cout << "initial caller passes rvalue:\n";
    forwarding( 5 );
    std::cout << "initial caller passes lvalue:\n";
    int x = 5;
    forwarding( x );
}
```
Expected output:
initial caller passes rvalue: 
via std::forward: by rvalue **(with forward it is considered as rvalue)**
via std::move: by rvalue
by simple passing: by lvalue **(without forward it is considered as lvalue)**
initial caller passes lvalue:
via std::forward: by lvalue
via std::move: by rvalue
by simple passing: by lvalue

#### What is `if-it-has-a-name rule` of rvalues?
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

#### State which of the following lettered statements will not compile:
```cpp
int main() {
	int x;
	
	// l-value references
	int &ref1 = x; // A
	int &ref2 = 5; // B
	
	const int &ref3 = x; // C
	const int &ref4 = 5; // D

	// r-value references
	int &&ref5 = x; // E
	int &&ref6 = 5; // F

	const int &&ref7 = x; // G
	const int &&ref8 = 5; // H
	
	return 0;
}
```
Solution:
B, E, and G won’t compile.
B: you cannot change a literal. 

### Lambdas
---
C++ 11 introduced lambda expression to allow us write an inline function which can be used for short snippets of code that are not going to be reuse and not worth naming. In its simplest form lambda expression can be defined as follows:

        [ capture clause ] (parameters) -> return-type  
        {   
           definition of method   
        } 

#### How to get input parameters into lambda (capture clause)?
      [&] : capture all external variable by reference
      [=] : capture all external variable by value
      [a, &b] : capture a by value and b by reference
      
#### Example:
```cpp    
    int N = 5;
    // below snippet find first number greater than N 
    // [N]  denotes,   can access only N by value 
    vector<int>:: iterator p = find_if(v1.begin(), v1.end(), [N](int i) 
    { 
        return i > N; 
    }); 
  
    cout << "First number greater than 5 is : " << *p << endl; 
    //     We can also access function by storing this into variable 
    auto square = [](int i) 
    { 
        return i * i; 
    }; 
    cout << "Square of 5 is : " << square(5) << endl; 
```
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

Other properties:
* C++ type casts are safer than c-style casts because they will return null or throw exception on error. 
* C-style casts are unsafe.

#### const_cast
* const_cast can be used to pass const data to a function that doesn’t receive const.
* It is undefined behavior to modify a value which is initially declared as const.
* const_cast is considered safer than simple type casting. It’safer in the sense that the casting won’t happen if the type of cast is not same as original object.
* const_cast is considered safer than simple type casting. It’safer in the sense that the casting won’t happen if the type of cast is not same as original object. 
    ```cpp
    int a1 = 40; 
    const int* b1 = &a1; 
    char* c1 = const_cast <char *> (b1); // compiler error 
    *c1 = 'A'; 
    ```
* const_cast can also be used to cast away volatile attribute.

#### dynamic_cast vs static_cast
Dynamic_cast should be used when downcasting.
Otherwise static_cast will be the option. 

If dynamic_cast is used to convert to a reference type and the conversion is not possible, an exception of type `bad_cast` is thrown instead.

http://www.cplusplus.com/doc/tutorial/typecasting

#### dynamic_cast
* for both upcasts  and downcast
* Dynamic_cast should be used when downcasting if -and only if- the pointed object is a valid complete object of the target type. It returns a `null pointer` to indicate the failure.
* dynamic_cast on fail will cast to NULL if you are casting pointers, and throw an exception otherwise.
** it will return null pointer, for pointer types, for error.
** it will throw bad_cast exception, for references, for error.
* It will return valid pointer on succces.


#### static_cast
* static_cast will give you a compilation error if it can't make the cast. 
* for both upcasts  and downcast
* No checks are performed during runtime to guarantee that the object being converted is in fact a full object of the destination type. 
* Therefore, it is up to the programmer to ensure that the conversion is safe. 
* On the other side, it does not incur the overhead of the type-safety checks of dynamic_cast.
* This would be valid code, although b would point to an incomplete object of the class and could lead to runtime errors if dereferenced.
    ```c
    class Base {};
    class Derived: public Base {};
    Base * a = new Base;
    Derived * b = static_cast<Derived*>(a);
    ```
#### reinterpret_cast 
* it converts any pointer type to any other pointer type, even of unrelated classes. 
* The operation result is a simple binary copy of the value from one pointer to the other.
* Dereferencing resulted pointer `b` is not safe.
    ```c
    class A { /* ... */ };
    class B { /* ... */ };
    A * a = new A;
    B * b = reinterpret_cast<B*>(a);
    ```

### explicit vs implicit casting
Implicit casting:
`
int i = 5;
double d = i;
`
### explicit vs implicit constructors
#### Implicit Constructor:

Class Declaration:

```
class A
{
public:
    A();
    A(int);
    A(const char*, int = 0);
};
```

```
A c = 1;
A d = "Venditti";
```

#### Explicit Constructor: 

You can only assign values that match the values of the class type.

Class Declaration:
```
class A
{
public:
    explicit A();
    explicit A(int);
    explicit A(const char*, int = 0);
};
```

```
  A a1;
  A a2 = A(1);
  A a3(1);
  A a4 = A("Venditti");
  A* p = new A(1);
  A a5 = (A)1;
  A a6 = static_cast<A>(1);
```

### Downcasting vs upcasting
upcasting: converting from pointer-to-derived to pointer-to-base
downcasting: converting from pointer-to-base to pointer-to-derived
* C++ will implicitly let you convert a Derived pointer into a Base pointer (in fact, getObject() does just that). This process is sometimes called upcasting. 
* Converting base-class pointers into derived-class pointers. This process is called downcasting

### Downcasting vs virtual functions
In general, using a virtual function should be preferred over downcasting. However, there are times when downcasting is the better choice:

* When you can not modify the base class to add a virtual function (e.g. because the base class is part of the standard library)
* When you need access to something that is derived-class specific (e.g. an access function that only exists in the derived class)
* When adding a virtual function to your base class doesn’t make sense (e.g. there is no appropriate value for the base class to return). Using a pure virtual function may be an option here if you don’t need to instantiate the base class.

### What is `const volatile`?
const and volatile sound like they refer to the same idea on a variable, but they don't. A `const` variable can't be changed by the current code. A `volatile` variable may be changed by some outside entity outside the current code. It's possible to have a const volatile variable - especially something like a memory mapped register - that gets changed by the computer at a time your program can't predict, but that your code is not allowed to change directly.

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

## C++17
---
---
### New features
---
References: 
https://www.codingame.com/playgrounds/2205/7-features-of-c17-that-will-simplify-your-code/introduction

Structured bindings/Decomposition declarations
Init-statement for if/switch
Inline variables
constexpr if
Fold expressions
Template argument deduction for class templates
Declaring non-type template parameters with auto
Namespaces  (namespace X::Y::Z{})

#### Structured bindings/Decomposition declarations
``` cpp
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <iterator>

#include <tuple>

// { autofold 
struct S {
    int n;
    std::string s;
    float d;
    bool operator<(const S& rhs) const
    {
        // compares n to rhs.n,
        // then s to rhs.s,
        // then d to rhs.d
        return std::tie(n, s, d) < std::tie(rhs.n, rhs.s, rhs.d);
    }
};
// }

int main()
{
    std::set<S> mySet;
 
    // pre C++17:
    {
	    S value{42, "Test", 3.14};
	    std::set<S>::iterator iter;
	    bool inserted;
 
	    // unpacks the return val of insert into iter and inserted
	    std::tie(iter, inserted) = mySet.insert(value);

	    if (inserted)
		    std::cout << "Value was inserted\n";
    }
	
	// with C++17:
    {
        S value{100, "abc", 100.0};
        const auto [iter, inserted] = mySet.insert(value);
		
        if (inserted)
		    std::cout << "Value(" << iter->n << ", " << iter->s << ", ...) was inserted" << "\n";
    }
        
}
```
Some of the other usages:
```cpp
auto& [ refA, refB, refC, refD ] = myTuple;

std::map myMap;    
for (const auto & [k,v] : myMap) 
{  
    // k - key
    // v - value
} 
```

### Init-statement for if/switch (if initializer)
```cpp
if (const auto it = myString.find("Hello"); it != std::string::npos)
    std::cout << it << " Hello\n";

if (const auto it = myString.find("World"); it != std::string::npos)
    std::cout << it << " World\n";
    
if (const auto it = myString.find("World"); it != std::string::npos)
    std::cout << it << " World\n";
else
    std::cout << it << " not found!!\n";
    
// better together: structured bindings + if initializer
if (auto [iter, succeeded] = mymap.insert(value); succeeded) {
    use(iter);  // ok
    // ...
} // iter and succeeded are destroyed here
```

### constexpr if
C++17 adds a constexpr if statement, which tells the compiler to specifically optimize the branch at compile time. If the compiler cannot optimize the if statement, an explicit compile-time error will occur, telling the user that optimization could not be done, providing the user with an opportunity to fix the issue (instead of assuming the optimization was taking place when in fact it might not be).

```cpp
#include <iostream>
int main(void)
{
    if constexpr (constexpr const auto i = 42; i > 0) {
        std::cout << "Hello World\n";
    }
}
```
We have a more complicated if statement that leverages both a compile-time constexpr optimization as well as an if statement
initializer.

# Managing DLLS
## Windows Way
`__declspec(dllexport)` tells the linker that you want this object to be made available for other DLL's to import. It is used when creating a DLL that others can link to.
`__declspec(dllimport)` imports the implementation from a DLL so your application can use it.

There are two types of linking: implicit linking and explicit linking.

## What is explicit linking?
With explicit linking, applications must make a function call to explicitly load the DLL at run time. To explicitly link to a DLL, an application must:
* Call `LoadLibrary` (or a similar function) to load the DLL and obtain a module handle.
* Call `GetProcAddress` to obtain a function pointer to each exported function that the application wants to call. Because applications are calling the DLL's functions through a pointer, the compiler does not generate external references, so there is no need to link with an import library.
* Call `FreeLibrary` when done with the DLL.

## What is implicit linking?
o implicitly link to a DLL, executables must obtain the following from the provider of the DLL:
* A `header file (.h file)` containing the declarations of the exported functions and/or C++ classes. The classes, functions, and data should all have __declspec(dllimport), for more information, see dllexport, dllimport.
* An `import library (.LIB files)` to link with. (The linker creates the import library when the DLL is built.)
* The actual `DLL (.dll file)`.

# GDB
#### Compilation the code
```c
gcc -std=c99 -g -o test test.c
```
* `g` flag means you can see the proper names of variables and functions in your stack frames, get line numbers and see the source as you step around in the executable.
* `-std=C99` flag implies use standard C99 to compile the code.
* `-o` flag writes the build output to an output file.

#### Some useful commands
Here are few useful commands to get started with gdb for the above example:-
* run or r –> executes the program from start to end.
* break or b –> sets breakpoint on a particular line.
* disable -> disable a breakpoint.
* enable –> enable a disabled breakpoint.
* next or n -> executes next line of code, but don’t dive into functions.
* step –> go to next instruction, diving into the function.
* list or l –> displays the code.
* print or p –> used to display the stored value.
* quit or q –> exits out of gdb.
clear –> to clear all breakpoints.
continue –> continue normal execution.

#### Example
https://www.geeksforgeeks.org/gdb-step-by-step-introduction/
```c
// sample program to show undefined behaviour
#include<stdio.h>

int main()
{
    int x;
    int a=x;
    int b=x;
    int c=a+b;
    printf("%d\n",c);
    return 0;
}
```

```c
gdb ./test
l
b 5
info b
disable b
enable b
r
p x
p c
```

# Concurrency
### What is `std::recursive_mutex`?
This mutex can be acquired several times by the same thread.
```c
std::recursive_mutex mutex;

void div(int x){
    std::lock_guard<std::recursive_mutex> lock(mutex);
    i /= x;
}
    
void mul(int x){
    std::lock_guard<std::recursive_mutex> lock(mutex);
    i *= x;
}

void both(int x, int y){
    std::lock_guard<std::recursive_mutex> lock(mutex);
    mul(x);
    div(y);
}
```
Without recursive mutex it will stuck in `deadlock`. In the both() function, the thread acquires the lock and then calls the mul() function. In this function, the threads tries to acquire the lock again, but the lock is already locked. This is a case of deadlock. By default, a thread cannot acquire the same mutex twice. The above example runs very well with `recursive_mutex`. 

### What is `condition variable`?
```c
// Acquire the lock
std::unique_lock<std::mutex> mlock(m_mutex);
m_condVar.wait(mlock, std::bind(&Application::isDataLoaded, this));
```
Wait() will internally release the lock and make the thread to block. As soon as condition variable get signaled, resume the thread and again acquire the lock. Then check if condition is met or not. If condition is met then continue else again go in wait.
