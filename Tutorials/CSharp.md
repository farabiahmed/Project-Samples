### Quick Review Of Key Items in C# ###
---
* Lazy / Eager Loading
* Dynamic / Object Reference Variables

### What are the keywords in C#? ###
---
[Link: List of keywords and details](https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/keywords/)
* Modifiers 
* Access Modifiers
  * public
  * private
  * internal
  * protected
* Statement
  * Selection (if-else, switch)
  * Iteration (do, for, foreach, in, while)
  * Jump (break, continue, goto, return)
  * Exception Handling (throw, try, catch, finaly)
  * checked and unchecked
* Method Parameter (params, in, ref, out)
  * *Parameters declared for a method without in, ref or out, are passed to the called method by value.*
* Operator (as, await, is, new, sizeof, typeof, true, false, stackalloc, nameof)
* Namespace (namespace, using, using static, .Operator, ::Operator, extern alias)
* Access 
* Literal 
* Type 
* Query 
* Contextual
* Internal
* IQueryable
* IEnumerable

### What is internal? ###
---
Internal types or members are accessible only within files in the same assembly, as in this example:
```csharp
public class BaseClass
{  
    // Only accessible within the same assembly.
    internal static int x = 0;
}  
```
Let say I have an AnimalFactory class which produces Dog, Cat and Fish. Using the AnimalFactory makes things incredibly simpler and everything is fine. My question is that Is there a rule or good practice that suggests us hide Dog, Cat and Fish from being used directly? I mean Dog myDog = new Dog(); will be forbidden in the code.
If there is, how can we hide them properly?
* In c#, if the consumers of Dog, Cat and Fish are on different assemblies, you can make the constructors internal, this way only the factory, which is implemented in the same assembly as your "animals" can create them.

### Define property? ###
---
By the property we can access private fields.
We have the three types of properties:
* Read/Write.
* ReadOnly.
* WriteOnly

Example for read/write:
```csharp
public string Name  {
 get{ return mName;}
 set{ mName = value;}        
}
```

### What is the difference between `const` and `readonly`?
---
The readonly keyword differs from the const keyword. A const field can only be initialized at the declaration of the field. A readonly field can be initialized either at the declaration or in a constructor

### What is the use of Checked and Unchecked keywords? ###
---
Checked: Adds exceptions on number overflows.
Unchecked: Numeric types cause no exceptions when they overflow. This is the default behavior.
```csharp
while (true) {
    checked{
        a++; // Will throw overflow error
    }
    unchecked{
        b++;
    }
}
```

#### If default behaviour is Unchecked, then why there is one? ####
The unchecked keyword is used to suppress overflow-checking for integral-type arithmetic operations and conversions. The overflow can be detected at compile time because all the terms of the expression are constants. For example, to supress the overflow checking when adding two const variables.
```csharp
unchecked
{
    int1 = 2147483647 + 10;
}
```

### What is `using static` namespace keyword? ###
---
The using static directive designates a type whose static members and nested types you can access without specifying a type name. 
```csharp
public double Area
{
  get { return Math.PI * Math.Pow(Radius, 2); }
}
```
can be used as; to have cleaner code;
```csharp
using static System.Math;
...
public double Area
{
  get { return PI * Pow(Radius, 2); }
}
```   

### What is the difference between Reflection and dynamic keyword in C#? ###
---
TODO

### What is `yield` keyword? ###
---
Some Highlights are as given below:
* Yields returns the expression without exiting the method. 
* The control is returned to the caller each time the "yield return" statement is encountered and executed.
*  The yield keyword creates a state machine to maintain state information:
   *  When a yield return statement is reached in the iterator method, expression is returned, and the current location in code is retained. Execution is restarted from that location the next time that the iterator function is called.
* You cannot have the yield return statement in a try-catch block though you can have it inside a try-finally block
* You cannot have the yield break statement inside a finally block
* The return type of the method where yield has been used, should be IEnumerable, IEnumerable<T>, IEnumerator, or IEnumerator<T>
* You cannot have a ref or out parameter in your method in which yield has been used
You cannot use the "yield return" or the "yield break" statements inside anonymous methods
* You cannot use the "yield return" or the "yield break" statements inside "unsafe" methods, i.e., methods that are marked with the "unsafe" keyword to denote an unsafe context.

The two forms of the yield statement.

```csharp
yield return <expression>;  
yield break;  
```

Example:
```csharp
static public IEnumerable<string> GetGalaxies()
{
    /* This function will not be triggered until it is used in a foreach iteration statement this is called `lazy loading` */
    yield return "Tadpole";
    yield return "Pinwheel";
    yield return "Milky Way";
    yield return "Andromeda";
}
static void Main(string[] args)
{
    IEnumerable<string> Galaxies = GetGalaxies()
 
    foreach (var item in Galaxies)
        Console.WriteLine(item);
}
```
Yield feature is managed by lazy loading. 

#### What is `lazy loading`then? ####
Lazy loading is a concept where we delay the loading of the object until the point where we need it. Putting in simple words, on demand object loading rather than loading objects unnecessarily. If an entity is not needed, then don't call it.
* Best example is yield example with foreach statement. 

Below are the advantages of lazy loading:
* Minimizes start up time of the application.
* Application consumes less memory because of on-demand loading.
* Unnecessary database SQL execution is avoided.

#### What is `eager loading`then? ####
In eager loading we load all the objects in memory as soon as the object is created.  
